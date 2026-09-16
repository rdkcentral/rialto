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

#include <stdexcept>

#include "IGstCapabilities.h"
#include "MediaCapabilitiesServerInternal.h"
#include "RialtoServerLogging.h"
#include <algorithm>
#include <mutex>

namespace firebolt::rialto::server
{
std::shared_ptr<IMediaCapabilitiesServerInternal>
MediaCapabilitiesServerInternalFactory::createMediaCapabilitiesServerInternal() const
{
    std::shared_ptr<IMediaCapabilitiesServerInternal> mediaCapabilities;

    try
    {
        // Create GstCapabilities for runtime GStreamer queries (fallback path)
        std::shared_ptr<IGstCapabilitiesFactory> gstCapabilitiesFactory = IGstCapabilitiesFactory::getFactory();
        if (!gstCapabilitiesFactory)
        {
            throw std::runtime_error("Failed to get the gstreamer capabilities factory");
        }

        auto gstCapabilitiesUnique = gstCapabilitiesFactory->createGstCapabilities();
        if (!gstCapabilitiesUnique)
        {
            throw std::runtime_error("Failed to create GstCapabilities");
        }

        // Create MediaCapabilitiesServerInternal orchestrator for GStreamer queries (fallback path)
        // Pass ownership of gstCapabilities to MediaCapabilitiesServerInternal (unique_ptr)
        // Preloaded capabilities are applied later by the caller via setPreloadedCapabilities()
        mediaCapabilities = std::make_shared<MediaCapabilitiesServerInternal>(std::move(gstCapabilitiesUnique));

        RIALTO_SERVER_LOG_DEBUG("Created server-side MediaCapabilitiesServerInternal with GStreamer orchestration");
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create server-side media capabilities, reason: %s", e.what());
    }

    return mediaCapabilities;
}

std::shared_ptr<IMediaCapabilitiesServerInternalFactory> IMediaCapabilitiesServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the server-internal media capabilities factory, reason: %s", e.what());
    }

    return factory;
}

