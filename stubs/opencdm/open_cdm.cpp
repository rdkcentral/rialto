/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "opencdm/open_cdm.h"

extern "C"
{
    OpenCDMSystem *opencdm_create_system(const char keySystem[])
    {
        return nullptr;
    }

    OpenCDMError opencdm_construct_session(OpenCDMSystem *system, const LicenseType licenseType,
                                           const char initDataType[], const uint8_t initData[],
                                           const uint16_t initDataLength, const uint8_t CDMData[],
                                           const uint16_t CDMDataLength, OpenCDMSessionCallbacks *callbacks,
                                           void *userData, OpenCDMSession **session)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_destruct_system(struct OpenCDMSystem *system)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_is_type_supported(const char keySystem[], const char mimeType[])
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_system_get_version(struct OpenCDMSystem *system, char versionStr[])
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_system_get_drm_time(struct OpenCDMSystem *system, uint64_t *time)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_load(struct OpenCDMSession *session)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_update(struct OpenCDMSession *session, const uint8_t keyMessage[],
                                        const uint16_t keyLength)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_remove(struct OpenCDMSession *session)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_close(struct OpenCDMSession *session)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_destruct_session(struct OpenCDMSession *session)
    {
        return ERROR_NONE;
    }

    KeyStatus opencdm_session_status(const struct OpenCDMSession *session, const uint8_t keyId[], const uint8_t length)
    {
        return Usable;
    }
    const char *opencdm_session_id(const struct OpenCDMSession *session)
    {
        return nullptr;
    }

    uint32_t opencdm_session_has_key_id(struct OpenCDMSession *session, const uint8_t length, const uint8_t keyId[])
    {
        return 1;
    }

    OpenCDMError opencdm_session_set_drm_header(struct OpenCDMSession *opencdmSession, const uint8_t drmHeader[],
                                                uint32_t drmHeaderSize)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_system_error(const struct OpenCDMSession *session)
    {
        return ERROR_NONE;
    }
}
