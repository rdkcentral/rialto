/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
#ifndef FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_CAPABILITIES_MODULE_STUB_H_
#define FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_CAPABILITIES_MODULE_STUB_H_

#include "IIpcServer.h"
#include "MediaCommon.h"
#include "RialtoLogging.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::client::ct
{
class MediaKeysCapabilitiesModuleStub
{
public:
    explicit MediaKeysCapabilitiesModuleStub(
        const std::shared_ptr<::firebolt::rialto::MediaKeysCapabilitiesModule> &mediaKeysCapabilitiesModuleMock);
    ~MediaKeysCapabilitiesModuleStub();

protected:
    std::shared_ptr<::firebolt::rialto::MediaKeysCapabilitiesModule> m_mediaKeysCapabilitiesModuleMock;
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_CAPABILITIES_MODULE_STUB_H_
