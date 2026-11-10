/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cinttypes>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

#include "MainThread.h"
#include "MediaKeySession.h"
#include "MediaKeysServerInternal.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto
{
const char *mediaKeyErrorStatusToString(const MediaKeyErrorStatus &status)
{
    switch (status)
    {
    case firebolt::rialto::MediaKeyErrorStatus::OK:
        return "OK";
    case firebolt::rialto::MediaKeyErrorStatus::BAD_SESSION_ID:
        return "BAD_SESSION_ID";
    case firebolt::rialto::MediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED:
        return "INTERFACE_NOT_IMPLEMENTED";
    case firebolt::rialto::MediaKeyErrorStatus::BUFFER_TOO_SMALL:
        return "BUFFER_TOO_SMALL";
    case firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED:
        return "NOT_SUPPORTED";
    case firebolt::rialto::MediaKeyErrorStatus::OUTPUT_RESTRICTED:
        return "OUTPUT_RESTRICTED";
    default:
        return "FAIL";
    }
}

std::shared_ptr<IMediaKeysFactory> IMediaKeysFactory::createFactory()
{
    return server::IMediaKeysServerInternalFactory::createFactory();
}
} // namespace firebolt::rialto

namespace
{
constexpr const char *kOcdmCallTimeoutEnv{"RIALTO_OCDM_CALL_TIMEOUT_MS"};

struct TimedTaskState
{
    std::mutex mutex;
    std::condition_variable cv;
    bool started{false};
    bool done{false};
    bool abandoned{false};
};

int64_t toMs(std::chrono::steady_clock::duration duration)
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

/**
 * @brief Keeps objects that may still be used by a blocked OCDM call alive until the process exits.
 *
 * Destroying them could block the caller (e.g. joining a stuck thread) or cause a use-after-free.
 */
struct DecryptBuffers
{
    std::mutex mutex;
    bool started{false};
    bool finished{false};
    bool callerGone{false};
    GstBuffer *buffer{nullptr};
    GstCaps *caps{nullptr};

    ~DecryptBuffers()
    {
        if (buffer || caps)
        {
            // This may run on the OCDM thread. Finalizing the buffer can call back into CdmService, which may then
            // need the OCDM thread, so the references are dropped on a separate thread.
            std::thread(
                [buffer = buffer, caps = caps]()
                {
                    if (buffer)
                    {
                        gst_buffer_unref(buffer);
                    }
                    if (caps)
                    {
                        gst_caps_unref(caps);
                    }
                })
                .detach();
        }
    }
};

void retainWhileOcdmBlocked(const std::shared_ptr<void> &object)
{
    static std::mutex retainedMutex;
    static auto *retained = new std::vector<std::shared_ptr<void>>();
    std::lock_guard<std::mutex> lock{retainedMutex};
    if (std::find(retained->begin(), retained->end(), object) == retained->end())
    {
        retained->push_back(object);
    }
}
} // namespace

namespace firebolt::rialto::server
{
int32_t generateSessionId()
{
    static std::atomic<int32_t> keySessionId{0};
    return keySessionId++;
}

std::shared_ptr<IMediaKeysServerInternalFactory> IMediaKeysServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaKeysServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaKeysServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaKeys> MediaKeysServerInternalFactory::createMediaKeys(const std::string &keySystem) const
{
    RIALTO_SERVER_LOG_ERROR("This function can't be used by rialto server. Please use createMediaKeysServerInternal");
    return nullptr;
}

std::unique_ptr<IMediaKeysServerInternal>
MediaKeysServerInternalFactory::createMediaKeysServerInternal(const std::string &keySystem) const
{
    std::unique_ptr<IMediaKeysServerInternal> mediaKeys;
    try
    {
        const std::chrono::milliseconds kOcdmCallTimeout{MediaKeysServerInternal::getOcdmCallTimeoutFromEnv()};
        std::shared_ptr<IMainThreadFactory> mainThreadFactory{server::IMainThreadFactory::createFactory()};
        std::shared_ptr<IMediaKeySessionFactory> mediaKeySessionFactory{server::IMediaKeySessionFactory::createFactory()};
        if (kOcdmCallTimeout.count() > 0)
        {
            RIALTO_SERVER_LOG_MIL("OCDM calls for %s are bounded to %" PRId64 " ms and run on a dedicated thread",
                                  keySystem.c_str(), static_cast<int64_t>(kOcdmCallTimeout.count()));
            mainThreadFactory = std::make_shared<server::DedicatedMainThreadFactory>();
            mediaKeySessionFactory = std::make_shared<server::MediaKeySessionFactory>(mainThreadFactory);
        }
        mediaKeys = std::make_unique<server::MediaKeysServerInternal>(keySystem, mainThreadFactory,
                                                                      wrappers::IOcdmSystemFactory::createFactory(),
                                                                      mediaKeySessionFactory, kOcdmCallTimeout);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys, reason: %s", e.what());
    }

    return mediaKeys;
}
}; // namespace firebolt::rialto::server

