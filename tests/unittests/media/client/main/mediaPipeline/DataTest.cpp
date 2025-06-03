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

#include "KeyIdMap.h"
#include "MediaFrameWriterMock.h"
#include "MediaPipelineTestBase.h"
#include "SharedMemoryHandleMock.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

MATCHER_P(ShmInfoMatcher, shmInfo, "")
{
    return ((arg->maxMetadataBytes == shmInfo->maxMetadataBytes) && (arg->metadataOffset == shmInfo->metadataOffset) &&
            (arg->mediaDataOffset == shmInfo->mediaDataOffset) && (arg->maxMediaBytes == shmInfo->maxMediaBytes));
}

class RialtoClientMediaPipelineDataTest : public MediaPipelineTestBase
{
protected:
    StrictMock<MediaFrameWriterMock> *m_mediaFrameWriterMock = nullptr;

    int32_t m_sourceId = 1;
    size_t m_frameCount = 2;
    uint32_t m_requestId = 4U;
    uint32_t m_numFrames = 5U;
    int32_t m_mksId{6};
    std::vector<uint8_t> m_keyId{1, 2, 3, 4};
    uint8_t m_shmBuffer;
    std::shared_ptr<MediaPlayerShmInfo> m_shmInfo;
    const bool m_kResetTime{true};
    std::shared_ptr<SharedMemoryHandleMock> m_sharedMemoryHandleMock;
    MediaSourceStatus m_status = MediaSourceStatus::NO_AVAILABLE_SAMPLES;
    IMediaPipeline::MediaSegmentVector m_dataVec;
    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock;

    std::thread m_haveDataThread;
    std::mutex m_haveDataMutex;
    std::condition_variable m_haveDataCond;

    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();

        // InitShm
        m_shmInfo = std::make_shared<MediaPlayerShmInfo>();
        m_shmInfo->maxMetadataBytes = 5;
        m_shmInfo->metadataOffset = 6;
        m_shmInfo->mediaDataOffset = 7;
        m_shmInfo->maxMediaBytes = 3U;

        m_sharedMemoryHandleMock = std::make_shared<SharedMemoryHandleMock>();

        attachSource(m_sourceId);

