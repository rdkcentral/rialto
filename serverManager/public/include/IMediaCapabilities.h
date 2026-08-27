/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_I_MEDIA_CAPABILITIES_H_
#define RIALTO_SERVERMANAGER_SERVICE_I_MEDIA_CAPABILITIES_H_

#include <AudioDecoderCapabilities.h>
#include <MediaCommon.h>
#include <VideoDecoderCapabilities.h>
#include <memory>

namespace rialto::servermanager::service
{
class IMediaCapabilities
{
public:
    IMediaCapabilities() = default;
    virtual ~IMediaCapabilities() = default;

    IMediaCapabilities(const IMediaCapabilities &) = delete;
    IMediaCapabilities &operator=(const IMediaCapabilities &) = delete;
    IMediaCapabilities(IMediaCapabilities &&) = delete;
    IMediaCapabilities &operator=(IMediaCapabilities &&) = delete;

    virtual firebolt::rialto::DecoderCapabilitiesStatus
    getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &capabilities) = 0;

    virtual firebolt::rialto::DecoderCapabilitiesStatus
    getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &capabilities) = 0;
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_I_MEDIA_CAPABILITIES_H_
