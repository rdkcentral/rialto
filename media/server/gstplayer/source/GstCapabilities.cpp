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
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "GstCapabilities.h"
#include "GstMimeMapping.h"
#include "RialtoServerLogging.h"

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
    RIALTO_SERVER_LOG_DEBUG("GstCapabilities: CreateGstCapabilities: entry");

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

        std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapperFactory> rdkGstreamerUtilsWrapperFactory =
            firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapperFactory::getFactory();
        std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper;

        if ((!rdkGstreamerUtilsWrapperFactory) ||
            (!(rdkGstreamerUtilsWrapper = rdkGstreamerUtilsWrapperFactory->createRdkGstreamerUtilsWrapper())))
        {
            throw std::runtime_error("Cannot create RdkGstreamerUtilsWrapper");
        }

        gstCapabilities = std::make_unique<GstCapabilities>(gstWrapper, glibWrapper, rdkGstreamerUtilsWrapper,
                                                            IGstInitialiser::instance());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer capabilities, reason: %s", e.what());
    }

    return gstCapabilities;
}

GstCapabilities::GstCapabilities(
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper,
    const IGstInitialiser &gstInitialiser)
    : m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper},
      m_gstInitialiser{gstInitialiser}
{
    RIALTO_SERVER_LOG_DEBUG("GstCapabilities: constructor - GstCapabilities performs GStreamer element queries only");
    // Initialize capabilities with empty defaults - will be populated by element queries
    m_initialisationThread = std::thread(
        [this]()
        {
            std::unique_lock lock{m_initialisationMutex};

            m_gstInitialiser.waitForInitialisation();
            fillSupportedMimeTypes();
            fillSupportedCapabilities();
            m_isInitialised = true;
            m_initialisationCv.notify_all();
        });
}

GstCapabilities::~GstCapabilities()
{
    if (m_initialisationThread.joinable())
    {
        m_initialisationThread.join();
    }
}

