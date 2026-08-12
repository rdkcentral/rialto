/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "MediaCapabilitiesFactory.h"
#include "MediaCapabilities.h"
#include "IYamlCppWrapper.h"

namespace rialto::servermanager::service
{
std::unique_ptr<IMediaCapabilities> createMediaCapabilities()
{
    auto factory = firebolt::rialto::wrappers::IYamlCppWrapperFactory::getFactory();
    if (!factory)
        return nullptr;
    auto wrapper = factory->createYamlCppWrapper();
    if (!wrapper)
        return nullptr;
    return std::make_unique<MediaCapabilities>(std::move(wrapper));
}

} // namespace rialto::servermanager::service
