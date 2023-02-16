/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "tasks/webAudio/WriteBuffer.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"
#include "WebAudioPlayerContext.h"
#include <cinttypes>

namespace firebolt::rialto::server::tasks::webaudio
{
WriteBuffer::WriteBuffer(WebAudioPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper, uint8_t *mainPtr,
                         uint32_t mainLength, uint8_t *wrapPtr, uint32_t wrapLength)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_mainPtr{mainPtr}, m_mainLength{mainLength}, m_wrapPtr{wrapPtr},
      m_wrapLength{wrapLength}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing WriteBuffer");
}

WriteBuffer::~WriteBuffer()
{
    RIALTO_SERVER_LOG_DEBUG("WriteBuffer finished");
}

void WriteBuffer::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing WriteBuffer");

    // Should flush the pipeline if Eos has been previously set
    if (m_context.m_shouldFlush)
    {
        if ((!m_gstWrapper->gstElementSendEvent(m_context.pipeline, m_gstWrapper->gstEventNewFlushStart())) ||
            (!m_gstWrapper->gstElementSendEvent(m_context.pipeline, m_gstWrapper->gstEventNewFlushStop(TRUE))))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to flush the pipeline");
        }
        m_context.m_shouldFlush = false;
    }

    uint64_t freeBytes = kMaxWebAudioBytes - m_gstWrapper->gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(m_context.source));
    uint64_t bytesToWrite = std::min(freeBytes, m_mainLength + m_wrapLength);
    uint64_t bytesWritten = 0;

    GstBuffer *gstBuffer = m_gstWrapper->gstBufferNewAllocate(nullptr, bytesToWrite, nullptr);
    if (gstBuffer)
    {
        if (bytesToWrite == m_mainLength + m_wrapLength)
        {
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, 0, m_mainPtr, m_mainLength);
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, bytesWritten, m_wrapPtr, m_wrapLength);
        }
        else if (bytesToWrite > m_mainLength)
        {
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, 0, m_mainPtr, m_mainLength);
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, bytesWritten, m_wrapPtr, bytesToWrite - bytesWritten);
        }
        else
        {
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, 0, m_mainPtr, bytesToWrite);
        }

        if (bytesWritten != bytesToWrite)
        {
            RIALTO_SERVER_LOG_WARN("Did not write the correct number of bytes! expected %" PRIu64 ", actual %" PRIu64,
                                   bytesToWrite, bytesWritten);
        }

        if (GST_FLOW_OK != m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.source), gstBuffer))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to push the buffers to the appsrc");
            m_gstWrapper->gstBufferUnref(gstBuffer);
            bytesWritten = 0;
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gst buffer");
    }

    {
        std::unique_lock<std::mutex> lock(m_context.m_writeBufferMutex);
        m_context.m_lastBytesWritten = bytesWritten;
    }
    m_context.m_writeBufferCond.notify_one();
}
} // namespace firebolt::rialto::server::tasks::webaudio
