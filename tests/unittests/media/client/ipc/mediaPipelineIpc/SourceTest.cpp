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
#include "MediaPipelineProtoRequestMatchers.h"
#include "MediaPipelineProtoUtils.h"

class RialtoClientMediaPipelineIpcSourceTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_id = 456;
    const char *m_kMimeType = "video/mpeg";
    const firebolt::rialto::SegmentAlignment m_kAlignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
    const firebolt::rialto::StreamFormat m_kStreamFormat = firebolt::rialto::StreamFormat::RAW;
    const int m_kWidth = 1920;
    const int m_kHeight = 1080;
    const std::string m_kTextTrackSelection = "CC1";
    const std::shared_ptr<CodecData> m_kNullCodecData;
    const uint32_t m_kNumberOfChannels = 6;
    const uint32_t m_kSampleRate = 48000;
    const std::string m_kCodecSpecificConfigStr = "1243567";
    const std::shared_ptr<firebolt::rialto::CodecData> m_kCodecData{std::make_shared<firebolt::rialto::CodecData>()};

    virtual void SetUp()
    {
        m_kCodecData->data = std::vector<std::uint8_t>{'T', 'E', 'S', 'T'};
        m_kCodecData->type = firebolt::rialto::CodecDataType::BUFFER;

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
 * Class to test MediaSourceAV and MediaSourceSubtitle
 */
class MediaSourceTest : public IMediaPipeline::MediaSource
{
public:
    MediaSourceTest(SourceConfigType configType, int id) : IMediaPipeline::MediaSource(configType) {}
    ~MediaSourceTest() {}

    std::unique_ptr<MediaSource> copy() const override { return std::make_unique<MediaSourceTest>(*this); }
};

/**
 * Class to test MediaSourceVideoDolbyVision, MediaSourceVideo and MediaSourceAudio
 */
class MediaSourceVideoDolbyVideoAudioTest : public IMediaPipeline::MediaSourceAV
{
public:
    MediaSourceVideoDolbyVideoAudioTest(SourceConfigType configType, int id) : MediaSourceAV(configType) {}
    ~MediaSourceVideoDolbyVideoAudioTest() {}

    std::unique_ptr<MediaSource> copy() const override
    {
        return std::make_unique<MediaSourceVideoDolbyVideoAudioTest>(*this);
    }
};

/**
 * Test that attachSource can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                                           attachSourceRequestMatcherVideo(m_sessionId, m_kMimeType, true, m_kWidth,
                                                                           m_kHeight, m_kAlignment, m_kNullCodecData,
                                                                           convertStreamFormat(m_kStreamFormat)),
                                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType, true, m_kWidth, m_kHeight, m_kAlignment,
                                                           m_kStreamFormat);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that attachSource can be called successfully with no drm.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceNoDrmSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                                           attachSourceRequestMatcherVideo(m_sessionId, m_kMimeType, false, m_kWidth,
                                                                           m_kHeight, m_kAlignment, m_kNullCodecData,
                                                                           convertStreamFormat(m_kStreamFormat)),
                                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType, false, m_kWidth, m_kHeight, m_kAlignment,
                                                           m_kStreamFormat);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test attach audio source with codec specific config.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachAudioSourceWithAdditionaldataSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherAudio(m_sessionId, m_kMimeType, true, m_kAlignment,
                                                           m_kNumberOfChannels, m_kSampleRate, m_kCodecSpecificConfigStr,
                                                           m_kCodecData, convertStreamFormat(m_kStreamFormat)),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(m_kCodecSpecificConfigStr.begin(), m_kCodecSpecificConfigStr.end());
    AudioConfig audioConfig{m_kNumberOfChannels, m_kSampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType, true, audioConfig, m_kAlignment,
                                                           m_kStreamFormat, m_kCodecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test attach audio source with codec specific config.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachAudioSourceWithEmptyCodecDataSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherAudio(m_sessionId, m_kMimeType, true, m_kAlignment,
                                                           m_kNumberOfChannels, m_kSampleRate, m_kCodecSpecificConfigStr,
                                                           m_kNullCodecData, convertStreamFormat(m_kStreamFormat)),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(m_kCodecSpecificConfigStr.begin(), m_kCodecSpecificConfigStr.end());
    AudioConfig audioConfig{m_kNumberOfChannels, m_kSampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType, true, audioConfig, m_kAlignment,
                                                           m_kStreamFormat, m_kNullCodecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachDolbyVisionSourceWithSuccess)
{
    expectIpcApiCallSuccess();

    uint32_t dolbyVisionProfile = 5;
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherDolby(m_sessionId, m_kMimeType, true, m_kWidth, m_kHeight,
                                                           m_kAlignment, m_kCodecData,
                                                           convertStreamFormat(m_kStreamFormat), dolbyVisionProfile),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideoDolbyVision>(m_kMimeType, dolbyVisionProfile, true, m_kWidth,
                                                                      m_kHeight, m_kAlignment, m_kStreamFormat,
                                                                      m_kCodecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSubtitleSourceWithSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherSubtitle(m_sessionId, m_kMimeType, false, m_kTextTrackSelection),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceSubtitle>(m_kMimeType, m_kTextTrackSelection);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test switch audio source
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, SwitchAudioSourceSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcherSwitchSource(m_sessionId, m_kMimeType, true, m_kAlignment,
                                                                  m_kNumberOfChannels, m_kSampleRate,
                                                                  m_kCodecSpecificConfigStr, m_kCodecData,
                                                                  convertStreamFormat(m_kStreamFormat)),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(m_kCodecSpecificConfigStr.begin(), m_kCodecSpecificConfigStr.end());
    AudioConfig audioConfig{m_kNumberOfChannels, m_kSampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType, true, audioConfig, m_kAlignment,
                                                           m_kStreamFormat, m_kCodecData);

    EXPECT_EQ(m_mediaPipelineIpc->switchSource(mediaSource, m_id), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

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
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);
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
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that switch audio source fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, SwitchAudioSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    EXPECT_EQ(m_mediaPipelineIpc->switchSource(mediaSource, m_id), false);
}

/**
 * Test that switch audio source fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, SwitchAudioSourceChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->switchSource(mediaSource, m_id), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that switch audio source fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, SwitchSourceReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->switchSource(mediaSource, m_id), true);
}

/**
 * Test that RemoveSource can be called successfully.
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
 * Test that RemoveSource fails when ipc fails.
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

/**
 * Test that AllSourcesAttached can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AllSourcesAttachedSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("allSourcesAttached"), m_controllerMock.get(),
                           allSourcesAttachedRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->allSourcesAttached(), true);
}

/**
 * Test that AllSourcesAttached fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AllSourcesAttachedeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("allSourcesAttached"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->allSourcesAttached(), false);
}

/**
 * Test that AllSourcesAttached fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AllSourcesAttachedChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->allSourcesAttached(), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that AllSourcesAttached fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AllSourcesAttachedReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("allSourcesAttached"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->allSourcesAttached(), true);
}

/**
 * Test that attachSource fails if the media source AV is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceWithInvalidMediaSourceAV)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceTest>(SourceConfigType::VIDEO_DOLBY_VISION, m_id);

    bool result = m_mediaPipelineIpc->attachSource(source, m_id);

    EXPECT_EQ(result, false);
}

/**
 * Test that attachSource fails if the media source video dolby vision is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceWithInvalidMediaSourceVideoDolbyVision)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceVideoDolbyVideoAudioTest>(SourceConfigType::VIDEO_DOLBY_VISION, m_id);

    bool result = m_mediaPipelineIpc->attachSource(source, m_id);

    EXPECT_EQ(result, false);
}

/**
 * Test that attachSource fails if the media source video is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceWithInvalidMediaSourceVideo)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceVideoDolbyVideoAudioTest>(SourceConfigType::VIDEO, m_id);

    bool result = m_mediaPipelineIpc->attachSource(source, m_id);

    EXPECT_EQ(result, false);
}

/**
 * Test that attachSource fails if the media source audio is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceWithInvalidMediaSourceAudio)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceVideoDolbyVideoAudioTest>(SourceConfigType::AUDIO, m_id);

    bool result = m_mediaPipelineIpc->attachSource(source, m_id);

    EXPECT_EQ(result, false);
}

/**
 * Test that attachSource fails if the media source subtitle is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceWithInvalidMediaSourceSubtitle)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceTest>(SourceConfigType::SUBTITLE, m_id);

    bool result = m_mediaPipelineIpc->attachSource(source, m_id);

    EXPECT_EQ(result, false);
}

/**
 * Test that switch source fails if the media source audio is invalid.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, SwitchSourceWithInvalidMediaSourceAudio)
{
    EXPECT_CALL(*m_channelMock, isConnected()).InSequence(m_isConnectedSeq).WillOnce(Return(true)).RetiresOnSaturation();
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaSourceVideoDolbyVideoAudioTest>(SourceConfigType::AUDIO, m_id);

    bool result = m_mediaPipelineIpc->switchSource(source, m_id);

    EXPECT_EQ(result, false);
}
