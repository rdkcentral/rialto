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

#ifndef FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_PRIVATE_H_
#define FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_PRIVATE_H_

#include "IDecryptionService.h"
#include "IGstProtectionMetadataWrapper.h"
#include "IGstWrapper.h"
#include <gst/gst.h>
#include <memory>

namespace firebolt::rialto::server
{
class GstRialtoDecryptorPrivate
{
public:
    /**
     * @brief The constructor.

     * @param[in] parentElement     : The parent decryptor element.
     * @param[in] gstWrapperFactory : The gstreamer wrapper factory.
     */
    GstRialtoDecryptorPrivate(GstBaseTransform *parentElement,
                              const std::shared_ptr<IGstWrapperFactory> &gstWrapperFactory);

    /**
     * @brief Decrypts the gst buffer.
     *
     * @param[in] buffer   : The gst buffer to decrypt.
     *
     * @retval the gst flow return status.
     */
    GstFlowReturn decrypt(GstBuffer *buffer);

    /**
     * @brief Set the decryption service object.
     *
     * @param[in] decryptionService     : The service that performs decryption.
     */
    void setDecryptionService(IDecryptionService *decryptionService);

    /**
     * @brief Set the protection metadata object
     *
     * @param[in] metadataWrapper     : The protection metadata object
     */
    void setProtectionMetadataWrapper(std::unique_ptr<IGstProtectionMetadataWrapper> &&metadataWrapper);

private:
    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<IGstWrapper> m_gstWrapper;

    /**
     * @brief The parent decryptor element.
     */
    GstBaseTransform *m_decryptorElement;

    /**
     * @brief The service to perform decryption.
     */
    IDecryptionService *m_decryptionService;

    /**
     * @brief The protection metadata object
     */
    std::unique_ptr<IGstProtectionMetadataWrapper> m_metadataWrapper;
    /**
     * @brief Extract decryption data from protection meta.
     *
     * @param[in]  protectionMetaInfo   : The protection meta info to extract data from.
     * @param[out] keySessionId         : The session id for the session.
     * @param[out] subSampleCount       : count of subsamples
     * @param[out] initWithLast15       : The value deciding whether decryption context needs to be initialized with
     *                                  last 15 bytes. Currently this only applies to PlayReady DRM.
     * @param[out] key                  : Gstreamer buffer containing keyID to use for decryption
     * @param[out] iv                   : Gstreamer buffer containing initial vector (IV) used during decryption.
     * @param[out] subSample            : Gstreamer buffer containing subsamples size which has been parsed from
     * protection meta data.
     *
     * @retval the gst flow return status.
     */
    GstFlowReturn extractDecryptionData(GstStructure *protectionMetaInfo, uint32_t &keySessionId,
                                        uint32_t &subsampleCount, uint32_t &initWithLast15, GstBuffer **key,
                                        GstBuffer **iv, GstBuffer **subsamples);
};
}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_PRIVATE_H_
