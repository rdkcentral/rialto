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

#include "Utils.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include <string.h>
#include <iostream>

namespace
{
const char *underflowSignals[]{"buffer-underflow-callback", "vidsink-underflow-callback"};
} // namespace

namespace firebolt::rialto::server
{
bool isVideoDecoder(const firebolt::rialto::wrappers::IGstWrapper &gstWrapper, GstElement *element)
{
    if (!element)
    {
        return false;
    }
    GstElementFactory *factory{gstWrapper.gstElementGetFactory(element)};
    if (!factory)
    {
        return false;
    }
    return gstWrapper.gstElementFactoryListIsType(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) &&
           gstWrapper.gstElementFactoryListIsType(factory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO);
}

bool isAudioDecoder(const firebolt::rialto::wrappers::IGstWrapper &gstWrapper, GstElement *element)
{
    if (!element)
    {
        return false;
    }
    GstElementFactory *factory{gstWrapper.gstElementGetFactory(element)};
    if (!factory)
    {
        return false;
    }
    return gstWrapper.gstElementFactoryListIsType(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) &&
           gstWrapper.gstElementFactoryListIsType(factory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO);
}

std::string getUnderflowSignalName(const firebolt::rialto::wrappers::IGlibWrapper &glibWrapper, GstElement *element)
{
    std::cout << "getUnderflowSignalName 1" << std::endl;
    for (GType type = glibWrapper.gObjectType(element); type; type = glibWrapper.gTypeParent(type))
    {
        std::cout << "getUnderflowSignalName 2" << std::endl;
        if (type == GST_TYPE_ELEMENT || type == GST_TYPE_OBJECT)
            break;

        std::cout << "getUnderflowSignalName 3" << std::endl;
        if (type == GST_TYPE_BIN && glibWrapper.gObjectType(element) != GST_TYPE_BIN)
            continue;

        std::cout << "getUnderflowSignalName 4" << std::endl;
        guint nsignals{0};
        guint *signals = glibWrapper.gSignalListIds(type, &nsignals);
        for (guint i = 0; i < nsignals; i++)
        {
            std::cout << "getUnderflowSignalName 5" << std::endl;
            GSignalQuery query;
            glibWrapper.gSignalQuery(signals[i], &query);
            std::cout << "getUnderflowSignalName 6" << std::endl;
            for (const char *signalName : underflowSignals)
            {
                std::cout << "getUnderflowSignalName 7" << std::endl;
                if (strcmp(signalName, query.signal_name) == 0)
                {
                    std::cout << "getUnderflowSignalName 8" << std::endl;
                    glibWrapper.gFree(signals);
                    return std::string(signalName);
                }
            }
        }
        std::cout << "getUnderflowSignalName 9" << std::endl;
        glibWrapper.gFree(signals);
        std::cout << "getUnderflowSignalName 10" << std::endl;
    }
    return "";
}
} // namespace firebolt::rialto::server