        // Set the MediaPipeline state to playing by default
        setPlaybackState(PlaybackState::PLAYING);
    }

    virtual void TearDown()
    {
        deleteFrames();

        m_mediaFrameWriterMock = nullptr;

        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }

    void attachSource(std::int32_t sourceId)
    {
        std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
            std::make_unique<IMediaPipeline::MediaSourceAudio>("audio/mp4");
        EXPECT_CALL(*m_mediaPipelineIpcMock, attachSource(Ref(mediaSource), _))
            .WillOnce(DoAll(SetArgReferee<1>(sourceId), Return(true)));
        EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    }

    void needDataGeneric() { needData(m_sourceId, m_frameCount, m_requestId, m_shmInfo); }

    void addFrames(MediaSourceType sourceType, uint32_t numberOfFrames, const std::vector<uint8_t> &data)
    {
        int64_t timestamp = 0;

        mediaFrameWriterMock = std::make_unique<StrictMock<MediaFrameWriterMock>>();
        m_mediaFrameWriterMock = mediaFrameWriterMock.get();

        EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
        EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle())
            .Times(numberOfFrames)
            .WillRepeatedly(Return(m_sharedMemoryHandleMock))
            .RetiresOnSaturation();

        if (sourceType == MediaSourceType::VIDEO)
        {
            EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
                .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))))
                .RetiresOnSaturation();
        }
        else if (sourceType == MediaSourceType::AUDIO)
        {
            EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
                .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))))
                .RetiresOnSaturation();
        }

        EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).Times(numberOfFrames).WillRepeatedly(Return(AddSegmentStatus::OK));

        for (uint32_t i = 0; i < numberOfFrames; i++)
        {
            std::unique_ptr<IMediaPipeline::MediaSegment> frame =
                createFrame(sourceType, data.size(), data.data(), timestamp);

            EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);
        }
    }

    void deleteFrames()
    {
        for (auto it = m_dataVec.begin(); it != m_dataVec.end(); it++)
        {
            delete[] (*it)->getData();
        }
        m_dataVec.clear();
    }

    void haveDataSuccess(MediaSourceType sourceType)
    {
        EXPECT_CALL(*m_mediaFrameWriterMock, getNumFrames()).WillOnce(Return(m_numFrames));
        EXPECT_CALL(*m_mediaPipelineIpcMock, haveData(m_status, m_numFrames, m_requestId))
            .WillOnce(Return(true))
            .RetiresOnSaturation();

        EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
    }

    std::unique_ptr<IMediaPipeline::MediaSegment> createFrame(MediaSourceType sourceType, uint32_t dataLength,
                                                              const uint8_t *data, int64_t timestamp = 0)
    {
        std::unique_ptr<IMediaPipeline::MediaSegment> frame;
        int64_t duration = 1000000000;

        if (sourceType == MediaSourceType::AUDIO)
        {
            int32_t sampleRate = 6;
            int32_t numberOfChannels = 7;
            uint64_t clippingStart = 1024;
            uint64_t clippingEnd = 2048;
            frame = std::make_unique<IMediaPipeline::MediaSegmentAudio>(m_sourceId, timestamp, duration, sampleRate,
                                                                        numberOfChannels, clippingStart, clippingEnd);
        }
        else if (sourceType == MediaSourceType::VIDEO)
        {
            int32_t width = 8;
            int32_t height = 9;
            Fraction frameRate = {10, 1};
            frame = std::make_unique<IMediaPipeline::MediaSegmentVideo>(m_sourceId, timestamp, duration, width, height,
                                                                        frameRate);
        }
        else
        {
            frame = std::make_unique<IMediaPipeline::MediaSegment>(m_sourceId, sourceType, timestamp, duration);
        }

        frame->setData(dataLength, data);
        return frame;
    }

    void startAddSegmentThread()
    {
        // Call haveData on a separate thread.
        m_haveDataThread = std::thread(
            [this]()
            {
                mediaFrameWriterMock = std::make_unique<StrictMock<MediaFrameWriterMock>>();

                std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
                std::unique_ptr<IMediaPipeline::MediaSegment> frame =
                    createFrame(MediaSourceType::VIDEO, data.size(), data.data());
                // Save a raw pointer to the unique object for use when testing mocks
                // Object shall be freed by the holder of the unique ptr on destruction
                m_mediaFrameWriterMock = mediaFrameWriterMock.get();

                EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
                EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

                EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
                    .WillOnce(DoAll(Invoke(
                                        [this](uint8_t *shmBuffer, const std::shared_ptr<MediaPlayerShmInfo> &shminfo)
                                        {
                                            m_haveDataCond.notify_all();

                                            // Sleep for 0.1 sec so that notifyBufferTerm can be called
                                            usleep(10000);
                                        }),
                                    Return(ByMove(std::move(mediaFrameWriterMock)))));

                // haveData should still write all the frames if the MediaPipeline has been notified of termination
                EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(Ref(frame))).WillOnce(Return(AddSegmentStatus::OK));
                EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);
            });
    }
};

/**
 * Test that a need data notification is forwarded to the client on the event thread.
 * shmPosition is stored and invalid shmPosition is passed to the client.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEvent)
{
    // Playing
    needDataGeneric();
}

/**
 * Test that a need data notification is forwarded to the client on the event thread in the BUFFERING state.
 * shmPosition is stored and invalid shmPosition is passed to the client.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventBuffering)
{
    setNetworkState(NetworkState::BUFFERING);
    needDataGeneric();
}

/**
 * Test that an audio need data notification is not forwarded to the client, when audio source switch is ongoing
 */
TEST_F(RialtoClientMediaPipelineDataTest, AudioNeedDataEventDuringAudioSourceSwitch)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, removeSource(m_sourceId)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->removeSource(m_sourceId), true);
    // Should not trigger any expect calls when audio source is removed
    m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, m_shmInfo);
}

/**
 * Test that a video need data notification is forwarded to the client, when audio source switch is ongoing
 */
TEST_F(RialtoClientMediaPipelineDataTest, VideoNeedDataEventDuringAudioSourceSwitch)
{
    attachSource(m_sourceId + 1);
    EXPECT_CALL(*m_mediaPipelineIpcMock, removeSource(m_sourceId)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->removeSource(m_sourceId), true);
    needData(m_sourceId + 1, m_frameCount, m_requestId, m_shmInfo);
}

/**
 * Test that an audio need data notification is forwarded to the client, when audio source switch is finished
 */
