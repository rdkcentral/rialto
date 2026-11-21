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

#ifndef FIREBOLT_RIALTO_SERVER_I_CAPTURE_COMMIT_H_
#define FIREBOLT_RIALTO_SERVER_I_CAPTURE_COMMIT_H_

#include "ICaptureAdmission.h"
#include "IGstGenericPlayer.h"
#include "IMediaPipeline.h"

#include <cstdint>
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief The capture configuration step of allSourcesAttached() (proposal 3.3 steps 1 to 6, then admission).
 */
class ICaptureCommit
{
public:
    ICaptureCommit() = default;
    virtual ~ICaptureCommit() = default;

    ICaptureCommit(const ICaptureCommit &) = delete;
    ICaptureCommit &operator=(const ICaptureCommit &) = delete;
    ICaptureCommit(ICaptureCommit &&) = delete;
    ICaptureCommit &operator=(ICaptureCommit &&) = delete;

    /**
     * @brief Runs the step on the main thread. On any failure everything it did is rolled back: the sink is
     *        discarded and the HAL session deleted after the sink is at NULL.
     *
     * @param[in]  player      : The pipeline's player.
     * @param[in]  sessionId   : The pipeline session.
     * @param[in]  captureId   : The attached capture.
     * @param[in]  videoSource : The attached video source, or null if there is none.
     * @param[out] committed   : The admitted session, on success.
     *
     * @retval true on success.
     */
    virtual bool run(IGstGenericPlayer &player, int sessionId, std::uint32_t captureId,
                     const std::unique_ptr<IMediaPipeline::MediaSource> &videoSource,
                     CaptureSessionHandle &committed) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_CAPTURE_COMMIT_H_
