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

#include "CaptureCore.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
CaptureCore::CaptureCore(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId,
                         const std::shared_ptr<IFrameImporter> &importer)
    : m_ipc{ipc}, m_captureId{captureId}, m_importer{importer}
{
}

CaptureCore::~CaptureCore() = default;

GLFrameAcquisition CaptureCore::acquireCurrentFrame()
{
    std::lock_guard<std::mutex> lock{m_acquireMutex};
    return GLFrameAcquisition{false, m_currentFrame};
}
} // namespace firebolt::rialto::client