TEST_F(RialtoClientMediaPipelineDataTest, AudioNeedDataEventAfterFinishOfAudioSourceSwitch)
{
    // Remove audio source
    EXPECT_CALL(*m_mediaPipelineIpcMock, removeSource(m_sourceId)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->removeSource(m_sourceId), true);

    // Reattach audio source
    attachSource(m_sourceId);

    // Need data should be forwarded to the client.
    needDataGeneric();
}

/**
 * Test that a need data notification is forwarded to the client on the event thread in the SEEK_DONE state.
 * shmPosition is stored and invalid shmPosition is passed to the client.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventSeekDone)
{
    setPlaybackState(PlaybackState::SEEK_DONE);
    needDataGeneric();
}

/**
 * Test that a need data notification in an invalid state is ignored.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventInvalidStates)
{
    std::vector<PlaybackState> invalidStates = {PlaybackState::SEEKING, PlaybackState::STOPPED,
                                                PlaybackState::END_OF_STREAM, PlaybackState::FAILURE};

    for (auto it = invalidStates.begin(); it != invalidStates.end(); it++)
    {
        setPlaybackState(*it);

        // Should not trigger any expect calls in all invalid states
        m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, m_shmInfo);
    }
}

/**
 * Test that a need data notification for flushing source is ignored.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventFlushingSource)
{
    // Flush source
    bool isAsync{false};
    EXPECT_CALL(*m_mediaPipelineIpcMock, flush(m_sourceId, m_kResetTime, _)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->flush(m_sourceId, m_kResetTime, isAsync), true);

    // Should not trigger any expect calls in all invalid states
    m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, m_shmInfo);
}

/**
 * Test that a need data notification is handled when flush is finished
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventAfterFlushingSource)
{
    // Flush source
    bool isAsync{false};
    EXPECT_CALL(*m_mediaPipelineIpcMock, flush(m_sourceId, m_kResetTime, _)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->flush(m_sourceId, m_kResetTime, isAsync), true);

    // Finish flush
    EXPECT_CALL(*m_mediaPipelineClientMock, notifySourceFlushed(m_sourceId));
    m_mediaPipelineCallback->notifySourceFlushed(m_sourceId);

    // Need data should be handled here.
    needDataGeneric();
}

/**
 * Test that a need data notification is handled when flush is finished in EOS state
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventAfterFlushingSourceInEos)
{
    setPlaybackState(PlaybackState::END_OF_STREAM);

    // Flush source
    bool isAsync{false};
    EXPECT_CALL(*m_mediaPipelineIpcMock, flush(m_sourceId, m_kResetTime, _)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->flush(m_sourceId, m_kResetTime, isAsync), true);

    // Finish flush
    EXPECT_CALL(*m_mediaPipelineClientMock, notifySourceFlushed(m_sourceId));
    m_mediaPipelineCallback->notifySourceFlushed(m_sourceId);

    // Need data should be handled here.
    needDataGeneric();
}

/**
 * Test that a have data call without a paired need data notification returns success.
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataNoNeedData)
{
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

TEST_F(RialtoClientMediaPipelineDataTest, HaveDataNoSegments)
{
    needDataGeneric();

    EXPECT_CALL(*m_mediaPipelineIpcMock, haveData(m_status, 0, m_requestId)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

// /**
//  * Test that a have data call succeeds when segments are added
//  */
TEST_F(RialtoClientMediaPipelineDataTest, haveDataSuccess)
{
    uint32_t numFrames = 1;
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::VIDEO, data.size(), data.data());

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::OK));
    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);

    EXPECT_CALL(*m_mediaFrameWriterMock, getNumFrames()).WillOnce(Return(numFrames));

    EXPECT_CALL(*m_mediaPipelineIpcMock, haveData(m_status, numFrames, m_requestId)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

// /**
//  * Test that an add segment call with a null segment returns error.
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentNullSegment)
{
    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, nullptr), AddSegmentStatus::ERROR);
}

// /**
//  * Test that an add segment call with an unknown data type returns error.
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentUnknownDataType)
{
    needDataGeneric();
    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::UNKNOWN, data.size(), data.data());

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::ERROR);
}

// /**
//  * Test that an add segment call with no data type returns error.
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentNoData)
{
    needDataGeneric();
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::UNKNOWN, 0, nullptr);

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::ERROR);
}

/**
 * Test that an add segment call with getting shared memory failure returns false
 */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentGetSharedMemoryFailure)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::UNKNOWN, data.size(), data.data());

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::ERROR);
}

