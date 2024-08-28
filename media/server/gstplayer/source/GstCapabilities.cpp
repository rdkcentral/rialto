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

#include <algorithm>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "GstCapabilities.h"
#include "GstMimeMapping.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
std::weak_ptr<IGstCapabilitiesFactory> GstCapabilitiesFactory::m_factory;

std::shared_ptr<IGstCapabilitiesFactory> IGstCapabilitiesFactory::getFactory()
{
    std::shared_ptr<IGstCapabilitiesFactory> factory = GstCapabilitiesFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstCapabilitiesFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer capabilities factory, reason: %s", e.what());
        }

        GstCapabilitiesFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstCapabilities> GstCapabilitiesFactory::createGstCapabilities()
{
    std::unique_ptr<IGstCapabilities> gstCapabilities;
    try
    {
        std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> gstWrapperFactory =
            firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
        std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper;

        if ((!gstWrapperFactory) || (!(gstWrapper = gstWrapperFactory->getGstWrapper())))
        {
            throw std::runtime_error("Cannot create GstWrapper");
        }

        std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapperFactory> glibWrapperFactory =
            firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory();
        std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper;

        if ((!glibWrapperFactory) || (!(glibWrapper = glibWrapperFactory->getGlibWrapper())))
        {
            throw std::runtime_error("Cannot create GlibWrapper");
        }
        gstCapabilities = std::make_unique<GstCapabilities>(gstWrapper, glibWrapper);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer capabilities, reason: %s", e.what());
    }

    return gstCapabilities;
}

GstCapabilities::GstCapabilities(const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                 const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    : m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}
{
    fillSupportedMimeTypes();
}

std::vector<std::string> GstCapabilities::getSupportedMimeTypes(MediaSourceType sourceType)
{
    std::vector<std::string> supportedMimeTypesSource;
    std::string type;
    if (sourceType == MediaSourceType::VIDEO)
    {
        type = "video/";
    }
    else if (sourceType == MediaSourceType::AUDIO)
    {
        type = "audio/";
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Unsupported media type");
        return {};
    }

    std::copy_if(m_supportedMimeTypes.begin(), m_supportedMimeTypes.end(), std::back_inserter(supportedMimeTypesSource),
                 [&type](const std::string &supportedMimeType) { return supportedMimeType.find(type) == 0; });

    return supportedMimeTypesSource;
}

bool GstCapabilities::isMimeTypeSupported(const std::string &mimeType)
{
    return m_supportedMimeTypes.find(mimeType) != m_supportedMimeTypes.end();
}

std::vector<std::string> GstCapabilities::getSupportedProperties(MediaSourceType mediaType,
                                                                 const std::vector<std::string> &propertyNames)
{
    // Get gstreamer element factories. The following flag settings will fetch both SINK and DECODER types
    // of gstreamer classes...
    GstElementFactoryListType factoryListType{GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER};
    {
        // If MediaSourceType::AUDIO is specified then adjust the flag so that we
        // restrict the list to gstreamer AUDIO element types (and likewise for video and subtitle)...
        static const std::unordered_map<MediaSourceType, GstElementFactoryListType>
            kLookupExtraConditions{{MediaSourceType::AUDIO, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO},
                                   {MediaSourceType::VIDEO, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO},
                                   {MediaSourceType::SUBTITLE, GST_ELEMENT_FACTORY_TYPE_MEDIA_SUBTITLE}};
        auto i = kLookupExtraConditions.find(mediaType);
        if (i != kLookupExtraConditions.end())
            factoryListType |= i->second;
    }

    GList *factories{m_gstWrapper->gstElementFactoryListGetElements(factoryListType, GST_RANK_NONE)};

    // Scan all returned elements for the specified properties...
    std::list<std::string> propertiesToLookFor{propertyNames.begin(), propertyNames.end()};
    std::vector<std::string> propertiesFound;
    for (GList *iter = factories; iter != nullptr && !propertiesToLookFor.empty(); iter = iter->next)
    {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(iter->data);
        GstElement *element = m_gstWrapper->gstElementFactoryCreate(factory, nullptr);
        if (element)
        {
            for (auto i = propertiesToLookFor.begin(); i != propertiesToLookFor.end();)
            {
                if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(element), i->c_str()))
                {
                    propertiesFound.push_back(*i);
                    i = propertiesToLookFor.erase(i);
                }
                else
                    ++i;
            }

            m_gstWrapper->gstObjectUnref(element);
        }
    }

    // Cleanup
    m_gstWrapper->gstPluginFeatureListFree(factories);
    return propertiesFound;
}