std::vector<std::string> GstCapabilities::getSupportedMimeTypes(MediaSourceType sourceType)
{
    waitForInitialisation();

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
    else if (sourceType == MediaSourceType::SUBTITLE)
    {
        return {"text/vtt", "text/ttml", "text/cc"};
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
    waitForInitialisation();
    return m_supportedMimeTypes.find(mimeType) != m_supportedMimeTypes.end();
}

std::vector<std::string> GstCapabilities::getSupportedProperties(MediaSourceType mediaType,
                                                                 const std::vector<std::string> &propertyNames)
{
    waitForInitialisation();

    // Get gstreamer element factories. The following flag settings will fetch both SINK and DECODER types
    // of gstreamer classes...
    GstElementFactoryListType factoryListType{GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER |
                                              GST_ELEMENT_FACTORY_TYPE_PARSER};
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
    std::unordered_set<std::string> propertiesToLookFor{propertyNames.begin(), propertyNames.end()};
    std::vector<std::string> propertiesFound;
    for (GList *iter = factories; iter != nullptr && !propertiesToLookFor.empty(); iter = iter->next)
    {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(iter->data);

        // WORKAROUND: initialising element "rtkv1sink" causes that another playback's video goes black
        // we don't need to scan this element, so ignore it
        if (std::string{GST_OBJECT_NAME(GST_OBJECT(factory))} == "rtkv1sink")
        {
            RIALTO_SERVER_LOG_DEBUG("Ignoring rtkv1sink element");
            continue;
        }

        GType elementType = m_gstWrapper->gstElementFactoryGetElementType(factory);
        if (elementType == G_TYPE_INVALID)
            continue;
        gpointer elementClass = m_glibWrapper->gTypeClassRef(elementType);
        if (elementClass)
        {
            GParamSpec **props;
            guint nProps;
            props = m_glibWrapper->gObjectClassListProperties(G_OBJECT_CLASS(elementClass), &nProps);
            if (props)
            {
                for (guint j = 0; j < nProps && !propertiesToLookFor.empty(); ++j)
                {
                    std::string propName{props[j]->name};
                    auto it = propertiesToLookFor.find(propName);
                    if (it != propertiesToLookFor.end())
                    {
                        RIALTO_SERVER_LOG_DEBUG("Found property '%s'", propName.c_str());
                        propertiesFound.push_back(std::move(propName));
                        propertiesToLookFor.erase(it);
                    }
                }
                m_glibWrapper->gFree(props);
            }
            m_glibWrapper->gTypeClassUnref(elementClass);
        }
    }

    // Some sinks do not specifically support the "audio-fade" property, but the mechanism is supported through the use
    // of the rdk_gstreamer_utils library. Check for audio fade support if the property is required and we haven't found it in the sinks.
    if (propertiesToLookFor.find("audio-fade") != propertiesToLookFor.end())
    {
        bool socAudioFadeSupported = m_rdkGstreamerUtilsWrapper->isSocAudioFadeSupported();
        if (socAudioFadeSupported)
        {
            RIALTO_SERVER_LOG_DEBUG("Audio fade property is supported by the SoC");
            propertiesFound.push_back("audio-fade"); // Add "audio-fade" if supported by SoC
        }
    }
    // Cleanup
    m_gstWrapper->gstPluginFeatureListFree(factories);
    return propertiesFound;
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

void GstCapabilities::fillSupportedCapabilities()
{
    // Populate audio decoder capabilities from supported MIME types
    // Separate audio and video MIME types and create capability objects with codec-specific data
    std::vector<firebolt::rialto::common::AudioDecoderCapability> audioCapabilities;
    std::vector<firebolt::rialto::common::VideoDecoderCapability> videoCapabilities;

    for (const auto &mimeType : m_supportedMimeTypes)
    {
        if (mimeType.find("audio/") == 0)
        {
            // Create audio capability with codec-specific fields based on MIME type
            // Each capability MUST have at least one codec field populated to avoid empty serialization
            firebolt::rialto::common::AudioDecoderCapability audioCap;

            if (mimeType.find("audio/x-opus") != std::string::npos || mimeType == "audio/opus")
            {
                // Opus codec with default profile capability
                audioCap.opus = firebolt::rialto::common::OpusCapability{
                    firebolt::rialto::common::AudioProfileCapability{0, 2, 48000, 16}};
            }
            else if (mimeType.find("audio/x-flac") != std::string::npos || mimeType == "audio/flac")
            {
                // FLAC codec with default profile capability
                audioCap.flac = firebolt::rialto::common::FlacCapability{
                    firebolt::rialto::common::AudioProfileCapability{0, 8, 192000, 24}};
            }
            else if (mimeType.find("audio/aac") != std::string::npos)
            {
                // AAC codec with default profile capability
                audioCap.aac = firebolt::rialto::common::AacCapability{};
                audioCap.aac->profiles[firebolt::rialto::common::AacProfile::LC] =
                    firebolt::rialto::common::AudioProfileCapability{0, 2, 48000, 16};
            }
            else if (mimeType.find("audio/mp3") != std::string::npos)
            {
                // MP3 codec with default profile capability
                audioCap.mp3 = firebolt::rialto::common::Mp3Capability{
                    firebolt::rialto::common::AudioProfileCapability{0, 2, 48000, 16}};
            }
            else if (mimeType.find("audio/x-eac3") != std::string::npos || mimeType == "audio/eac3")
            {
                // Dolby EAC3 codec with default profile capability
                audioCap.dolbyEac3 = firebolt::rialto::common::DolbyEac3Capability{};
                audioCap.dolbyEac3->profiles[firebolt::rialto::common::DolbyEac3Profile::PLUS] =
                    firebolt::rialto::common::AudioProfileCapability{0, 6, 48000, 16};
            }
            else if (mimeType.find("audio/x-ac3") != std::string::npos || mimeType == "audio/ac3")
            {
                // Dolby AC3 codec with default profile capability
                audioCap.dolbyAc3 = firebolt::rialto::common::DolbyAc3Capability{};
                audioCap.dolbyAc3->profiles[firebolt::rialto::common::DolbyAc3Profile::STANDARD] =
                    firebolt::rialto::common::AudioProfileCapability{0, 6, 48000, 16};
            }
            else if (mimeType.find("audio/x-vorbis") != std::string::npos || mimeType == "audio/vorbis")
            {
                // Vorbis codec with default profile capability
                audioCap.vorbis = firebolt::rialto::common::VorbisCapability{
                    firebolt::rialto::common::AudioProfileCapability{0, 8, 192000, 16}};
            }
            else
            {
                // Unknown audio codec - create with PCM as default fallback
                audioCap.pcm = firebolt::rialto::common::PcmCapability{
                    firebolt::rialto::common::AudioProfileCapability{0, 2, 48000, 16}};
                RIALTO_SERVER_LOG_DEBUG("Unknown audio MIME type '%s', using PCM as fallback", mimeType.c_str());
            }

            audioCapabilities.push_back(std::move(audioCap));
        }
        else if (mimeType.find("video/") == 0)
        {
            // Create video capability with codec-specific data based on MIME type
            // Initialize with empty codec capabilities - will be populated based on MIME type
            firebolt::rialto::common::VideoDecoderCapability videoCap;

            if (mimeType.find("video/h264") != std::string::npos)
            {
                // H.264 codec with default profile
                firebolt::rialto::common::H264Profile profile;
                profile.type = firebolt::rialto::common::H264ProfileType::H264_MAIN;
                profile.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_5_2;
                profile.maxBitrateInBps = 0; // No bitrate limit
                videoCap.codecCapabilities.h264 = firebolt::rialto::common::H264CodecCapability{};
                videoCap.codecCapabilities.h264->profiles.push_back(profile);
            }
            else if (mimeType.find("video/h265") != std::string::npos || mimeType.find("video/hevc") != std::string::npos)
            {
                // H.265/HEVC codec with default profile
                firebolt::rialto::common::H265Profile profile;
                profile.type = firebolt::rialto::common::H265ProfileType::H265_MAIN;
                profile.maxLevel = firebolt::rialto::common::H265Level::H265_LEVEL_5_2;
                profile.maxBitrateInBps = 0; // No bitrate limit
                videoCap.codecCapabilities.h265 = firebolt::rialto::common::H265CodecCapability{};
                videoCap.codecCapabilities.h265->profiles.push_back(profile);
            }
            else if (mimeType.find("video/x-vp9") != std::string::npos || mimeType == "video/vp9")
            {
                // VP9 codec with default profile
                firebolt::rialto::common::Vp9Profile profile;
                profile.type = firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_0;
                profile.maxLevel = firebolt::rialto::common::Vp9Level::VP9_LEVEL_5_2;
                profile.maxBitrateInBps = 0; // No bitrate limit
                videoCap.codecCapabilities.vp9 = firebolt::rialto::common::Vp9CodecCapability{};
                videoCap.codecCapabilities.vp9->profiles.push_back(profile);
            }
            else if (mimeType.find("video/x-av1") != std::string::npos || mimeType == "video/av1")
            {
                // AV1 codec with default profile
                firebolt::rialto::common::Av1Profile profile;
                profile.type = firebolt::rialto::common::Av1ProfileType::AV1_MAIN;
                profile.maxLevel = firebolt::rialto::common::Av1Level::AV1_LEVEL_6_2;
                profile.maxBitrateInBps = 0; // No bitrate limit
                videoCap.codecCapabilities.av1 = firebolt::rialto::common::Av1CodecCapability{};
                videoCap.codecCapabilities.av1->profiles.push_back(profile);
            }
            else
            {
                // Unknown video codec - create with H.264 as default fallback
                firebolt::rialto::common::H264Profile profile;
                profile.type = firebolt::rialto::common::H264ProfileType::H264_MAIN;
                profile.maxLevel = firebolt::rialto::common::H264Level::H264_LEVEL_5_2;
                profile.maxBitrateInBps = 0;
                videoCap.codecCapabilities.h264 = firebolt::rialto::common::H264CodecCapability{};
                videoCap.codecCapabilities.h264->profiles.push_back(profile);
                RIALTO_SERVER_LOG_DEBUG("Unknown video MIME type '%s', using H.264 as fallback", mimeType.c_str());
            }

            videoCapabilities.push_back(std::move(videoCap));
        }
    }

    // Initialize audio decoder capabilities if we found audio MIME types
    if (!audioCapabilities.empty())
    {
        m_audioDecoderCapabilities.capabilities = std::move(audioCapabilities);
        RIALTO_SERVER_LOG_INFO("Populated %zu audio decoder capabilities from GStreamer",
                               m_audioDecoderCapabilities.capabilities.size());
    }

    // Initialize video decoder capabilities if we found video MIME types
    if (!videoCapabilities.empty())
    {
        m_videoDecoderCapabilities.capabilities = std::move(videoCapabilities);
        RIALTO_SERVER_LOG_INFO("Populated %zu video decoder capabilities from GStreamer",
                               m_videoDecoderCapabilities.capabilities.size());
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
    return std::find_if(capsVector.begin(), capsVector.end(), [&](const GstCaps *comparedCaps)
                        { return m_gstWrapper->gstCapsIsStrictlyEqual(caps, comparedCaps); }) != capsVector.end();
}

void GstCapabilities::waitForInitialisation()
{
    std::unique_lock lock{m_initialisationMutex};
    m_initialisationCv.wait(lock, [this]() { return m_isInitialised; });
}

bool GstCapabilities::isVideoMaster(bool &isVideoMaster)
{
    waitForInitialisation();

    GstRegistry *reg = m_gstWrapper->gstRegistryGet();
    if (!reg)
    {
        RIALTO_SERVER_LOG_ERROR("Failed get the gst registry");
        return false;
    }
    GstPluginFeature *feature{nullptr};
    isVideoMaster = true;
    if (nullptr != (feature = m_gstWrapper->gstRegistryLookupFeature(reg, "amlhalasink")))
    {
        isVideoMaster = false;
        m_gstWrapper->gstObjectUnref(feature);
    }
    return true;
}

firebolt::rialto::common::AudioDecoderCapabilities GstCapabilities::getSupportedAudioCapabilities()
{
    waitForInitialisation();
    return m_audioDecoderCapabilities;
}

firebolt::rialto::common::VideoDecoderCapabilities GstCapabilities::getSupportedVideoCapabilities()
{
    waitForInitialisation();
    return m_videoDecoderCapabilities;
}

} // namespace firebolt::rialto::server