/**
 * Test that an add segment call with getting missing shared memory handle returns false
 */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentGetSharedMemoryHandleFailure)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::UNKNOWN, data.size(), data.data());

    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(nullptr));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::ERROR);
}

// /**
//  * Test that have data fails if failed to create the frame writer.
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentcreateFrameWriterFailure)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::AUDIO, data.size(), data.data());

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock;

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::ERROR);
}

// /**
//  * Test that add segment fails if writing the frame to the buffer fails due to no space
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentWriteFrameFailure)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::VIDEO, data.size(), data.data());

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::NO_SPACE));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::NO_SPACE);
}

// /**
//  * Test that a an add segment call returns success if writing frame succeeds
//  */
TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentVideoSuccess)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::VIDEO, data.size(), data.data());

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::OK));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);
}

TEST_F(RialtoClientMediaPipelineDataTest, AddSegmentAudioSuccess)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::AUDIO, data.size(), data.data());

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::OK));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);
}

/**
 * Test that a an add segment call with encrypted frame does not update keyId when KeyId is not found in map
 */
TEST_F(RialtoClientMediaPipelineDataTest, AddEncryptedCobaltSegmentSuccess)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::VIDEO, data.size(), data.data());
    frame->setEncrypted(true);
    frame->setMediaKeySessionId(m_mksId);

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::OK));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);

    // For non-netflix segment, key ID should not be updated
    EXPECT_TRUE(frame->isEncrypted());
    EXPECT_TRUE(frame->getKeyId().empty());
}

/**
 * Test that a an add segment call with encrypted frame updates keyId when KeyId is found in map
 */
TEST_F(RialtoClientMediaPipelineDataTest, AddEncryptedNetflixSegmentSuccess)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    KeyIdMap::instance().addSession(m_mksId);
    KeyIdMap::instance().updateKey(m_mksId, m_keyId);
    std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::VIDEO, data.size(), data.data());
    frame->setEncrypted(true);
    frame->setMediaKeySessionId(m_mksId);

    std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
        std::make_unique<StrictMock<MediaFrameWriterMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_mediaFrameWriterMock = mediaFrameWriterMock.get();

    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(&m_shmBuffer));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Return(m_sharedMemoryHandleMock));

    EXPECT_CALL(*m_mediaFrameWriterFactoryMock, createFrameWriter(&m_shmBuffer, ShmInfoMatcher(m_shmInfo)))
        .WillOnce(Return(ByMove(std::move(mediaFrameWriterMock))));

    EXPECT_CALL(*m_mediaFrameWriterMock, writeFrame(_)).WillOnce(Return(AddSegmentStatus::OK));

    EXPECT_EQ(m_mediaPipeline->addSegment(m_requestId, frame), AddSegmentStatus::OK);

    // For non-netflix segment, key ID should not be updated
    EXPECT_TRUE(frame->isEncrypted());
    EXPECT_EQ(frame->getKeyId(), m_keyId);

    // Cleanup
    KeyIdMap::instance().erase(m_mksId);
}

/**
 * Test that a have data writes data successfully in the BUFFERING state.
 */
// TEST_F(RialtoClientMediaPipelineDataTest, haveDataSuccessBuffering)
// {
//     needDataGeneric();

//     std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
//     std::unique_ptr<IMediaPipeline::MediaSegment> frame = createFrame(MediaSourceType::AUDIO, data.size(), data.data());

//     std::unique_ptr<StrictMock<MediaFrameWriterMock>> mediaFrameWriterMock =
//         std::make_unique<StrictMock<MediaFrameWriterMock>>();

//     // Save a raw pointer to the unique object for use when testing mocks
//     // Object shall be freed by the holder of the unique ptr on destruction
//     m_mediaFrameWriterMock = mediaFrameWriterMock.get();

// <<<<<<< HEAD
//     setNetworkState(NetworkState::BUFFERING);

//     haveDataSuccess(MediaSourceType::AUDIO);
// }

/**
 * Test that a have data writes data successfully in the BUFFERING state.
 */
TEST_F(RialtoClientMediaPipelineDataTest, haveDataSuccessBuffering)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    addFrames(MediaSourceType::AUDIO, m_numFrames, data);

    setNetworkState(NetworkState::BUFFERING);

    haveDataSuccess(MediaSourceType::AUDIO);
}