void GstCapabilities::fillSupportedMimeTypes()
{
    std::vector<GstCaps *> supportedCaps = getSupportedCapsFromDecoders();
    appendSupportedCapsFromParserDecoderChains(supportedCaps);

    if (supportedCaps.empty())
    {
        RIALTO_SERVER_LOG_WARN("There are no supported caps");
        return;
    }

    m_supportedMimeTypes = firebolt::rialto::server::convertFromCapsVectorToMimeSet(supportedCaps, m_gstWrapper);

    for (GstCaps *caps : supportedCaps)
    {
        m_gstWrapper->gstCapsUnref(caps);
    }
}

std::vector<GstCaps *> GstCapabilities::getSupportedCapsFromDecoders()
{
    std::vector<GstCaps *> supportedCaps;

    GList *decoderFactories =
        m_gstWrapper->gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    if (!decoderFactories)
    {
        RIALTO_SERVER_LOG_WARN("Could not find any decoder");
        return {};
    }

    for (GList *factoriesIter = decoderFactories; factoriesIter; factoriesIter = factoriesIter->next)
    {
        GstElementFactory *factory = static_cast<GstElementFactory *>(factoriesIter->data);
        const GList *kDecoderPadTemplates = m_gstWrapper->gstElementFactoryGetStaticPadTemplates(factory);

        addAllUniqueSinkPadsCapsToVector(supportedCaps, kDecoderPadTemplates);
    }

    m_gstWrapper->gstPluginFeatureListFree(decoderFactories);
    return supportedCaps;
}

void GstCapabilities::appendSupportedCapsFromParserDecoderChains(std::vector<GstCaps *> &supportedCaps)
{
    if (supportedCaps.empty())
    {
        return;
    }

    std::vector<GstCaps *> decoderSupportedCaps = supportedCaps;

    GList *parserFactories =
        m_gstWrapper->gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    if (!parserFactories)
    {
        RIALTO_SERVER_LOG_WARN("Could not find any parser");
        return;
    }

    for (GstCaps *decoderCaps : decoderSupportedCaps)
    {
        for (GList *factoriesIter = parserFactories; factoriesIter; factoriesIter = factoriesIter->next)
        {
            GstElementFactory *factory = static_cast<GstElementFactory *>(factoriesIter->data);
            const GList *kParserPadTemplates = m_gstWrapper->gstElementFactoryGetStaticPadTemplates(factory);

            if (canCreateParserDecoderChain(decoderCaps, kParserPadTemplates))
            {
                addAllUniqueSinkPadsCapsToVector(supportedCaps, kParserPadTemplates);
            }
        }
    }

    m_gstWrapper->gstPluginFeatureListFree(parserFactories);
}

bool GstCapabilities::canCreateParserDecoderChain(GstCaps *decoderCaps, const GList *kParserPadTemplates)
{
    for (const GList *padTemplateIter = kParserPadTemplates; padTemplateIter; padTemplateIter = padTemplateIter->next)
    {
        GstStaticPadTemplate *padTemplate = static_cast<GstStaticPadTemplate *>(padTemplateIter->data);
        if (padTemplate->direction == GST_PAD_SRC)
        {
            GstCaps *padTemplateCaps = m_gstWrapper->gstStaticCapsGet(&padTemplate->static_caps);

            // check if parser's src pad can be connected to decoder's sink pad
            bool canIntersect = m_gstWrapper->gstCapsCanIntersect(decoderCaps, padTemplateCaps);
            m_gstWrapper->gstCapsUnref(padTemplateCaps);
            if (canIntersect)
            {
                return true;
            }
        }
    }

    return false;
}

void GstCapabilities::addAllUniqueSinkPadsCapsToVector(std::vector<GstCaps *> &capsVector, const GList *padTemplates)
{
    for (const GList *padTemplateIter = padTemplates; padTemplateIter; padTemplateIter = padTemplateIter->next)
    {
        GstStaticPadTemplate *padTemplate = static_cast<GstStaticPadTemplate *>(padTemplateIter->data);
        if (padTemplate->direction == GST_PAD_SINK)
        {
            GstCaps *padTemplateCaps = m_gstWrapper->gstStaticCapsGet(&padTemplate->static_caps);
            if (!isCapsInVector(capsVector, padTemplateCaps))
            {
                capsVector.push_back(padTemplateCaps);
            }
            else
            {
                m_gstWrapper->gstCapsUnref(padTemplateCaps);
            }
        }
    }
}

bool GstCapabilities::isCapsInVector(const std::vector<GstCaps *> &capsVector, GstCaps *caps) const
{
    return std::find_if(capsVector.begin(), capsVector.end(),
                        [&](const GstCaps *comparedCaps)
                        { return m_gstWrapper->gstCapsIsStrictlyEqual(caps, comparedCaps); }) != capsVector.end();
}

} // namespace firebolt::rialto::server

// namespace firebolt::rialto::server
