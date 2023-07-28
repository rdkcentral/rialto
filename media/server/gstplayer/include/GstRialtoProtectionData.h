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

#ifndef FIREBOLT_RIALTO_SERVER_GST_RIALTOI_PROTECTION_DATA_H_ // NOLINT(build/header_guard)
#define FIREBOLT_RIALTO_SERVER_GST_RIALTOI_PROTECTION_DATA_H_

#include "IDecryptionService.h"
#include "IGstWrapper.h"

struct GstRialtoProtectionData
{
    int32_t keySessionId = 0;
    uint32_t subsampleCount = 0;
    uint32_t initWithLast15 = 0;
    GstBuffer *key = nullptr;
    GstBuffer *iv = nullptr;
    GstBuffer *subsamples = nullptr;
    firebolt::rialto::CipherMode cipherMode = firebolt::rialto::CipherMode::UNKNOWN;
    uint32_t crypt = 0;
    uint32_t skip = 0;
    bool encryptionPatternSet = false;
    firebolt::rialto::server::IDecryptionService *decryptionService = nullptr;
};

#endif // FIREBOLT_RIALTO_SERVER_GST_RIALTOI_PROTECTION_DATA_H_
