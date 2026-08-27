/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_H_
#define RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_H_

#include "IMediaCapabilities.h"
#include "IYamlCppWrapper.h"
#include <memory>

namespace rialto::servermanager::service
{
class MediaCapabilities : public IMediaCapabilities
{
public:
    explicit MediaCapabilities(std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> yamlCppWrapper);
    ~MediaCapabilities() override = default;

    firebolt::rialto::DecoderCapabilitiesStatus
    getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &capabilities) override;

    firebolt::rialto::DecoderCapabilitiesStatus
    getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &capabilities) override;

private:
    std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> m_yamlCppWrapper;
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_H_
