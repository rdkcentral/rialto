/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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

#ifndef FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_PRIVATE_H_
#define FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_PRIVATE_H_

#include "GstDecryptorPrivate.h"
#include <gst/gst.h>
#include <memory>
#include <vector>

namespace firebolt::rialto::server
{
class GstRialtoBatchDecryptorPrivate : public GstRialtoDecryptorPrivate
{
public:
    /**
     * @brief The maximum number of buffers decrypted in one batch.
     */
    static constexpr size_t kMaxBatchSize{32};

    /**
     * @brief The constructor.
     *
     * @param[in] parentElement         : The parent decryptor element.
     * @param[in] gstWrapperFactory     : The gstreamer wrapper factory.
     * @param[in] glibWrapperFactory    : The glib wrapper factory.
     */
    GstRialtoBatchDecryptorPrivate(
        GstElement *parentElement,
        const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> &gstWrapperFactory,
        const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapperFactory> &glibWrapperFactory);

    /**
     * @brief The destructor. Releases the caps.
     */
    ~GstRialtoBatchDecryptorPrivate() override;

    /**
     * @brief Sets the source pad, that decrypted buffers are pushed to.
     *
     * @param[in] srcPad : The source pad of the decryptor element.
     */
    void setSrcPad(GstPad *srcPad);

    /**
     * @brief Handles a buffer list received on the sink pad. Takes ownership of the list.
     *
     * @param[in] bufferList : The received buffer list.
     *
     * @retval the gst flow return status.
     */
    GstFlowReturn chainList(GstBufferList *bufferList);

    /**
     * @brief Handles an event received on the sink pad. Takes ownership of the event.
     *
     * @param[in] pad    : The sink pad.
     * @param[in] parent : The decryptor element.
     * @param[in] event  : The received event.
     *
     * @retval TRUE if the event was handled.
     */
    gboolean sinkEvent(GstPad *pad, GstObject *parent, GstEvent *event);

    /**
     * @brief Decrypts the gst buffers.
     *
     * @param[in] bufferVec : The gst buffers to decrypt.
     * @param[in] caps      : The gst caps of the buffers.
     *
     * @retval the gst flow return status.
     */
    virtual GstFlowReturn decryptMulti(const std::vector<GstBuffer *> &bufferVec, GstCaps *caps);

private:
    /**
     * @brief Decrypts the batch of buffers and pushes them to the source pad. Takes ownership of the buffers.
     *
     * @param[in] buffers : The buffers to decrypt, at most kMaxBatchSize.
     *
     * @retval the gst flow return status.
     */
    GstFlowReturn processBuffers(const std::vector<GstBuffer *> &buffers);

    /**
     * @brief The source pad of the decryptor element.
     */
    GstPad *m_srcPad{nullptr};

    /**
     * @brief The caps received on the sink pad.
     */
    GstCaps *m_sinkCaps{nullptr};
};
}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_PRIVATE_H_
