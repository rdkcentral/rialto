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

#include "GstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::weak_ptr<IGstWrapperFactory> GstWrapperFactory::m_factory;
std::weak_ptr<IGstWrapper> GstWrapperFactory::m_gstWrapper;
std::mutex GstWrapperFactory::m_creationMutex;

std::shared_ptr<IGstWrapperFactory> IGstWrapperFactory::getFactory()
{
    std::shared_ptr<IGstWrapperFactory> factory = GstWrapperFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstWrapperFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer wrapper factory, reason: %s", e.what());
        }

        GstWrapperFactory::m_factory = factory;
    }

    return factory;
}

std::shared_ptr<IGstWrapper> GstWrapperFactory::getGstWrapper()
{
    std::lock_guard<std::mutex> lock{m_creationMutex};

    std::shared_ptr<IGstWrapper> gstWrapper = GstWrapperFactory::m_gstWrapper.lock();

    if (!gstWrapper)
    {
        try
        {
            gstWrapper = std::make_shared<GstWrapper>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer wrapper, reason: %s", e.what());
        }

        GstWrapperFactory::m_gstWrapper = gstWrapper;
    }

    return gstWrapper;
}

void GstWrapper::gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const
{
    va_list vl;
    va_start(vl, field);
    gst_caps_set_simple_valist(caps, field, vl);
    va_end(vl);
}

GstStructure *GstWrapper::gstStructureNew(const gchar *name, const gchar *firstfield, ...) const
{
    GstStructure *structure{nullptr};
    va_list vl;
    va_start(vl, firstfield);
    structure = gst_structure_new_valist(name, firstfield, vl);
    va_end(vl);
    return structure;
}

}; // namespace firebolt::rialto::server
