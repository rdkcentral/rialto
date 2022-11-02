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

#ifndef FIREBOLT_RIALTO_SERVER_OCDM_COMMON_H_
#define FIREBOLT_RIALTO_SERVER_OCDM_COMMON_H_

#include "opencdm/open_cdm.h"
#include "opencdm/open_cdm_adapter.h"
#include "opencdm/open_cdm_ext.h"
#include <MediaCommon.h>
#include <string>

namespace firebolt::rialto::server
{
/**
 * @brief Convers the OpenCdmError status to a string.
 *
 * @param[in] status    : The open cdm error status.
 *
 * @retval the string of the status.
 */
std::string openCdmErrorToString(const OpenCDMError &status);

/**
 * @brief Convers the OpenCdmError status to a MediaKeyErrorStatus.
 *
 * @param[in] status    : The open cdm error status.
 *
 * @retval the media key error status.
 */
firebolt::rialto::MediaKeyErrorStatus convertOpenCdmError(const OpenCDMError &status);
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_OCDM_COMMON_H_
