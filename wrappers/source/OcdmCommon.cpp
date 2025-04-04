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

#include "OcdmCommon.h"

namespace firebolt::rialto::wrappers
{
firebolt::rialto::MediaKeyErrorStatus convertOpenCdmError(const OpenCDMError &status)
{
    switch (status)
    {
    case OpenCDMError::ERROR_NONE:
    {
        return firebolt::rialto::MediaKeyErrorStatus::OK;
    }
    case OpenCDMError::ERROR_INVALID_SESSION:
    {
        return firebolt::rialto::MediaKeyErrorStatus::BAD_SESSION_ID;
    }
    case OpenCDMError::ERROR_INTERFACE_NOT_IMPLEMENTED:
    {
        return firebolt::rialto::MediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED;
    }
    case OpenCDMError::ERROR_BUFFER_TOO_SMALL:
    {
        return firebolt::rialto::MediaKeyErrorStatus::BUFFER_TOO_SMALL;
    }
    case OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED:
    {
        return firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED;
    }
    default:
    {
        return firebolt::rialto::MediaKeyErrorStatus::FAIL;
    }
    }
}
} // namespace firebolt::rialto::wrappers