MediaCapabilitiesServerInternal::MediaCapabilitiesServerInternal(std::unique_ptr<IGstCapabilities> gstCapabilities)
    : m_gstCapabilities{std::move(gstCapabilities)}
{
    RIALTO_SERVER_LOG_DEBUG("MediaCapabilitiesServerInternal: constructor - uses GStreamer for fallback queries");
    if (!m_gstCapabilities)
    {
        throw std::runtime_error("GstCapabilities is required");
    }
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilitiesServerInternal::getSupportedAudioCapabilities()
{
    // Path 0: Check preloaded capabilities first (highest priority)
    {
        std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
        if (m_preloadedAudioCapabilities.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Found preloaded audio capabilities, enhancing with missing codecs from GStreamer");

            // Create a copy to avoid modifying the cached version
            firebolt::rialto::common::AudioDecoderCapabilities enhanced = *m_preloadedAudioCapabilities;

            // Query GStreamer to get complete codec support
            auto gstCaps = m_gstCapabilities->getSupportedAudioCapabilities();

            // Append any missing codecs from GStreamer to the YAML capabilities
            appendMissingAudioCodecsToYaml(enhanced, gstCaps);

            RIALTO_SERVER_LOG_DEBUG("Returning enhanced audio capabilities (Path 0 with appended GStreamer codecs)");
            return enhanced;
        }
    }

    // Path B: Fall back to GStreamer queries
    RIALTO_SERVER_LOG_DEBUG("Returning GStreamer audio capabilities (Path B fallback)");
    return m_gstCapabilities->getSupportedAudioCapabilities();
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilitiesServerInternal::getSupportedVideoCapabilities()
{
    // Path 0: Check preloaded capabilities first (highest priority)
    {
        std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
        if (m_preloadedVideoCapabilities.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Found preloaded video capabilities, enhancing with missing codecs from GStreamer");

            // Create a copy to avoid modifying the cached version
            firebolt::rialto::common::VideoDecoderCapabilities enhanced = *m_preloadedVideoCapabilities;

            // Query GStreamer to get complete codec support
            auto gstCaps = m_gstCapabilities->getSupportedVideoCapabilities();

            // Append any missing codecs from GStreamer to the YAML capabilities
            appendMissingVideoCodecsToYaml(enhanced, gstCaps);

            RIALTO_SERVER_LOG_DEBUG("Returning enhanced video capabilities (Path 0 with appended GStreamer codecs)");
            return enhanced;
        }
    }

    // Path B: Fall back to GStreamer queries
    RIALTO_SERVER_LOG_DEBUG("Returning GStreamer video capabilities (Path B fallback)");
    return m_gstCapabilities->getSupportedVideoCapabilities();
}

void MediaCapabilitiesServerInternal::setPreloadedCapabilities(
    const std::optional<firebolt::rialto::common::AudioDecoderCapabilities> &audioCapabilities,
    const std::optional<firebolt::rialto::common::VideoDecoderCapabilities> &videoCapabilities)
{
    std::lock_guard<std::mutex> lock{m_preloadedCapabilitiesMutex};
    m_preloadedAudioCapabilities = audioCapabilities;
    m_preloadedVideoCapabilities = videoCapabilities;
    RIALTO_SERVER_LOG_DEBUG("Preloaded capabilities set (audio: %s, video: %s)",
                            audioCapabilities.has_value() ? "yes" : "no", videoCapabilities.has_value() ? "yes" : "no");
}

void MediaCapabilitiesServerInternal::appendMissingAudioCodecsToYaml(
    firebolt::rialto::common::AudioDecoderCapabilities &yamlCaps,
    const firebolt::rialto::common::AudioDecoderCapabilities &gstCaps)
{
    RIALTO_SERVER_LOG_DEBUG("Starting audio codec append: YAML has %u ranks, GStreamer has %u ranks",
                            static_cast<uint32_t>(yamlCaps.capabilities.size()),
                            static_cast<uint32_t>(gstCaps.capabilities.size()));

    // Append missing codecs for each existing rank
    for (size_t rankIdx = 0; rankIdx < yamlCaps.capabilities.size() && rankIdx < gstCaps.capabilities.size(); ++rankIdx)
    {
        appendMissingCodecsToAudioRank(yamlCaps.capabilities[rankIdx], gstCaps.capabilities[rankIdx]);
    }

    // If GStreamer has more ranks than YAML, append those ranks entirely
    if (gstCaps.capabilities.size() > yamlCaps.capabilities.size())
    {
        RIALTO_SERVER_LOG_DEBUG("GStreamer has %u additional ranks, appending to YAML",
                                static_cast<uint32_t>(gstCaps.capabilities.size() - yamlCaps.capabilities.size()));
        yamlCaps.capabilities.insert(yamlCaps.capabilities.end(),
                                     gstCaps.capabilities.begin() + yamlCaps.capabilities.size(),
                                     gstCaps.capabilities.end());
    }

    RIALTO_SERVER_LOG_DEBUG("Audio codec append complete, total ranks now: %u",
                            static_cast<uint32_t>(yamlCaps.capabilities.size()));
}

void MediaCapabilitiesServerInternal::appendMissingCodecsToAudioRank(
    firebolt::rialto::common::AudioDecoderCapability &yamlRank,
    const firebolt::rialto::common::AudioDecoderCapability &gstRank)
{
    // Helper lambda to merge profiles for multi-profile codecs
    auto mergeProfiles = [](auto &yamlProfiles, const auto &gstProfiles)
    {
        for (const auto &gstProfile : gstProfiles)
        {
            if (yamlProfiles.find(gstProfile.first) == yamlProfiles.end())
            {
                yamlProfiles[gstProfile.first] = gstProfile.second;
            }
        }
    };

    // PCM - single profile, no merge needed
    if (!yamlRank.pcm.has_value() && gstRank.pcm.has_value())
    {
        yamlRank.pcm = gstRank.pcm;
        RIALTO_SERVER_LOG_DEBUG("Appended missing PCM codec");
    }

    // AAC - multi-profile codec
    if (!yamlRank.aac.has_value() && gstRank.aac.has_value())
    {
        yamlRank.aac = gstRank.aac;
        RIALTO_SERVER_LOG_DEBUG("Appended missing AAC codec with %u profiles",
                                static_cast<uint32_t>(gstRank.aac->profiles.size()));
    }
    else if (yamlRank.aac.has_value() && gstRank.aac.has_value())
    {
        mergeProfiles(yamlRank.aac->profiles, gstRank.aac->profiles);
    }

    // MPEG Audio - multi-profile codec
    if (!yamlRank.mpegAudio.has_value() && gstRank.mpegAudio.has_value())
    {
        yamlRank.mpegAudio = gstRank.mpegAudio;
        RIALTO_SERVER_LOG_DEBUG("Appended missing MPEG Audio codec with %u profiles",
                                static_cast<uint32_t>(gstRank.mpegAudio->profiles.size()));
    }
    else if (yamlRank.mpegAudio.has_value() && gstRank.mpegAudio.has_value())
    {
        mergeProfiles(yamlRank.mpegAudio->profiles, gstRank.mpegAudio->profiles);
    }

    // MP3 - single profile
    if (!yamlRank.mp3.has_value() && gstRank.mp3.has_value())
    {
        yamlRank.mp3 = gstRank.mp3;
        RIALTO_SERVER_LOG_DEBUG("Appended missing MP3 codec");
    }

    // ALAC - single profile
    if (!yamlRank.alac.has_value() && gstRank.alac.has_value())
    {
        yamlRank.alac = gstRank.alac;
        RIALTO_SERVER_LOG_DEBUG("Appended missing ALAC codec");
    }

    // SBC - single profile
    if (!yamlRank.sbc.has_value() && gstRank.sbc.has_value())
    {
        yamlRank.sbc = gstRank.sbc;
        RIALTO_SERVER_LOG_DEBUG("Appended missing SBC codec");
    }

    // Dolby AC3 - multi-profile codec
    if (!yamlRank.dolbyAc3.has_value() && gstRank.dolbyAc3.has_value())
    {
        yamlRank.dolbyAc3 = gstRank.dolbyAc3;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Dolby AC3 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.dolbyAc3->profiles.size()));
    }
    else if (yamlRank.dolbyAc3.has_value() && gstRank.dolbyAc3.has_value())
    {
        mergeProfiles(yamlRank.dolbyAc3->profiles, gstRank.dolbyAc3->profiles);
    }

    // Dolby AC4 - single profile
    if (!yamlRank.dolbyAc4.has_value() && gstRank.dolbyAc4.has_value())
    {
        yamlRank.dolbyAc4 = gstRank.dolbyAc4;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Dolby AC4 codec");
    }

    // Dolby EAC3 - multi-profile codec
    if (!yamlRank.dolbyEac3.has_value() && gstRank.dolbyEac3.has_value())
    {
        yamlRank.dolbyEac3 = gstRank.dolbyEac3;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Dolby EAC3 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.dolbyEac3->profiles.size()));
    }
    else if (yamlRank.dolbyEac3.has_value() && gstRank.dolbyEac3.has_value())
    {
        mergeProfiles(yamlRank.dolbyEac3->profiles, gstRank.dolbyEac3->profiles);
    }

    // Dolby TrueHD - single profile
    if (!yamlRank.dolbyTruehd.has_value() && gstRank.dolbyTruehd.has_value())
    {
        yamlRank.dolbyTruehd = gstRank.dolbyTruehd;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Dolby TrueHD codec");
    }

    // FLAC - single profile
    if (!yamlRank.flac.has_value() && gstRank.flac.has_value())
    {
        yamlRank.flac = gstRank.flac;
        RIALTO_SERVER_LOG_DEBUG("Appended missing FLAC codec");
    }

    // Vorbis - single profile
    if (!yamlRank.vorbis.has_value() && gstRank.vorbis.has_value())
    {
        yamlRank.vorbis = gstRank.vorbis;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Vorbis codec");
    }

    // Opus - single profile
    if (!yamlRank.opus.has_value() && gstRank.opus.has_value())
    {
        yamlRank.opus = gstRank.opus;
        RIALTO_SERVER_LOG_DEBUG("Appended missing Opus codec");
    }

    // RealAudio - multi-profile codec
    if (!yamlRank.realAudio.has_value() && gstRank.realAudio.has_value())
    {
        yamlRank.realAudio = gstRank.realAudio;
        RIALTO_SERVER_LOG_DEBUG("Appended missing RealAudio codec with %u profiles",
                                static_cast<uint32_t>(gstRank.realAudio->profiles.size()));
    }
    else if (yamlRank.realAudio.has_value() && gstRank.realAudio.has_value())
    {
        mergeProfiles(yamlRank.realAudio->profiles, gstRank.realAudio->profiles);
    }

    // USAC - multi-profile codec
    if (!yamlRank.usac.has_value() && gstRank.usac.has_value())
    {
        yamlRank.usac = gstRank.usac;
        RIALTO_SERVER_LOG_DEBUG("Appended missing USAC codec with %u profiles",
                                static_cast<uint32_t>(gstRank.usac->profiles.size()));
    }
    else if (yamlRank.usac.has_value() && gstRank.usac.has_value())
    {
        mergeProfiles(yamlRank.usac->profiles, gstRank.usac->profiles);
    }

    // DTS - multi-profile codec
    if (!yamlRank.dts.has_value() && gstRank.dts.has_value())
    {
        yamlRank.dts = gstRank.dts;
        RIALTO_SERVER_LOG_DEBUG("Appended missing DTS codec with %u profiles",
                                static_cast<uint32_t>(gstRank.dts->profiles.size()));
    }
    else if (yamlRank.dts.has_value() && gstRank.dts.has_value())
    {
        mergeProfiles(yamlRank.dts->profiles, gstRank.dts->profiles);
    }

    // AVS - multi-profile codec
    if (!yamlRank.avs.has_value() && gstRank.avs.has_value())
    {
        yamlRank.avs = gstRank.avs;
        RIALTO_SERVER_LOG_DEBUG("Appended missing AVS codec with %u profiles",
                                static_cast<uint32_t>(gstRank.avs->profiles.size()));
    }
    else if (yamlRank.avs.has_value() && gstRank.avs.has_value())
    {
        mergeProfiles(yamlRank.avs->profiles, gstRank.avs->profiles);
    }
}

