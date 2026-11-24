/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "CaptureCommit.h"
#include "RialtoServerLogging.h"

#include <cstdlib>
#include <cstring>
#include <utility>

namespace firebolt::rialto::server
{
CaptureCommit::CaptureCommit(const std::shared_ptr<wrappers::ICaptureHalLibraryFactory> &captureHalLibraryFactory,
                             const std::shared_ptr<ICaptureAdmission> &admission)
    : m_captureHalLibraryFactory{captureHalLibraryFactory}, m_admission{admission}
{
}

bool CaptureCommit::run(IGstGenericPlayer &player, int sessionId, std::uint32_t captureId,
                        const std::unique_ptr<IMediaPipeline::MediaSource> &videoSource,
                        const VideoRequirements &videoRequirements, CaptureSessionHandle &committed)
{
    // 1. a clear video source
    if (!videoSource)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u: session %d has no video source", captureId, sessionId);
        return false;
    }
    if (videoSource->getHasDrm())
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u: the video of session %d is encrypted", captureId, sessionId);
        return false;
    }

    // 2. westerossink is the video sink
    const char *kOverride = std::getenv("RIALTO_PLAYBIN_VIDEO_SINK");
    if (kOverride && *kOverride && std::strcmp(kOverride, "westerossink") != 0)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u: RIALTO_PLAYBIN_VIDEO_SINK selects '%s', not westerossink",
                                captureId, kOverride);
        return false;
    }

    // 3. a new HAL session for this pipeline
    std::shared_ptr<wrappers::ICaptureHalLibrary> library =
        m_captureHalLibraryFactory ? m_captureHalLibraryFactory->getCaptureHalLibrary() : nullptr;
    wrappers::CaptureSessionPtr session = library ? library->createSession() : nullptr;
    if (!session)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u: no capture HAL session for session %d", captureId, sessionId);
        return false;
    }

    // 4 to 6. the sink, configured and READY; the player rolls the sink back itself if this fails
    if (!player.configureCaptureSink(session.get(), videoSource))
    {
        session.reset(); // the sink is already back at NULL and discarded
        return false;
    }

    // 7 onwards. admission
    CaptureSessionHandle handle{sessionId, captureId, std::move(session)};
    handle.videoRequirements = videoRequirements;
    if (!m_admission || !m_admission->admit(handle))
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u: session %d was not admitted", captureId, sessionId);
        player.releaseCaptureSink(); // returns once the sink is at NULL
        handle.session.reset();      // so only now is the session deleted
        return false;
    }
    committed = std::move(handle);
    RIALTO_SERVER_LOG_MIL("Video frame capture %u committed for session %d", captureId, sessionId);
    return true;
}
} // namespace firebolt::rialto::server
