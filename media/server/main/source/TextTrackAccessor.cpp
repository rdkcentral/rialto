/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "TextTrackAccessor.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr char kSubtitleDataEnv[] = "RIALTO_SHOW_SUBTITLE_DATA";
constexpr char kSubtitleLogPathEnv[] = "RIALTO_SUBTITLE_LOG_PATH";
constexpr std::size_t kSubtitleLogChunkSize{400};
constexpr const char *kSubtitleLogDirectories[]{"/opt/logs", "/tmp"};

bool isValidSubtitleLogFileName(const std::string &fileName)
{
    return !fileName.empty() && fileName != "." && fileName != ".." && fileName.find("..") == std::string::npos &&
           std::all_of(fileName.begin(), fileName.end(),
                       [](unsigned char ch) { return std::isalnum(ch) || ch == '.' || ch == '_' || ch == '-'; });
}

bool isEnvironmentVariableEnabled(const char *name)
{
    const char *value = std::getenv(name);
    if (!value || value[0] == '\0')
    {
        return false;
    }

    std::string stringValue(value);
    std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(),
                   [](char character)
                   {
                       if (character >= 'A' && character <= 'Z')
                       {
                           return static_cast<char>(character + ('a' - 'A'));
                       }
                       return character;
                   });
    return stringValue == "1" || stringValue == "true" || stringValue == "yes" || stringValue == "on";
}

#ifdef RIALTO_LOG_DEBUG_ENABLED
bool isSubtitleDebugLoggingEnabled()
{
    const RIALTO_DEBUG_LEVEL logLevels = firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_SERVER);
    return logLevels == RIALTO_DEBUG_LEVEL_EXTERNAL || (logLevels & RIALTO_DEBUG_LEVEL_DEBUG) != 0;
}
#endif

#ifdef RIALTO_LOG_DEBUG_ENABLED
std::string escapeNullBytes(const std::string &data)
{
    std::string escaped;
    escaped.reserve(data.size());
    for (char ch : data)
    {
        if (ch == '\0')
        {
            escaped += "\\0";
        }
        else
        {
            escaped += ch;
        }
    }
    return escaped;
}
#endif

void logSubtitleData(const std::string &data)
{
#ifdef RIALTO_LOG_DEBUG_ENABLED
    if (!isSubtitleDebugLoggingEnabled())
    {
        return;
    }

    const std::string logData{escapeNullBytes(data)};
    if (logData.empty())
    {
        RIALTO_SERVER_LOG_DEBUG("Subtitle data: ");
        return;
    }

    std::size_t offset{};
    bool firstChunk{true};
    while (offset < logData.size())
    {
        std::size_t chunkEnd{std::min(offset + kSubtitleLogChunkSize, logData.size())};
        while (chunkEnd < logData.size() && chunkEnd > offset &&
               (static_cast<unsigned char>(logData[chunkEnd]) & 0xc0) == 0x80)
        {
            --chunkEnd;
        }
        if (chunkEnd == offset)
        {
            chunkEnd = std::min(offset + kSubtitleLogChunkSize, logData.size());
        }

        const std::string chunk{logData.substr(offset, chunkEnd - offset)};
        if (firstChunk)
        {
            RIALTO_SERVER_LOG_DEBUG("Subtitle data: %s\n", chunk.c_str());
            firstChunk = false;
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Subtitle data (continued): %s\n", chunk.c_str());
        }
        offset = chunkEnd;
    }
#else
    static_cast<void>(data);
#endif
}

