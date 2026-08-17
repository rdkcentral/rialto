/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_
#define FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_

#include "AudioDecoderCapabilities.h"
#include "VideoDecoderCapabilities.h"
#include <memory>

namespace firebolt::rialto
{
class IMediaCapabilities;

class IMediaCapabilitiesFactory
{
public:
    IMediaCapabilitiesFactory() = default;
    virtual ~IMediaCapabilitiesFactory() = default;

    static std::shared_ptr<IMediaCapabilitiesFactory> createFactory();

    virtual std::unique_ptr<IMediaCapabilities> createMediaCapabilities() const = 0;
};

/**
 * @brief Single capabilities query interface replacing IMediaPipelineCapabilities capability methods.
 *        If the ServerManager supplied HFP YAML data at startup, it is returned directly;
 *        otherwise the session server falls back to its GStreamer element-query path.
 */
class IMediaCapabilities
{
public:
    IMediaCapabilities() = default;
    virtual ~IMediaCapabilities() = default;

    IMediaCapabilities(const IMediaCapabilities &) = delete;
    IMediaCapabilities &operator=(const IMediaCapabilities &) = delete;
    IMediaCapabilities(IMediaCapabilities &&) = delete;
    IMediaCapabilities &operator=(IMediaCapabilities &&) = delete;

    virtual firebolt::rialto::common::AudioDecoderCapabilities getSupportedAudioCapabilities() = 0;

    virtual firebolt::rialto::common::VideoDecoderCapabilities getSupportedVideoCapabilities() = 0;
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_