void MediaCapabilitiesServerInternal::appendMissingVideoCodecsToYaml(
    firebolt::rialto::common::VideoDecoderCapabilities &yamlCaps,
    const firebolt::rialto::common::VideoDecoderCapabilities &gstCaps)
{
    RIALTO_SERVER_LOG_DEBUG("Starting video codec append: YAML has %u ranks, GStreamer has %u ranks",
                            static_cast<uint32_t>(yamlCaps.capabilities.size()),
                            static_cast<uint32_t>(gstCaps.capabilities.size()));

    // Append missing codecs for each existing rank
    for (size_t rankIdx = 0; rankIdx < yamlCaps.capabilities.size() && rankIdx < gstCaps.capabilities.size(); ++rankIdx)
    {
        appendMissingCodecsToVideoRank(yamlCaps.capabilities[rankIdx], gstCaps.capabilities[rankIdx]);
    }

    // If GStreamer has more ranks than YAML, append those ranks entirely
    if (gstCaps.capabilities.size() > yamlCaps.capabilities.size())
    {
        RIALTO_SERVER_LOG_DEBUG("GStreamer has %u additional ranks, appending to YAML",
                                static_cast<uint32_t>(gstCaps.capabilities.size() - yamlCaps.capabilities.size()));
        yamlCaps.capabilities.insert(yamlCaps.capabilities.end(),
                                     gstCaps.capabilities.begin() + yamlCaps.capabilities.size(),
                                     gstCaps.capabilities.end());
    }

    RIALTO_SERVER_LOG_DEBUG("Video codec append complete, total ranks now: %u",
                            static_cast<uint32_t>(yamlCaps.capabilities.size()));
}

