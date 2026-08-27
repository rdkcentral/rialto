/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "MediaCapabilities.h"
#include "RialtoServerManagerLogging.h"
#include <chrono>

namespace rialto::servermanager::service
{
MediaCapabilities::MediaCapabilities(std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> yamlCppWrapper)
    : m_yamlCppWrapper{std::move(yamlCppWrapper)}
{
    RIALTO_SERVER_MANAGER_LOG_ERROR(
        "USHA: MediaCapabilities: constructor body where MediaCapabilities object is created with yamlCppWrapper");
}

firebolt::rialto::DecoderCapabilitiesStatus
MediaCapabilities::getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &capabilities)
{
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: loading audio capabilities from YAML");
    RIALTO_SERVER_MANAGER_LOG_DEBUG("MediaCapabilities: loading audio capabilities from YAML");

    auto startTime = std::chrono::high_resolution_clock::now();
    const auto status = m_yamlCppWrapper->getAudioDecoderCapabilities(capabilities);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: audio YAML read completed in %lld ms",
                                    (long long)duration.count());
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: audio capabilities load status: %d",
                                    static_cast<int>(status));
    if (status == firebolt::rialto::DecoderCapabilitiesStatus::OK)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: audio capabilities loaded successfully");
    else if (status == firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: HFP YAML config not found - returning empty capabilities");
    else
        RIALTO_SERVER_MANAGER_LOG_WARN("MediaCapabilities: YAML schema validation or internal error for audio");
    return status;
}

firebolt::rialto::DecoderCapabilitiesStatus
MediaCapabilities::getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &capabilities)
{
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: loading video capabilities from YAML");
    RIALTO_SERVER_MANAGER_LOG_DEBUG("MediaCapabilities: loading video capabilities from YAML");

    auto startTime = std::chrono::high_resolution_clock::now();
    const auto status = m_yamlCppWrapper->getVideoDecoderCapabilities(capabilities);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: video YAML read completed in %lld ms",
                                    (long long)duration.count());
    RIALTO_SERVER_MANAGER_LOG_ERROR("USHA: MediaCapabilities: video capabilities load status: %d",
                                    static_cast<int>(status));
    if (status == firebolt::rialto::DecoderCapabilitiesStatus::OK)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: video capabilities loaded successfully");
    else if (status == firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: HFP YAML config not found - returning empty capabilities");
    else
        RIALTO_SERVER_MANAGER_LOG_WARN("MediaCapabilities: YAML schema validation or internal error for video");
    return status;
}

} // namespace rialto::servermanager::service
