/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef RIALTO_SERVERMANAGER_IPC_CAPABILITY_SERIALISER_H_
#define RIALTO_SERVERMANAGER_IPC_CAPABILITY_SERIALISER_H_

#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>
#include "servermanagermodule.pb.h"

namespace rialto::servermanager::ipc
{
// Converts C++ AudioDecoderCapabilities into the typed proto AudioCapabilities message.
<<<<<<< HEAD
void serialiseAudioCapabilities(const firebolt::rialto::AudioDecoderCapabilities &src, rialto::AudioCapabilities *dst);

// Converts C++ VideoDecoderCapabilities into the typed proto VideoCapabilities message.
void serialiseVideoCapabilities(const firebolt::rialto::VideoDecoderCapabilities &src, rialto::VideoCapabilities *dst);
=======
void serialiseAudioCapabilities(const firebolt::rialto::common::AudioDecoderCapabilities &src, rialto::AudioCapabilities *dst);

// Converts C++ VideoDecoderCapabilities into the typed proto VideoCapabilities message.
void serialiseVideoCapabilities(const firebolt::rialto::common::VideoDecoderCapabilities &src, rialto::VideoCapabilities *dst);
>>>>>>> c39f8bc1 (RDKEMW-15078: Yaml Reader in RailtoServerManager changes)

} // namespace rialto::servermanager::ipc

#endif // RIALTO_SERVERMANAGER_IPC_CAPABILITY_SERIALISER_H_
