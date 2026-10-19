/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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

#include "SetBatchDecryption.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
SetBatchDecryption::SetBatchDecryption(GenericPlayerContext &context, bool batchDecryption)
    : m_context{context}, m_batchDecryption{batchDecryption}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetBatchDecryption");
}

SetBatchDecryption::~SetBatchDecryption()
{
    RIALTO_SERVER_LOG_DEBUG("SetBatchDecryption finished");
}

void SetBatchDecryption::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetBatchDecryption, batch decryption: %s", m_batchDecryption ? "true" : "false");

    m_context.batchDecryption = m_batchDecryption;
}
} // namespace firebolt::rialto::server::tasks::generic
