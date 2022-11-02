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

#ifndef FIREBOLT_RIALTO_SERVER_I_MEDIA_KEYS_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_I_MEDIA_KEYS_SERVER_INTERNAL_H_

/**
 * @file IMediaKeysServerInternal.h
 *
 * The definition of the IMediaKeys interface.
 *
 * This interface defines the server internal API of Rialto for EME decryption of AV content.
 */

#include "IMediaKeys.h"
#include "MediaCommon.h"
#include <cstdint>
#include <gst/gst.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class IMediaKeysServerInternal;
/**
 * @brief IMediaKeysServerInternal factory class, returns a concrete implementation of IMediaKeysServerInternal
 */
class IMediaKeysServerInternalFactory : public IMediaKeysFactory
{
public:
    IMediaKeysServerInternalFactory() = default;
    ~IMediaKeysServerInternalFactory() override = default;

    /**
     * @brief Create a IMediaKeysServerInternalFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaKeysServerInternalFactory> createFactory();

    /**
     * @brief IMediaKeysServerInternal factory method, returns a concrete implementation of IMediaKeysServerInternal
     *
     * @param[in] keySystem : The key system for which to create a Media Keys instance
     *
     * @retval the new media keys instance or null on error.
     */
    virtual std::unique_ptr<IMediaKeysServerInternal> createMediaKeysServerInternal(const std::string &keySystem) const = 0;
};

/**
 * @brief The definition of the IMediaKeysServerInternal interface.
 *
 * This interface defines the public API of Rialto for EME decryption of AV content
 * which should be implemented by Rialto Server.
 */
class IMediaKeysServerInternal : public IMediaKeys
{
public:
    /**
     * @brief Decrypts the buffer.
     *
     * @param[in] keySessionId    : The session id for the session.
     * @param[in]  encrypted      : Gstreamer buffer containing encrypted data and related meta data. If applicable,
     *                              decrypted data will be stored here after this call returns.
     * @param[in]  subSample      : Gstreamer buffer containing subsamples size which has been parsed from protection
     *                              meta data.
     * @param[in]  subSampleCount : count of subsamples
     * @param[in]  IV             : Gstreamer buffer containing initial vector (IV) used during decryption.
     * @param[in]  keyId          : Gstreamer buffer containing keyID to use for decryption
     * @param[in]  initWithLast15 : The value deciding whether decryption context needs to be initialized with
     *                              last 15 bytes. Currently this only applies to PlayReady DRM.
     *
     * @retval an error status.
     */
    virtual MediaKeyErrorStatus decrypt(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                        const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                        uint32_t initWithLast15) = 0;

    /**
     * @brief Checks if session with given id is handled by this MediaKeys instance
     *
     * @param[in] keySessionId    : The session id for the session.
     *
     * @retval true if session is handled by this MediaKeys instance
     */
    virtual bool hasSession(int32_t keySessionId) const = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_MEDIA_KEYS_SERVER_INTERNAL_H_
