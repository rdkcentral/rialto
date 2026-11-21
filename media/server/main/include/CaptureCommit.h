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

#ifndef FIREBOLT_RIALTO_SERVER_CAPTURE_COMMIT_H_
#define FIREBOLT_RIALTO_SERVER_CAPTURE_COMMIT_H_

#include "ICaptureCommit.h"

#include <memory>

namespace firebolt::rialto::server
{
class CaptureCommit : public ICaptureCommit
{
public:
    /**
     * @param[in] captureHalLibraryFactory : Gives the capture HAL loader that creates sessions.
     * @param[in] admission                : Pool admission (R8).
     */
    CaptureCommit(const std::shared_ptr<wrappers::ICaptureHalLibraryFactory> &captureHalLibraryFactory,
                  const std::shared_ptr<ICaptureAdmission> &admission);
    ~CaptureCommit() override = default;

    bool run(IGstGenericPlayer &player, int sessionId, std::uint32_t captureId,
             const std::unique_ptr<IMediaPipeline::MediaSource> &videoSource, CaptureSessionHandle &committed) override;

private:
    std::shared_ptr<wrappers::ICaptureHalLibraryFactory> m_captureHalLibraryFactory;
    std::shared_ptr<ICaptureAdmission> m_admission;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_COMMIT_H_
