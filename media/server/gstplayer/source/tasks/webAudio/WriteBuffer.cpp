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

#include "tasks/webAudio/WriteBuffer.h"
#include "WebAudioPlayerContext.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"
#include <cinttypes>

namespace firebolt::rialto::server::webaudio
{
WriteBuffer::WriteBuffer(WebAudioPlayerContext &context, IGstWebAudioPlayerPrivate &player, std::shared_ptr<IGstWrapper> gstWrapper, std::shared_ptr<IGlibWrapper> glibWrapper, uint8_t *mainPtr, uint32_t mainLength, uint8_t *wrapPtr, uint32_t wrapLength)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_mainPtr{mainPtr}, m_mainLength{mainLength}, m_wrapPtr{wrapPtr}, m_wrapLength{wrapLength}
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

    uint64_t freeBytes = kMaxWebAudioBytes - m_gstWrapper->gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(m_context.source));
#if 0 //TODO(RIALTO-2) Enable writing of partial data
    uint64_t bytesToWrite = std::min(freeBytes, m_mainLength + m_wrapLength);
#else
    uint64_t bytesToWrite = m_mainLength + m_wrapLength;
#endif
    uint64_t bytesWritten = 0;

#if 0 //TODO(RIALTO-2) Enable writing of partial data
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

#else
    if (freeBytes >= m_mainLength + m_wrapLength)
    {
        GstBuffer *gstBuffer = m_gstWrapper->gstBufferNewAllocate(nullptr, bytesToWrite, nullptr);
        if (gstBuffer)
        {
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, 0, m_mainPtr, m_mainLength);
            bytesWritten += m_gstWrapper->gstBufferFill(gstBuffer, bytesWritten, m_wrapPtr, m_wrapLength);
        }

#endif

        if (bytesWritten != bytesToWrite)
        {
            RIALTO_SERVER_LOG_ERROR("Did not write the correct number of bytes! expected %" PRIu64 ", actual %" PRIu64, bytesToWrite, bytesWritten);
            m_gstWrapper->gstBufferUnref(gstBuffer);
            bytesWritten = 0;
        }
        else if (GST_FLOW_OK != m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.source), gstBuffer))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to push the buffers to the appsrc");
            m_gstWrapper->gstBufferUnref(gstBuffer);
            bytesWritten = 0;
        }
    }
    m_context.m_lastBytesWritten = bytesWritten;
}
} // namespace firebolt::rialto::server::webaudio