/**
 * Test that a have data is ignored and the needDataRequest discarded in the SEEKING state.
 * haveData should return success as this is a valid scenario.
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataSeeking)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    addFrames(MediaSourceType::AUDIO, m_numFrames, data);

    setPlaybackState(PlaybackState::SEEKING);

    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);

    // Check that the needDataRequest has been discarded by initiating another haveData
    setPlaybackState(PlaybackState::PLAYING);

    // haveData should return success but will not be forwarded to media ipc
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

/**
 * Test that a have data is ignored and the needDataRequest discarded in the SEEKING state.
 * haveData should return success when Seek is completed in SEEK_DONE state
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataAfterSeekCompleted)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    addFrames(MediaSourceType::AUDIO, m_numFrames, data);

    setPlaybackState(PlaybackState::SEEKING);

    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);

    // Check that new haveData is handled after changing state to SEEK_DONE
    setPlaybackState(PlaybackState::SEEK_DONE);

    // haveData should return success but will not be forwarded to media ipc
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

/**
 * Test that a have data is ignored and the needDataRequest discarded when source is flushing.
 * haveData should return success as this is a valid scenario.
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataFlushing)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    addFrames(MediaSourceType::AUDIO, m_numFrames, data);

    bool isAsync{false};
    EXPECT_CALL(*m_mediaPipelineIpcMock, flush(m_sourceId, m_kResetTime, _)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->flush(m_sourceId, m_kResetTime, isAsync), true);

    // haveData should return success but will not be forwarded to media ipc
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

/**
 * Test that a have data is handled when flush is finished.
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataFlushCompleted)
{
    needDataGeneric();

    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    addFrames(MediaSourceType::AUDIO, m_numFrames, data);

    bool isAsync{false};
    EXPECT_CALL(*m_mediaPipelineIpcMock, flush(m_sourceId, m_kResetTime, _)).WillOnce(Return(true));
    EXPECT_EQ(m_mediaPipeline->flush(m_sourceId, m_kResetTime, isAsync), true);

    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);

    EXPECT_CALL(*m_mediaPipelineClientMock, notifySourceFlushed(m_sourceId));
    m_mediaPipelineCallback->notifySourceFlushed(m_sourceId);

    // haveData should return success but will not be forwarded to media ipc
    EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
}

/**
 * Test that a have data fails and the needDataRequest discarded in the invalid states.
 */
TEST_F(RialtoClientMediaPipelineDataTest, HaveDataInvalidStates)
{
    std::vector<PlaybackState> invalidStates = {PlaybackState::STOPPED, PlaybackState::END_OF_STREAM,
                                                PlaybackState::FAILURE};

    for (auto it = invalidStates.begin(); it != invalidStates.end(); it++)
    {
        needDataGeneric();
        std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
        addFrames(MediaSourceType::AUDIO, m_numFrames, data);

        setPlaybackState(*it);

        EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), false);

        // Should not trigger any expect calls in all invalid states
        m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, m_shmInfo);

        // Check that the needDataRequest has been discarded by initiating another haveData
        setPlaybackState(PlaybackState::PLAYING);

        // haveData should return success but will not be forwarded to media ipc
        EXPECT_EQ(m_mediaPipeline->haveData(m_status, m_requestId), true);
    }
}

/**
 * Test a notification buffer terminating waits for the frame writing to finish before returning.
 * This test validates that there are no deadlocks.
 */
TEST_F(RialtoClientMediaPipelineDataTest, TermSharedMemoryDuringWrite)
{
    needDataGeneric();

    // The thread will call have data and signal when it is processing the frames
    startAddSegmentThread();

    // Wait for frame processing to start
    std::unique_lock<std::mutex> haveDataLock(m_haveDataMutex);
    m_haveDataCond.wait(haveDataLock);

    // When notifyBufferTerm called, it shall wait for haveData to finish before returning
    m_mediaPipeline->notifyApplicationState(ApplicationState::INACTIVE);

    m_haveDataThread.join();
}

/**
 * Test that a need data notification in app state != RUNNING is ignored.
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventInactiveState)
{
    m_mediaPipeline->notifyApplicationState(ApplicationState::INACTIVE);

    // Should not trigger any expect calls in all invalid states
    m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, m_shmInfo);
}

/**
 * Test that a need data notification is skipped for uknown source
 */
TEST_F(RialtoClientMediaPipelineDataTest, NeedDataEventSkipForUnknownSource)
{
    // Should not trigger any expect calls when source is not present
    m_mediaPipelineCallback->notifyNeedMediaData(m_sourceId + 1, m_frameCount, m_requestId, m_shmInfo);
}