bool writeSubtitleDataToFile(const char *path, const std::string &data, std::string &error)
{
    namespace fs = std::filesystem;
    const fs::path configuredPath{path};
    if (!configuredPath.is_absolute())
    {
        error = "path must be absolute";
        return false;
    }

    const fs::path fileName{configuredPath.filename()};
    const std::string fileNameString{fileName.string()};
    if (!isValidSubtitleLogFileName(fileNameString))
    {
        error = "filename must contain only letters, digits, '.', '_' or '-' and must not contain '..'";
        return false;
    }

    const fs::path parentDirectory{configuredPath.parent_path()};
    const char *allowedDirectory{};
    for (const char *allowedRoot : kSubtitleLogDirectories)
    {
        if (parentDirectory == fs::path{allowedRoot})
        {
            allowedDirectory = allowedRoot;
            break;
        }
    }
    if (!allowedDirectory)
    {
        error = "directory must be exactly /opt/logs or /tmp";
        return false;
    }

    static std::mutex logMutex;
    const std::lock_guard<std::mutex> lock{logMutex};
    const int directoryFd{open(allowedDirectory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (directoryFd < 0)
    {
        error = "cannot open log directory: " + std::string{std::strerror(errno)};
        return false;
    }

    const int logFileFd{openat(directoryFd, fileNameString.c_str(),
                               O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0664)};
    const int openError{errno};
    if (close(directoryFd) != 0 && logFileFd >= 0)
    {
        const int closeError{errno};
        close(logFileFd);
        error = "cannot close log directory: " + std::string{std::strerror(closeError)};
        return false;
    }
    if (logFileFd < 0)
    {
        error = "cannot open log file: " + std::string{std::strerror(openError)};
        return false;
    }

    struct stat fileStatus
    {
    };
    if (fstat(logFileFd, &fileStatus) != 0)
    {
        const int statError{errno};
        close(logFileFd);
        error = "cannot inspect log file: " + std::string{std::strerror(statError)};
        return false;
    }
    if (!S_ISREG(fileStatus.st_mode))
    {
        close(logFileFd);
        error = "log path must refer to a regular file";
        return false;
    }
    if (fileStatus.st_nlink != 1)
    {
        close(logFileFd);
        error = "log file must not have hard links";
        return false;
    }
    if (fileStatus.st_uid != geteuid())
    {
        close(logFileFd);
        error = "log file must be owned by the Rialto process user";
        return false;
    }

    const std::string logEntry{"Subtitle data: " + data + '\n'};
    const char *nextByte{logEntry.data()};
    std::size_t remaining{logEntry.size()};
    while (remaining > 0)
    {
        const ssize_t result{write(logFileFd, nextByte, remaining)};
        if (result < 0 && errno == EINTR)
        {
            continue;
        }
        if (result < 0)
        {
            const int writeError{errno};
            close(logFileFd);
            error = "cannot write log file: " + std::string{std::strerror(writeError)};
            return false;
        }
        if (result == 0)
        {
            close(logFileFd);
            error = "write returned zero bytes";
            return false;
        }

        const std::size_t bytesWritten{static_cast<std::size_t>(result)};
        if (bytesWritten > remaining)
        {
            close(logFileFd);
            error = "write returned more bytes than requested";
            return false;
        }
        nextByte += bytesWritten;
        remaining -= bytesWritten;
    }

    if (close(logFileFd) != 0)
    {
        error = "cannot close log file: " + std::string{std::strerror(errno)};
        return false;
    }
    return true;
}
} // namespace

namespace firebolt::rialto::server
{
ITextTrackAccessorFactory &ITextTrackAccessorFactory::getFactory()
{
    static TextTrackAccessorFactory factory;
    return factory;
}

std::shared_ptr<ITextTrackAccessor> TextTrackAccessorFactory::getTextTrackAccessor() const
try
{
    static std::shared_ptr<TextTrackAccessor> textTrackAccessor{
        std::make_shared<TextTrackAccessor>(firebolt::rialto::wrappers::ITextTrackPluginWrapperFactory::getFactory()
                                                ->getTextTrackPluginWrapper(),
                                            firebolt::rialto::wrappers::IThunderWrapperFactory::getFactory()
                                                ->getThunderWrapper())};

    return textTrackAccessor;
}
catch (const std::exception &e)
{
    return nullptr;
}

TextTrackAccessor::TextTrackAccessor(
    const std::shared_ptr<firebolt::rialto::wrappers::ITextTrackPluginWrapper> &textTrackPluginWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IThunderWrapper> &thunderWrapper)
    : m_textTrackPluginWrapper{textTrackPluginWrapper}, m_thunderWrapper{thunderWrapper}
{
    if (!createTextTrackControlInterface())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack interfaces");
        throw std::runtime_error("Failed to create TextTrack interfaces");
    }
}

TextTrackAccessor::~TextTrackAccessor() {}

std::optional<uint32_t> TextTrackAccessor::openSession(const std::string &displayName)
{
    uint32_t sessionId = {};
    uint32_t result = m_textTrackWrapper->openSession(displayName, sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_MIL("TextTrack session %u created with display '%s'", sessionId, displayName.c_str());
        return sessionId;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack session with display '%s'; error '%s'", displayName.c_str(),
                            m_thunderWrapper->errorToString(result));
    return std::nullopt;
}

bool TextTrackAccessor::closeSession(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->closeSession(sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_MIL("TextTrack session %u closed", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to close TextTrack session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::pause(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->pauseSession(sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u paused", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to pause TextTrack session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::play(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->resumeSession(sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u resumed", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to resume TextTrack session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::mute(uint32_t sessionId, bool mute)
{
    if (mute)
    {
        uint32_t result = m_textTrackWrapper->muteSession(sessionId);
        if (m_thunderWrapper->isSuccessful(result))
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u muted", sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to mute TextTrack session %u; error %s", sessionId,
                                m_thunderWrapper->errorToString(result));
    }
    else
    {
        uint32_t result = m_textTrackWrapper->unmuteSession(sessionId);
        if (m_thunderWrapper->isSuccessful(result))
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u unmuted", sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to unmute TextTrack session %u; error %s", sessionId,
                                m_thunderWrapper->errorToString(result));
    }

    return false;
}

bool TextTrackAccessor::setPosition(uint32_t sessionId, uint64_t mediaTimestampMs)
{
    uint32_t result = m_textTrackWrapper->sendSessionTimestamp(sessionId, mediaTimestampMs);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u set position to %" PRIu64, sessionId, mediaTimestampMs);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set position of TextTrack session %u to  %" PRIu64 "; error %s", sessionId,
                            mediaTimestampMs, m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::sendData(uint32_t sessionId, const std::string &data, DataType datatype, int64_t displayOffsetMs)
{
    const char *subtitleLogPath = std::getenv(kSubtitleLogPathEnv);
    if (subtitleLogPath && subtitleLogPath[0] != '\0')
    {
        std::string error;
        if (!writeSubtitleDataToFile(subtitleLogPath, data, error))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to capture subtitle data to '%s': %s", subtitleLogPath, error.c_str());
            if (isEnvironmentVariableEnabled(kSubtitleDataEnv))
            {
                logSubtitleData(data);
            }
        }
    }
    else if (isEnvironmentVariableEnabled(kSubtitleDataEnv))
    {
        logSubtitleData(data);
    }

    firebolt::rialto::wrappers::ITextTrackWrapper::DataType wrapperDataType{};
    if (datatype == DataType::WebVTT)
    {
        wrapperDataType = firebolt::rialto::wrappers::ITextTrackWrapper::DataType::WEBVTT;
    }
    else if (datatype == DataType::TTML)
    {
        wrapperDataType = firebolt::rialto::wrappers::ITextTrackWrapper::DataType::TTML;
    }
    else if (datatype == DataType::CC)
    {
        RIALTO_SERVER_LOG_WARN("Data received for ClosedCaptions. It should not happen.");
        return false;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Unknown data type");
        return false;
    }

    const uint32_t result = m_textTrackWrapper->sendSessionData(sessionId, wrapperDataType, displayOffsetMs, data);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_DEBUG("Sending data to TextTrack session %u was successful; size %zu", sessionId, data.size());
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to send data to TextTrack session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::createTextTrackControlInterface()
{
    if (!m_textTrackPluginWrapper)
    {
        RIALTO_SERVER_LOG_ERROR("TextTrackPlugin is null!");
        return false;
    }

    uint32_t openResult = m_textTrackPluginWrapper->open();

    if (m_thunderWrapper->isSuccessful(openResult))
    {
        if (m_textTrackPluginWrapper->isOperational())
        {
            m_textTrackWrapper = m_textTrackPluginWrapper->interface();
            if (m_textTrackWrapper)
            {
                RIALTO_SERVER_LOG_INFO("Created TextTrack interface");
                return true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack interface");
            }
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("TextTrack plugin is NOT operational");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to open TextTrack plugin; error '%s'",
                                m_thunderWrapper->errorToString(openResult));
    }

    return false;
}

bool TextTrackAccessor::setSessionWebVTTSelection(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->setSessionWebVTTSelection(sessionId);

    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("Setting WebVTT selection for session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set WebVTT selection for session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::setSessionTTMLSelection(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->setSessionTTMLSelection(sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("Setting TTML selection for session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set TTML selection for session %u; error %s", sessionId,
                            m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::setSessionCCSelection(uint32_t sessionId, const std::string &service)
{
    uint32_t result = m_textTrackWrapper->setSessionClosedCaptionsService(sessionId, service);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_INFO("Setting CC selection service '%s' for session %u was successful", service.c_str(),
                               sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set CC selection service '%s' for session %u; error %s", service.c_str(),
                            sessionId, m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::resetSession(uint32_t sessionId)
{
    uint32_t result = m_textTrackWrapper->resetSession(sessionId);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_MIL("Resseting session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to reset session %u; error %s", sessionId, m_thunderWrapper->errorToString(result));
    return false;
}

bool TextTrackAccessor::associateVideoDecoder(uint32_t sessionId, const std::string &videoDecoder)
{
    uint32_t result = m_textTrackWrapper->associateVideoDecoder(sessionId, videoDecoder);
    if (m_thunderWrapper->isSuccessful(result))
    {
        RIALTO_SERVER_LOG_MIL("Associating video decoder '%s' with session %u was successful", videoDecoder.c_str(),
                              sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to associate video decoder '%s' with session %u; error %s", videoDecoder.c_str(),
                            sessionId, m_thunderWrapper->errorToString(result));
    return false;
}

} // namespace firebolt::rialto::server