namespace firebolt::rialto::server
{
struct MediaKeysServerInternal::OcdmCallState
{
    std::mutex mutex;
    bool blocked{false};
    std::string blockedOperation;
    std::chrono::steady_clock::time_point blockedSince;
};

std::chrono::milliseconds MediaKeysServerInternal::getOcdmCallTimeoutFromEnv()
{
    const char *value = std::getenv(kOcdmCallTimeoutEnv);
    if (!value || '\0' == *value)
    {
        return std::chrono::milliseconds{0};
    }
    char *end{nullptr};
    errno = 0;
    const int64_t kTimeoutMs{std::strtoll(value, &end, 10)};
    if (0 != errno || end == value || '\0' != *end || kTimeoutMs < 0)
    {
        RIALTO_SERVER_LOG_WARN("Invalid %s value '%s', OCDM calls are not bounded", kOcdmCallTimeoutEnv, value);
        return std::chrono::milliseconds{0};
    }
    return std::chrono::milliseconds{kTimeoutMs};
}

MediaKeysServerInternal::MediaKeysServerInternal(
    const std::string &keySystem, const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
    const std::shared_ptr<firebolt::rialto::wrappers::IOcdmSystemFactory> &ocdmSystemFactory,
    const std::shared_ptr<IMediaKeySessionFactory> &mediaKeySessionFactory, std::chrono::milliseconds ocdmCallTimeout)
    : m_kOcdmCallTimeout{ocdmCallTimeout}, m_ocdmCallState{std::make_shared<OcdmCallState>()},
      m_mediaKeySessionFactory(mediaKeySessionFactory), m_kKeySystem(keySystem)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    m_mainThread = mainThreadFactory->getMainThread();
    if (!m_mainThread)
    {
        throw std::runtime_error("Failed to get the main thread");
    }
    m_mainThreadClientId = m_mainThread->registerClient();

    if (!ocdmSystemFactory)
    {
        throw std::runtime_error("No ocdmSystemFactory");
    }

    auto ocdmSystem = std::make_shared<std::shared_ptr<firebolt::rialto::wrappers::IOcdmSystem>>();
    auto task = [ocdmSystem, ocdmSystemFactory, keySystem]()
    {
        *ocdmSystem = ocdmSystemFactory->createOcdmSystem(keySystem);
        if (!*ocdmSystem)
        {
            RIALTO_SERVER_LOG_ERROR("Ocdm system could not be created");
        }
    };

    if (!runTask("createOcdmSystem", task))
    {
        // The call may still complete later, destroying its result or the thread here could block
        retainWhileOcdmBlocked(ocdmSystem);
        retainWhileOcdmBlocked(m_mainThread);
        throw std::runtime_error("MediaKeys construction failed");
    }
    if (!*ocdmSystem)
    {
        throw std::runtime_error("MediaKeys construction failed");
    }
    m_ocdmSystem = *ocdmSystem;
}