void MediaCapabilitiesServerInternal::appendMissingCodecsToVideoRank(
    firebolt::rialto::common::VideoDecoderCapability &yamlRank,
    const firebolt::rialto::common::VideoDecoderCapability &gstRank)
{
    // MPEG2 - has profiles and dynamic ranges
    if (!yamlRank.codecCapabilities.mpeg2.has_value() && gstRank.codecCapabilities.mpeg2.has_value())
    {
        yamlRank.codecCapabilities.mpeg2 = gstRank.codecCapabilities.mpeg2;
        RIALTO_SERVER_LOG_DEBUG("Appended missing MPEG2 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.codecCapabilities.mpeg2->profiles.size()));
    }
    else if (yamlRank.codecCapabilities.mpeg2.has_value() && gstRank.codecCapabilities.mpeg2.has_value())
    {
        // Merge profiles: keep all YAML profiles and add missing GStreamer profiles
        for (const auto &gstProfile : gstRank.codecCapabilities.mpeg2->profiles)
        {
            auto profileExists = std::any_of(yamlRank.codecCapabilities.mpeg2->profiles.begin(),
                                             yamlRank.codecCapabilities.mpeg2->profiles.end(),
                                             [&gstProfile](const auto &yamlProfile)
                                             {
                                                 return yamlProfile.type == gstProfile.type &&
                                                        yamlProfile.maxLevel == gstProfile.maxLevel &&
                                                        yamlProfile.maxBitrateInBps == gstProfile.maxBitrateInBps;
                                             });
            if (!profileExists)
            {
                yamlRank.codecCapabilities.mpeg2->profiles.push_back(gstProfile);
            }
        }
    }

    // H264 - has profiles and dynamic ranges
    if (!yamlRank.codecCapabilities.h264.has_value() && gstRank.codecCapabilities.h264.has_value())
    {
        yamlRank.codecCapabilities.h264 = gstRank.codecCapabilities.h264;
        RIALTO_SERVER_LOG_DEBUG("Appended missing H.264 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.codecCapabilities.h264->profiles.size()));
    }
    else if (yamlRank.codecCapabilities.h264.has_value() && gstRank.codecCapabilities.h264.has_value())
    {
        // Merge profiles: keep all YAML profiles and add missing GStreamer profiles
        for (const auto &gstProfile : gstRank.codecCapabilities.h264->profiles)
        {
            auto profileExists = std::any_of(yamlRank.codecCapabilities.h264->profiles.begin(),
                                             yamlRank.codecCapabilities.h264->profiles.end(),
                                             [&gstProfile](const auto &yamlProfile)
                                             {
                                                 return yamlProfile.type == gstProfile.type &&
                                                        yamlProfile.maxLevel == gstProfile.maxLevel &&
                                                        yamlProfile.maxBitrateInBps == gstProfile.maxBitrateInBps;
                                             });
            if (!profileExists)
            {
                yamlRank.codecCapabilities.h264->profiles.push_back(gstProfile);
            }
        }
    }

    // H265 - has profiles and dynamic ranges
    if (!yamlRank.codecCapabilities.h265.has_value() && gstRank.codecCapabilities.h265.has_value())
    {
        yamlRank.codecCapabilities.h265 = gstRank.codecCapabilities.h265;
        RIALTO_SERVER_LOG_DEBUG("Appended missing H.265 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.codecCapabilities.h265->profiles.size()));
    }
    else if (yamlRank.codecCapabilities.h265.has_value() && gstRank.codecCapabilities.h265.has_value())
    {
        // Merge profiles: keep all YAML profiles and add missing GStreamer profiles
        for (const auto &gstProfile : gstRank.codecCapabilities.h265->profiles)
        {
            auto profileExists = std::any_of(yamlRank.codecCapabilities.h265->profiles.begin(),
                                             yamlRank.codecCapabilities.h265->profiles.end(),
                                             [&gstProfile](const auto &yamlProfile)
                                             {
                                                 return yamlProfile.type == gstProfile.type &&
                                                        yamlProfile.maxLevel == gstProfile.maxLevel &&
                                                        yamlProfile.maxBitrateInBps == gstProfile.maxBitrateInBps;
                                             });
            if (!profileExists)
            {
                yamlRank.codecCapabilities.h265->profiles.push_back(gstProfile);
            }
        }
    }

    // VP9 - has profiles and dynamic ranges
    if (!yamlRank.codecCapabilities.vp9.has_value() && gstRank.codecCapabilities.vp9.has_value())
    {
        yamlRank.codecCapabilities.vp9 = gstRank.codecCapabilities.vp9;
        RIALTO_SERVER_LOG_DEBUG("Appended missing VP9 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.codecCapabilities.vp9->profiles.size()));
    }
    else if (yamlRank.codecCapabilities.vp9.has_value() && gstRank.codecCapabilities.vp9.has_value())
    {
        // Merge profiles: keep all YAML profiles and add missing GStreamer profiles
        for (const auto &gstProfile : gstRank.codecCapabilities.vp9->profiles)
        {
            auto profileExists = std::any_of(yamlRank.codecCapabilities.vp9->profiles.begin(),
                                             yamlRank.codecCapabilities.vp9->profiles.end(),
                                             [&gstProfile](const auto &yamlProfile)
                                             {
                                                 return yamlProfile.type == gstProfile.type &&
                                                        yamlProfile.maxLevel == gstProfile.maxLevel &&
                                                        yamlProfile.maxBitrateInBps == gstProfile.maxBitrateInBps;
                                             });
            if (!profileExists)
            {
                yamlRank.codecCapabilities.vp9->profiles.push_back(gstProfile);
            }
        }
    }

    // AV1 - has profiles and dynamic ranges
    if (!yamlRank.codecCapabilities.av1.has_value() && gstRank.codecCapabilities.av1.has_value())
    {
        yamlRank.codecCapabilities.av1 = gstRank.codecCapabilities.av1;
        RIALTO_SERVER_LOG_DEBUG("Appended missing AV1 codec with %u profiles",
                                static_cast<uint32_t>(gstRank.codecCapabilities.av1->profiles.size()));
    }
    else if (yamlRank.codecCapabilities.av1.has_value() && gstRank.codecCapabilities.av1.has_value())
    {
        // Merge profiles: keep all YAML profiles and add missing GStreamer profiles
        for (const auto &gstProfile : gstRank.codecCapabilities.av1->profiles)
        {
            auto profileExists = std::any_of(yamlRank.codecCapabilities.av1->profiles.begin(),
                                             yamlRank.codecCapabilities.av1->profiles.end(),
                                             [&gstProfile](const auto &yamlProfile)
                                             {
                                                 return yamlProfile.type == gstProfile.type &&
                                                        yamlProfile.maxLevel == gstProfile.maxLevel &&
                                                        yamlProfile.maxBitrateInBps == gstProfile.maxBitrateInBps;
                                             });
            if (!profileExists)
            {
                yamlRank.codecCapabilities.av1->profiles.push_back(gstProfile);
            }
        }
    }
}

} // namespace firebolt::rialto::server
