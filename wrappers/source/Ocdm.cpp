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

#include "Ocdm.h"
#include "OcdmCommon.h"

namespace firebolt::rialto::wrappers
{
std::weak_ptr<IOcdm> OcdmFactory::m_ocdm;
std::mutex OcdmFactory::m_creationMutex;

std::shared_ptr<IOcdmFactory> IOcdmFactory::createFactory()
{
    std::shared_ptr<OcdmFactory> factory;

    try
    {
        factory = std::make_shared<OcdmFactory>();
    }
    catch (const std::exception &e)
    {
    }

    return factory;
}

std::shared_ptr<IOcdm> OcdmFactory::getOcdm() const
{
    std::lock_guard<std::mutex> lock{m_creationMutex};

    std::shared_ptr<IOcdm> ocdm = OcdmFactory::m_ocdm.lock();

    if (!ocdm)
    {
        try
        {
            ocdm = std::make_shared<Ocdm>();
        }
        catch (const std::exception &e)
        {
        }

        OcdmFactory::m_ocdm = ocdm;
    }

    return ocdm;
}

MediaKeyErrorStatus Ocdm::isTypeSupported(std::string keySystem)
{
    // mimeType is currently ignored by ocdm
    OpenCDMError status = opencdm_is_type_supported(keySystem.c_str(), "");
    return convertOpenCdmError(status);
}

} // namespace firebolt::rialto::wrappers
