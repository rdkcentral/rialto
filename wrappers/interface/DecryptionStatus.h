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

#ifndef FIREBOLT_RIALTO_WRAPPERS_DECRYPTION_STATUS_H_
#define FIREBOLT_RIALTO_WRAPPERS_DECRYPTION_STATUS_H_

namespace firebolt::rialto::wrappers
{
/**
 * @brief Decryption status.
 */
enum class DecryptionStatus
{
    OK,
    INVALID_SESSION,
    INTERFACE_NOT_IMPLEMENTED,
    BUFFER_TOO_SMALL,
    KEYSYSTEM_NOT_SUPPORTED,
    OUTPUT_RESTRICTED,
    KEY_NOT_USABLE,
    FAIL
};

} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_DECRYPTION_STATUS_H_
