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

#include <stdexcept>

#include "GstCapabilities.h"
#include "GstMimeMapping.h"
#include "RialtoServerLogging.h"
#include <algorithm>
#include <unordered_set>

namespace
{
const char *toString(const GstElementFactoryListType &listType)
{
    switch (listType)
    {
    case GST_ELEMENT_FACTORY_TYPE_ANY:
        return "Any";
    case GST_ELEMENT_FACTORY_TYPE_AUDIOVIDEO_SINKS:
        return "AudioVideo Sinks";
    case GST_ELEMENT_FACTORY_TYPE_AUDIO_ENCODER:
        return "Audio Encoder";
    case GST_ELEMENT_FACTORY_TYPE_DECODABLE:
        return "Decodable";
    case GST_ELEMENT_FACTORY_TYPE_DECODER:
        return "Decoder";
    case GST_ELEMENT_FACTORY_TYPE_DECRYPTOR:
        return "Decryptor";
    case GST_ELEMENT_FACTORY_TYPE_DEMUXER:
        return "Demuxer";
    case GST_ELEMENT_FACTORY_TYPE_DEPAYLOADER:
        return "Depayloader";
    case GST_ELEMENT_FACTORY_TYPE_ENCODER:
        return "Encoder";
    case GST_ELEMENT_FACTORY_TYPE_ENCRYPTOR:
        return "Encryptor";
    case GST_ELEMENT_FACTORY_TYPE_FORMATTER:
        return "Formatter";
    case GST_ELEMENT_FACTORY_TYPE_HARDWARE:
        return "Hardware";
    case GST_ELEMENT_FACTORY_TYPE_MAX_ELEMENTS:
        return "Max Elements";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_ANY:
        return "Media Any";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO:
        return "Media Audio";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE:
        return "Media Image";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_METADATA:
        return "Media Metadata";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_SUBTITLE:
        return "Media Subtitle";
    case GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO:
        return "Media Video";
    case GST_ELEMENT_FACTORY_TYPE_MUXER:
        return "Muxer";
    case GST_ELEMENT_FACTORY_TYPE_PARSER:
        return "Parser";
    case GST_ELEMENT_FACTORY_TYPE_PAYLOADER:
        return "Payloader";
    case GST_ELEMENT_FACTORY_TYPE_SINK:
        return "Sink";
    case GST_ELEMENT_FACTORY_TYPE_SRC:
        return "Source";
    case GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER:
        return "Video Encoder";
    default:
        return "Unknown";
    }
}
} // namespace

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

        gstCapabilities = std::make_unique<GstCapabilities>(gstWrapper);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer capabilities, reason: %s", e.what());
    }

    return gstCapabilities;
}

GstCapabilities::GstCapabilities(const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper)
    : m_gstWrapper{gstWrapper}
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

void GstCapabilities::fillSupportedMimeTypes()
{
    std::vector<GstCaps *> supportedCaps;
    appendSupportedCapsFromFactoryType(GST_ELEMENT_FACTORY_TYPE_DECODER, supportedCaps);

    // Only append caps from decoder parser if they can link with the decoder
    appendLinkableCapsFromParserDecoderChains(supportedCaps);

    // Sink caps do not require decoder support
    appendSupportedCapsFromFactoryType(GST_ELEMENT_FACTORY_TYPE_SINK, supportedCaps);

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

void GstCapabilities::appendLinkableCapsFromParserDecoderChains(std::vector<GstCaps *> &supportedCaps)
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

void GstCapabilities::appendSupportedCapsFromFactoryType(const GstElementFactoryListType &type,
                                                         std::vector<GstCaps *> &supportedCaps)
{
    GList *factories = m_gstWrapper->gstElementFactoryListGetElements(type, GST_RANK_MARGINAL);
    if (!factories)
    {
        RIALTO_SERVER_LOG_WARN("Could not find any %s", toString(type));
        return;
    }

    for (GList *factoriesIter = factories; factoriesIter; factoriesIter = factoriesIter->next)
    {
        GstElementFactory *factory = static_cast<GstElementFactory *>(factoriesIter->data);
        const GList *kPadTemplates = m_gstWrapper->gstElementFactoryGetStaticPadTemplates(factory);

        addAllUniqueSinkPadsCapsToVector(supportedCaps, kPadTemplates);
    }

    m_gstWrapper->gstPluginFeatureListFree(factories);
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