MediaKeysServerInternal::~MediaKeysServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto ocdmSystem = std::make_shared<std::shared_ptr<firebolt::rialto::wrappers::IOcdmSystem>>(std::move(m_ocdmSystem));
    auto mediaKeySessions = std::make_shared<std::map<int32_t, std::unique_ptr<IMediaKeySession>>>();
    if (m_kOcdmCallTimeout.count() > 0)
    {
        // Session destructors close and destruct the OCDM sessions, so they have to be bounded too
        mediaKeySessions->swap(m_mediaKeySessions);
    }
    const size_t kMediaKeySessionsCount{mediaKeySessions->size()};
    IMainThread *mainThread{m_mainThread.get()};
    const uint32_t kMainThreadClientId{m_mainThreadClientId};
    auto task = [mediaKeySessions, ocdmSystem, mainThread, kMainThreadClientId]()
    {
        mediaKeySessions->clear();
        ocdmSystem->reset();

        mainThread->unregisterClient(kMainThreadClientId);
    };

    if (!runTask("destroyOcdmSystem", task))
    {
        RIALTO_SERVER_LOG_ERROR("OCDM is not responsive, leaking the OCDM system and %zu key session(s)",
                                kMediaKeySessionsCount);
        retainWhileOcdmBlocked(ocdmSystem);
        retainWhileOcdmBlocked(mediaKeySessions);
        retainWhileOcdmBlocked(m_mainThread);
    }
}

bool MediaKeysServerInternal::runTask(const char *operation, const IMainThread::Task &task, bool canAbandonRunningTask)
{
    if (m_kOcdmCallTimeout.count() <= 0)
    {
        m_mainThread->enqueueTaskAndWait(m_mainThreadClientId, task);
        return true;
    }

    {
        std::lock_guard<std::mutex> lock{m_ocdmCallState->mutex};
        if (m_ocdmCallState->blocked)
        {
            RIALTO_SERVER_LOG_ERROR("Skipping %s, OCDM call %s has been blocked for %" PRId64 " ms", operation,
                                    m_ocdmCallState->blockedOperation.c_str(),
                                    toMs(std::chrono::steady_clock::now() - m_ocdmCallState->blockedSince));
            return false;
        }
    }

    auto taskState = std::make_shared<TimedTaskState>();
    auto ocdmCallState = m_ocdmCallState;
    const std::string kOperation{operation};
    const std::chrono::milliseconds kTimeout{m_kOcdmCallTimeout};
    auto timedTask = [taskState, ocdmCallState, task, kOperation, kTimeout]()
    {
        bool abandoned{false};
        {
            std::lock_guard<std::mutex> lock{taskState->mutex};
            abandoned = taskState->abandoned;
            taskState->started = !abandoned;
        }
        const auto kStart = std::chrono::steady_clock::now();
        if (abandoned)
        {
            RIALTO_SERVER_LOG_WARN("%s skipped, its caller has already timed out", kOperation.c_str());
        }
        else
        {
            task();
            {
                std::lock_guard<std::mutex> lock{taskState->mutex};
                taskState->done = true;
            }
            taskState->cv.notify_all();
        }
        const int64_t kDurationMs{toMs(std::chrono::steady_clock::now() - kStart)};
        if (!abandoned && kDurationMs * 2 >= kTimeout.count())
        {
            RIALTO_SERVER_LOG_WARN("Slow OCDM call %s took %" PRId64 " ms (timeout %" PRId64 " ms)", kOperation.c_str(),
                                   kDurationMs, static_cast<int64_t>(kTimeout.count()));
        }
        std::lock_guard<std::mutex> lock{ocdmCallState->mutex};
        if (ocdmCallState->blocked)
        {
            RIALTO_SERVER_LOG_MIL("OCDM is responsive again, %s was blocked for %" PRId64 " ms",
                                  ocdmCallState->blockedOperation.c_str(),
                                  toMs(std::chrono::steady_clock::now() - ocdmCallState->blockedSince));
            ocdmCallState->blocked = false;
        }
    };

    const auto kStart = std::chrono::steady_clock::now();
    m_mainThread->enqueueTask(m_mainThreadClientId, timedTask);

    std::unique_lock<std::mutex> lock{taskState->mutex};
    if (taskState->cv.wait_for(lock, m_kOcdmCallTimeout, [&taskState]() { return taskState->done; }))
    {
        return true;
    }
    if (taskState->started && !canAbandonRunningTask)
    {
        RIALTO_SERVER_LOG_WARN("%s exceeded the OCDM call timeout of %" PRId64 " ms, waiting for it to complete",
                               operation, static_cast<int64_t>(m_kOcdmCallTimeout.count()));
        taskState->cv.wait(lock, [&taskState]() { return taskState->done; });
        return true;
    }
    if (!taskState->started)
    {
        taskState->abandoned = true;
    }
    {
        std::lock_guard<std::mutex> stateLock{m_ocdmCallState->mutex};
        m_ocdmCallState->blocked = true;
        m_ocdmCallState->blockedOperation = taskState->started ? kOperation : "preceding " + kOperation;
        m_ocdmCallState->blockedSince = kStart;
    }
    RIALTO_SERVER_LOG_ERROR("OCDM call %s did not %s within %" PRId64
                            " ms, failing it and rejecting OCDM calls until OCDM "
                            "is responsive again",
                            operation, taskState->started ? "complete" : "start",
                            static_cast<int64_t>(m_kOcdmCallTimeout.count()));
    return false;
}

