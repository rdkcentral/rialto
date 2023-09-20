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
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::string openCdmErrorToString(const OpenCDMError &status)
{
    switch (status)
    {
    case OpenCDMError::ERROR_NONE:
    {
        return "ERROR_NONE";
    }
    case OpenCDMError::ERROR_UNKNOWN:
    {
        return "ERROR_UNKNOWN";
    }
    case OpenCDMError::ERROR_MORE_DATA_AVAILABLE:
    {
        return "ERROR_MORE_DATA_AVAILABLE";
    }
    case OpenCDMError::ERROR_INTERFACE_NOT_IMPLEMENTED:
    {
        return "ERROR_INTERFACE_NOT_IMPLEMENTED";
    }
    case OpenCDMError::ERROR_BUFFER_TOO_SMALL:
    {
        return "ERROR_BUFFER_TOO_SMALL";
    }
    case OpenCDMError::ERROR_INVALID_ACCESSOR:
    {
        return "ERROR_INVALID_ACCESSOR";
    }
    case OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED:
    {
        return "ERROR_KEYSYSTEM_NOT_SUPPORTED";
    }
    case OpenCDMError::ERROR_INVALID_SESSION:
    {
        return "ERROR_INVALID_SESSION";
    }
    case OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER:
    {
        return "ERROR_INVALID_DECRYPT_BUFFER";
    }
    case OpenCDMError::ERROR_OUT_OF_MEMORY:
    {
        return "ERROR_OUT_OF_MEMORY";
    }
    case OpenCDMError::ERROR_METHOD_NOT_IMPLEMENTED:
    {
        return "ERROR_METHOD_NOT_IMPLEMENTED";
    }
    case OpenCDMError::ERROR_FAIL:
    {
        return "ERROR_FAIL";
    }
    case OpenCDMError::ERROR_INVALID_ARG:
    {
        return "ERROR_INVALID_ARG";
    }
    case OpenCDMError::ERROR_SERVER_INTERNAL_ERROR:
    {
        return "ERROR_SERVER_INTERNAL_ERROR";
    }
    case OpenCDMError::ERROR_SERVER_INVALID_MESSAGE:
    {
        return "ERROR_SERVER_INVALID_MESSAGE";
    }
    case OpenCDMError::ERROR_SERVER_SERVICE_SPECIFIC:
    {
        return "ERROR_SERVER_SERVICE_SPECIFIC";
    }
    case OpenCDMError::ERROR_BUSY_CANNOT_INITIALIZE:
    {
        return "ERROR_BUSY_CANNOT_INITIALIZE";
    }
    }
    return "UNKNOWN";
}

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
    case OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED:
    {
        return firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED;
    }
    case OpenCDMError::ERROR_UNKNOWN:
    case OpenCDMError::ERROR_MORE_DATA_AVAILABLE:
    case OpenCDMError::ERROR_INVALID_ACCESSOR:
    case OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER:
    case OpenCDMError::ERROR_OUT_OF_MEMORY:
    case OpenCDMError::ERROR_FAIL:
    case OpenCDMError::ERROR_INVALID_ARG:
    case OpenCDMError::ERROR_SERVER_INTERNAL_ERROR:
    case OpenCDMError::ERROR_SERVER_INVALID_MESSAGE:
    case OpenCDMError::ERROR_SERVER_SERVICE_SPECIFIC:
    default:
    {
        return firebolt::rialto::MediaKeyErrorStatus::FAIL;
    }
    }
}
} // namespace firebolt::rialto::server
