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

#include "MediaPipelineIpcTestBase.h"

MATCHER_P8(attachSourceRequestMatcher2, sessionId, mimeType, numberOfChannels, sampleRate, codecSpecificConfig,
           alignment, streamFormat, codecData, "")
{
    const ::firebolt::rialto::AttachSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    firebolt::rialto::CodecData codecDataFromReq{};
    if (request->has_codec_data())
    {
        codecDataFromReq = firebolt::rialto::CodecData{{request->codec_data().begin(), request->codec_data().end()}};
    }
    return ((request->session_id() == sessionId) &&
            (static_cast<const unsigned int>(request->config_type()) ==
             static_cast<const unsigned int>(SourceConfigType::AUDIO)) &&
            (request->mime_type() == mimeType) && (request->has_audio_config()) &&
            (request->audio_config().number_of_channels() == numberOfChannels) &&
            (request->audio_config().sample_rate() == sampleRate) &&
            (request->audio_config().codec_specific_config() == codecSpecificConfig) &&
            (request->segment_alignment() == alignment) && (request->stream_format() == streamFormat) &&
            (codecDataFromReq == codecData));
}

MATCHER_P6(attachSourceRequestMatcherDolby, sessionId, mimeType, dolbyVisionProfile, alignment, streamFormat, codecData,
           "")
{
    const ::firebolt::rialto::AttachSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    firebolt::rialto::CodecData codecDataFromReq{};
    if (request->has_codec_data())
    {
        codecDataFromReq = firebolt::rialto::CodecData{{request->codec_data().begin(), request->codec_data().end()}};
    }
    return ((request->session_id() == sessionId) &&
            (static_cast<const unsigned int>(request->config_type()) ==
             static_cast<const unsigned int>(SourceConfigType::VIDEO_DOLBY_VISION)) &&
            (request->mime_type() == mimeType) && (request->segment_alignment() == alignment) &&
            (request->stream_format() == streamFormat) && (request->has_dolby_vision_profile()) &&
            (request->dolby_vision_profile() == dolbyVisionProfile) && (codecDataFromReq == codecData));
}

MATCHER_P3(attachSourceRequestMatcher, sessionId, configType, mimeType, "")
{
    const ::firebolt::rialto::AttachSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    return ((request->session_id() == sessionId) &&
            (static_cast<const unsigned int>(request->config_type()) == configType) &&
            (request->mime_type() == mimeType));
}

MATCHER_P2(removeSourceRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::RemoveSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::RemoveSourceRequest *>(arg);
    return ((request->session_id() == sessionId) && (request->source_id() == sourceId));
}

class RialtoClientMediaPipelineIpcSourceTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kMimeType = "video/mpeg";

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }

public:
    void setAttachSourceResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::AttachSourceResponse *attachSourceResponse =
            dynamic_cast<firebolt::rialto::AttachSourceResponse *>(response);
        attachSourceResponse->set_source_id(m_id);
    }
};

/**
 * Test that Load can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcher(m_sessionId, static_cast<uint32_t>(SourceConfigType::AUDIO),
                                                      m_kMimeType),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test attach audio source with codec specific config.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachAudioSourceWithAdditionaldataSuccess)
{
    expectIpcApiCallSuccess();

    uint32_t numberOfChannels = 6;
    uint32_t sampleRate = 48000;
    std::string codecSpecificConfigStr("1243567");
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
    firebolt::rialto::CodecData codecData{{'T', 'E', 'S', 'T'}};
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::RAW;
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcher2(m_sessionId, m_kMimeType, numberOfChannels, sampleRate,
                                                       codecSpecificConfigStr,
                                                       firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED,
                                                       firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW,
                                                       codecData),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
    AudioConfig audioConfig{6, 48000, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType, audioConfig, alignment, streamFormat,
                                                           codecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test attach audio source with codec specific config.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachAudioSourceWithEmptyCodecDataSuccess)
{
    expectIpcApiCallSuccess();

    uint32_t numberOfChannels = 6;
    uint32_t sampleRate = 48000;
    std::string codecSpecificConfigStr("1243567");
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
    firebolt::rialto::CodecData codecData{{}}; // Codec data present, but empty vector
    EXPECT_TRUE(codecData);
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::RAW;
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcher2(m_sessionId, m_kMimeType, numberOfChannels, sampleRate,
                                                       codecSpecificConfigStr,
                                                       firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED,
                                                       firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW,
                                                       codecData),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
    AudioConfig audioConfig{6, 48000, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType, audioConfig, alignment, streamFormat,
                                                           codecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachDolbyVisionSourceWithSuccess)
{
    expectIpcApiCallSuccess();

    uint32_t dolbyVisionProfile = 5;
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
    firebolt::rialto::CodecData codecData{{'T', 'E', 'S', 'T'}};
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::RAW;
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherDolby(m_sessionId, m_kMimeType, dolbyVisionProfile,
                                                           firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED,
                                                           firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW,
                                                           codecData),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideoDolbyVision>(m_id, m_kMimeType, dolbyVisionProfile, alignment,
                                                                      streamFormat, codecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), false);
}

/**
 * Test that AttachSource fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that AttachSource fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_id, m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that Load can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("removeSource"), m_controllerMock.get(),
                           removeSourceRequestMatcher(m_sessionId, m_id), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeSource"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), false);
}

/**
 * Test that RemoveSource fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that RemoveSource fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeSource"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), true);
}
