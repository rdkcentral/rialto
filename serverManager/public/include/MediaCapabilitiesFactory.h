/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_FACTORY_H_
#define RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_FACTORY_H_

#include "IMediaCapabilities.h"
#include <memory>

namespace rialto::servermanager::service
{
std::unique_ptr<IMediaCapabilities> createMediaCapabilities();

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_MEDIA_CAPABILITIES_FACTORY_H_
