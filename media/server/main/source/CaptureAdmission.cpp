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

#include "ICaptureAdmission.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
namespace
{
class UnavailableCaptureAdmissionFactory : public ICaptureAdmissionFactory
{
public:
    std::shared_ptr<ICaptureAdmission> createCaptureAdmission() const override
    {
        return std::make_shared<UnavailableCaptureAdmission>();
    }
};
} // namespace

std::shared_ptr<ICaptureAdmissionFactory> &ICaptureAdmissionFactory::instance()
{
    static std::shared_ptr<ICaptureAdmissionFactory> factory{std::make_shared<UnavailableCaptureAdmissionFactory>()};
    return factory;
}

bool UnavailableCaptureAdmission::admit(CaptureSessionHandle &handle)
{
    RIALTO_SERVER_LOG_WARN("Video frame capture %u of session %d cannot be admitted: pool admission is not available",
                           handle.captureId, handle.sessionId);
    return false;
}
} // namespace firebolt::rialto::server