bool MediaKeysServerInternal::isOcdmCallBlocked() const
{
    std::lock_guard<std::mutex> lock{m_ocdmCallState->mutex};
    return m_ocdmCallState->blocked;
}

MediaKeyErrorStatus MediaKeysServerInternal::selectKeyId(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId, keyId]() { *status = selectKeyIdInternal(keySessionId, keyId); };

    if (!runTask("selectKeyId", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::selectKeyIdInternal(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->selectKeyId(keyId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to select key id");
        return status;
    }

    return status;
}

bool MediaKeysServerInternal::containsKey(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result = std::make_shared<bool>(false);
    auto task = [this, result, keySessionId, keyId]() { *result = containsKeyInternal(keySessionId, keyId); };

    if (!runTask("containsKey", task))
    {
        return false;
    }
    return *result;
}

bool MediaKeysServerInternal::containsKeyInternal(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return false;
    }

    return sessionIter->second->containsKey(keyId);
}

MediaKeyErrorStatus MediaKeysServerInternal::createKeySession(KeySessionType sessionType,
                                                              std::weak_ptr<IMediaKeysClient> client,
                                                              int32_t &keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, int32_t>>(MediaKeyErrorStatus::FAIL, -1);
    auto task = [this, result, sessionType, client]()
    { result->first = createKeySessionInternal(sessionType, client, result->second); };

    if (!runTask("createKeySession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    if (MediaKeyErrorStatus::OK == result->first)
    {
        keySessionId = result->second;
    }
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::createKeySessionInternal(KeySessionType sessionType,
                                                                      std::weak_ptr<IMediaKeysClient> client,
                                                                      int32_t &keySessionId)
{
    int32_t keySessionIdTemp = generateSessionId();
    std::unique_ptr<IMediaKeySession> mediaKeySession =
        m_mediaKeySessionFactory->createMediaKeySession(m_kKeySystem, keySessionIdTemp, *m_ocdmSystem, sessionType,
                                                        client);
    if (!mediaKeySession)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create a new media key session");
        return MediaKeyErrorStatus::FAIL;
    }
    keySessionId = keySessionIdTemp;
    m_mediaKeySessions.emplace(std::make_pair(keySessionId, std::move(mediaKeySession)));

    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus MediaKeysServerInternal::generateRequest(int32_t keySessionId, InitDataType initDataType,
                                                             const std::vector<uint8_t> &initData,
                                                             const LimitedDurationLicense &ldlState)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId, initDataType, initData, ldlState]()
    { *status = generateRequestInternal(keySessionId, initDataType, initData, ldlState); };

    if (!runTask("generateRequest", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::generateRequestInternal(int32_t keySessionId, InitDataType initDataType,
                                                                     const std::vector<uint8_t> &initData,
                                                                     const LimitedDurationLicense &ldlState)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->generateRequest(initDataType, initData, ldlState);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to generate request for the key session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::loadSession(int32_t keySessionId, const std::string &cdmKeySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId, cdmKeySessionId]()
    { *status = loadSessionInternal(keySessionId, cdmKeySessionId); };

    if (!runTask("loadSession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::loadSessionInternal(int32_t keySessionId, const std::string &cdmKeySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->loadSession(cdmKeySessionId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load the session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::updateSession(int32_t keySessionId, const std::vector<uint8_t> &responseData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId, responseData]()
    { *status = updateSessionInternal(keySessionId, responseData); };

    if (!runTask("updateSession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::updateSessionInternal(int32_t keySessionId,
                                                                   const std::vector<uint8_t> &responseData)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->updateSession(responseData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to update the session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::setDrmHeader(int32_t keySessionId, const std::vector<uint8_t> &requestData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId, requestData]()
    { *status = setDrmHeaderInternal(keySessionId, requestData); };

    if (!runTask("setDrmHeader", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::setDrmHeaderInternal(int32_t keySessionId,
                                                                  const std::vector<uint8_t> &requestData)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->setDrmHeader(requestData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set drm header");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::closeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId]() { *status = closeKeySessionInternal(keySessionId); };

    if (!runTask("closeKeySession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::closeKeySessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->closeKeySession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to close the key session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::removeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId]() { *status = removeKeySessionInternal(keySessionId); };

    if (!runTask("removeKeySession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::removeKeySessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->removeKeySession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove the key session %d", keySessionId);
        return status;
    }
    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteDrmStore()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status]() { *status = m_ocdmSystem->deleteSecureStore(); };

    if (!runTask("deleteDrmStore", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteKeyStore()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status]() { *status = m_ocdmSystem->deleteKeyStore(); };

    if (!runTask("deleteKeyStore", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmStoreHash(std::vector<unsigned char> &drmStoreHash)
{
    constexpr size_t kHashSize{256};
    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, std::vector<unsigned char>>>(MediaKeyErrorStatus::FAIL,
                                                                                               std::vector<unsigned char>(
                                                                                                   kHashSize));
    auto task = [this, result]() { result->first = m_ocdmSystem->getSecureStoreHash(&result->second[0], kHashSize); };
    drmStoreHash.resize(kHashSize);
    if (!runTask("getDrmStoreHash", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    drmStoreHash = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::getKeyStoreHash(std::vector<unsigned char> &keyStoreHash)
{
    constexpr size_t kHashSize{256};
    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, std::vector<unsigned char>>>(MediaKeyErrorStatus::FAIL,
                                                                                               std::vector<unsigned char>(
                                                                                                   kHashSize));
    auto task = [this, result]() { result->first = m_ocdmSystem->getKeyStoreHash(&result->second[0], kHashSize); };
    keyStoreHash.resize(kHashSize);
    if (!runTask("getKeyStoreHash", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    keyStoreHash = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLdlSessionsLimit(uint32_t &ldlLimit)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, uint32_t>>(MediaKeyErrorStatus::FAIL, 0);
    auto task = [this, result]() { result->first = m_ocdmSystem->getLdlSessionsLimit(&result->second); };

    if (!runTask("getLdlSessionsLimit", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    ldlLimit = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLastDrmError(int32_t keySessionId, uint32_t &errorCode)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, uint32_t>>(MediaKeyErrorStatus::FAIL, 0);
    auto task = [this, result, keySessionId]() { result->first = getLastDrmErrorInternal(keySessionId, result->second); };

    if (!runTask("getLastDrmError", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    errorCode = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLastDrmErrorInternal(int32_t keySessionId, uint32_t &errorCode)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->getLastDrmError(errorCode);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get last drm error");
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmTime(uint64_t &drmTime)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result = std::make_shared<std::pair<MediaKeyErrorStatus, uint64_t>>(MediaKeyErrorStatus::FAIL, 0);
    auto task = [this, result]() { result->first = m_ocdmSystem->getDrmTime(&result->second); };

    if (!runTask("getDrmTime", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    drmTime = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::getCdmKeySessionId(int32_t keySessionId, std::string &cdmKeySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto result =
        std::make_shared<std::pair<MediaKeyErrorStatus, std::string>>(MediaKeyErrorStatus::FAIL, cdmKeySessionId);
    auto task = [this, result, keySessionId]()
    { result->first = getCdmKeySessionIdInternal(keySessionId, result->second); };

    if (!runTask("getCdmKeySessionId", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    cdmKeySessionId = result->second;
    return result->first;
}

MediaKeyErrorStatus MediaKeysServerInternal::releaseKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto task = [this, status, keySessionId]() { *status = releaseKeySessionInternal(keySessionId); };

    if (!runTask("releaseKeySession", task))
    {
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::releaseKeySessionInternal(int32_t keySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    m_mediaKeySessions.erase(sessionIter);
    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus MediaKeysServerInternal::getCdmKeySessionIdInternal(int32_t keySessionId, std::string &cdmKeySessionId)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->getCdmKeySessionId(cdmKeySessionId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get cdm key session id");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::decrypt(int32_t keySessionId, GstBuffer *encrypted, GstCaps *caps)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto status = std::make_shared<MediaKeyErrorStatus>(MediaKeyErrorStatus::FAIL);
    auto buffers = std::make_shared<DecryptBuffers>();
    auto task = [this, status, keySessionId, encrypted, caps, buffers]()
    {
        {
            std::lock_guard<std::mutex> lock{buffers->mutex};
            if (buffers->callerGone)
            {
                return;
            }
            buffers->started = true;
        }
        *status = decryptInternal(keySessionId, encrypted, caps);
        std::lock_guard<std::mutex> lock{buffers->mutex};
        buffers->finished = true;
    };

    if (!runTask("decrypt", task))
    {
        std::lock_guard<std::mutex> lock{buffers->mutex};
        if (!buffers->started)
        {
            buffers->callerGone = true;
        }
        else if (!buffers->finished)
        {
            // The timed out decrypt is still using the buffer and caps, keep them alive until it is released
            buffers->buffer = gst_buffer_ref(encrypted);
            if (caps)
            {
                buffers->caps = gst_caps_ref(caps);
            }
        }
        return MediaKeyErrorStatus::FAIL;
    }
    return *status;
}

MediaKeyErrorStatus MediaKeysServerInternal::decryptInternal(int32_t keySessionId, GstBuffer *encrypted, GstCaps *caps)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session %d", keySessionId);
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->decrypt(encrypted, caps);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_DEBUG("Failed to decrypt buffer.");
        return status;
    }
    RIALTO_SERVER_LOG_INFO("Successfully decrypted buffer.");

    return status;
}

void MediaKeysServerInternal::ping(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto handler = std::make_shared<std::unique_ptr<IHeartbeatHandler>>(std::move(heartbeatHandler));
    auto task = [handler]() { handler->reset(); };

    if (!runTask("ping", task, false))
    {
        // Restarting the server does not recover a blocked OCDM, so keep the server (and non-DRM playback) alive.
        RIALTO_SERVER_LOG_ERROR(
            "MediaKeys thread is blocked by OCDM, acknowledging heartbeat; DRM operations will fail");
        handler->reset();
    }
}

MediaKeyErrorStatus MediaKeysServerInternal::getMetricSystemData(std::vector<uint8_t> &buffer)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    uint32_t bufferLength{1024};
    const uint32_t kMaxBufferLength{65536};
    MediaKeyErrorStatus status;
    buffer.resize(bufferLength);

    for (int attempts = 0; bufferLength <= kMaxBufferLength; ++attempts)
    {
        auto result =
            std::make_shared<std::tuple<MediaKeyErrorStatus, uint32_t, std::vector<uint8_t>>>(MediaKeyErrorStatus::FAIL,
                                                                                              bufferLength, buffer);
        auto task = [this, result]()
        { std::get<0>(*result) = m_ocdmSystem->getMetricSystemData(std::get<1>(*result), std::get<2>(*result)); };
        if (!runTask("getMetricSystemData", task))
        {
            return MediaKeyErrorStatus::FAIL;
        }
        status = std::get<0>(*result);
        bufferLength = std::get<1>(*result);
        buffer = std::move(std::get<2>(*result));

        if (status != MediaKeyErrorStatus::BUFFER_TOO_SMALL)
        {
            break;
        }

        if (bufferLength >= kMaxBufferLength)
        {
            RIALTO_SERVER_LOG_ERROR("Buffer size %u exceeds the maximum allowed size %u", bufferLength, kMaxBufferLength);
            return MediaKeyErrorStatus::BUFFER_TOO_SMALL;
        }

        RIALTO_SERVER_LOG_WARN("Buffer is too small, resizing from %u to %u", bufferLength, bufferLength * 2);
        bufferLength *= 2;
        buffer.resize(bufferLength);
    }

    if (status == MediaKeyErrorStatus::OK)
    {
        // If the buffer remains larger than bufferLength (due to a previous resize),
        // the client may have values in the extra space. So this resize would ensure the buffer is trimmed to the correct size.
        buffer.resize(bufferLength);
        RIALTO_SERVER_LOG_DEBUG("Successfully retrieved metric system data, final buffer length: %u", bufferLength);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to retrieve metric system data, status: %s, last buffer length tried: %u",
                                firebolt::rialto::mediaKeyErrorStatusToString(status), bufferLength);
    }
    return status;
}

}; // namespace firebolt::rialto::server
