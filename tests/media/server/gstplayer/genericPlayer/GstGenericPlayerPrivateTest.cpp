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

#include "GstGenericPlayerTestCommon.h"
#include "Matchers.h"
#include "MediaSourceUtil.h"
#include "PlayerTaskMock.h"
#include "TimerMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr std::chrono::milliseconds kPositionReportTimerMs{250};
constexpr int32_t kSampleRate{13};
constexpr int32_t kNumberOfChannels{4};
constexpr int32_t kInvalidSampleRate{0};
constexpr int32_t kInvalidNumberOfChannels{0};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
constexpr Fraction kFrameRate{15, 1};
constexpr int32_t kSourceId{2};
constexpr int64_t kTimeStamp{123};
constexpr int64_t kDuration{432};
constexpr int32_t kMediaKeySessionId{4235};
const std::vector<uint8_t> kKeyId{1, 2, 3, 4};
const std::vector<uint8_t> kInitVector{5, 6, 7, 8};
constexpr uint32_t kInitWithLast15{1};
constexpr size_t kNumClearBytes{3};
constexpr size_t kNumEncryptedBytes{5};
constexpr uint32_t kCrypt{7};
constexpr uint32_t kSkip{7};
constexpr firebolt::rialto::CipherMode kCipherMode{firebolt::rialto::CipherMode::CENS};
constexpr VideoRequirements m_videoReq{kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
const std::shared_ptr<firebolt::rialto::CodecData> kEmptyCodecData{};
const std::shared_ptr<firebolt::rialto::CodecData> kCodecDataWithBuffer{std::make_shared<firebolt::rialto::CodecData>(
    firebolt::rialto::CodecData{std::vector<std::uint8_t>{1, 2, 3, 4}, firebolt::rialto::CodecDataType::BUFFER})};
const std::string kCodecDataStr{"test"};
const std::shared_ptr<firebolt::rialto::CodecData> kCodecDataWithString{std::make_shared<firebolt::rialto::CodecData>(
    firebolt::rialto::CodecData{std::vector<std::uint8_t>{kCodecDataStr.begin(), kCodecDataStr.end()},
                                firebolt::rialto::CodecDataType::STRING})};
const std::string kAutoVideoSinkTypeName{"GstAutoVideoSink"};
const std::string kElementTypeName{"GenericSink"};
} // namespace

bool operator==(const GstRialtoProtectionData &lhs, const GstRialtoProtectionData &rhs)
{
    return lhs.keySessionId == rhs.keySessionId && lhs.subsampleCount == rhs.subsampleCount &&
           lhs.initWithLast15 == rhs.initWithLast15 && lhs.key == rhs.key && lhs.iv == rhs.iv &&
           lhs.subsamples == rhs.subsamples && lhs.cipherMode == rhs.cipherMode && lhs.crypt == rhs.crypt &&
           lhs.skip == rhs.skip;
}

class GstGenericPlayerPrivateTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayerPrivate> m_sut;

    GstElement *m_realElement;
    GstElement m_element{};
    GParamSpec m_rectangleSpec{};

    GstGenericPlayerPrivateTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                                   m_videoReq, m_gstWrapperMock, m_glibWrapperMock, m_gstSrcFactoryMock,
                                                   m_timerFactoryMock, std::move(m_taskFactory),
                                                   std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock);
        m_realElement = initRealElement();
    }

    ~GstGenericPlayerPrivateTest() override
    {
        gst_object_unref(m_realElement);
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }

    void modifyContext(const std::function<void(GenericPlayerContext &)> &fun)
    {
        std::mutex m_waitMutex;
        std::condition_variable m_waitCv;
        bool m_waitBool = false;

        // Call any method to modify GstGenericPlayer context
        GstAppSrc appSrc{};
        std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
        EXPECT_CALL(m_taskFactoryMock, createNeedData(_, &appSrc))
            .WillOnce(Invoke(
                [&](GenericPlayerContext &context, GstAppSrc *src)
                {
                    fun(context);
                    std::unique_lock<std::mutex> lock{m_waitMutex};
                    m_waitBool = true;
                    m_waitCv.notify_one();
                    return std::move(task);
                }));

        m_sut->scheduleNeedMediaData(&appSrc);

        std::unique_lock<std::mutex> lock{m_waitMutex};
        m_waitCv.wait(lock, [&] { return m_waitBool; });
    }

    GstElement *initRealElement()
    {
        gst_init(nullptr, nullptr);
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        GstElement *element = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
        return element;
    }

    GstElement *setAutoVideoSinkChild()
    {
        modifyContext([&](GenericPlayerContext &context) { context.autoVideoChildSink = &m_element; });
        return &m_element;
    }

    GenericPlayerContext *getPlayerContext()
    {
        GenericPlayerContext *saveContext;
        modifyContext([&](GenericPlayerContext &context) { saveContext = &context; });
        return saveContext;
    }
};

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleNeedData)
{
    GstAppSrc appSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createNeedData(_, &appSrc)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleNeedMediaData(&appSrc);
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleEnoughDataData)
{
    GstAppSrc appSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createEnoughData(_, &appSrc)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleEnoughData(&appSrc);
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleAudioUnderflowWithUnderflowEnabled)
{
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.isPlaying = true;
            context.audioSourceRemoved = false;
        });

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _, _, true, MediaSourceType::AUDIO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleAudioUnderflow();
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleAudioUnderflowWithUnderflowDisabledNotPlaying)
{
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.isPlaying = false;
            context.audioSourceRemoved = false;
        });

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _, _, false, MediaSourceType::AUDIO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleAudioUnderflow();
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleAudioUnderflowWithUnderflowDisabledRemoveSource)
{
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.isPlaying = true;
            context.audioSourceRemoved = true;
        });

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _, _, false, MediaSourceType::AUDIO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleAudioUnderflow();
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleVideoUnderflowWithUnderflowEnabled)
{
    modifyContext([&](GenericPlayerContext &context) { context.isPlaying = true; });

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _, _, true, MediaSourceType::VIDEO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleVideoUnderflow();
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleVideoUnderflowWithUnderflowDisabled)
{
    modifyContext([&](GenericPlayerContext &context) { context.isPlaying = false; });

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _, _, false, MediaSourceType::VIDEO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleVideoUnderflow();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotSetVideoRectangleWhenVideoSinkIsNull)
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _));
    EXPECT_FALSE(m_sut->setVideoSinkRectangle());
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotSetVideoRectangleWhenVideoSinkDoesNotHaveRectangleProperty)
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = m_realElement;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_realElement))).WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("rectangle"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_realElement));
    EXPECT_FALSE(m_sut->setVideoSinkRectangle());
}

TEST_F(GstGenericPlayerPrivateTest, shouldSetVideoRectangle)
{
    setAutoVideoSinkChild();

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = m_realElement;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_realElement))).WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("rectangle")))
        .WillOnce(Return(&m_rectangleSpec));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_realElement, CharStrMatcher("rectangle")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_realElement));
    EXPECT_TRUE(m_sut->setVideoSinkRectangle());
}

TEST_F(GstGenericPlayerPrivateTest, shouldSetVideoRectangleAutoVideoSink)
{
    GstElement *autoVideoSinkChild = setAutoVideoSinkChild();

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = m_realElement;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_realElement)))
        .WillOnce(Return(kAutoVideoSinkTypeName.c_str()));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("rectangle")))
        .WillOnce(Return(&m_rectangleSpec));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(autoVideoSinkChild, CharStrMatcher("rectangle")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_realElement));
    EXPECT_TRUE(m_sut->setVideoSinkRectangle());
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotifyNeedAudioData)
{
    modifyContext([&](GenericPlayerContext &context) { context.audioNeedData = true; });

    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(MediaSourceType::AUDIO)).WillOnce(Return(true));
    m_sut->notifyNeedMediaData(true, false);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotifyNeedVideoData)
{
    modifyContext([&](GenericPlayerContext &context) { context.videoNeedData = true; });

    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(MediaSourceType::VIDEO)).WillOnce(Return(true));
    m_sut->notifyNeedMediaData(false, true);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotNotifyNeedAudioDataWhenNotNeeded)
{
    m_sut->notifyNeedMediaData(true, false);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotNotifyNeedVideoDataWhenNotNeeded)
{
    m_sut->notifyNeedMediaData(false, true);
}

TEST_F(GstGenericPlayerPrivateTest, shouldCreateClearGstBuffer)
{
    GstBuffer buffer{};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight, kFrameRate};
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    m_sut->createBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstGenericPlayerPrivateTest, shouldCreateCENSEncryptedGstBuffer)
{
    GstBuffer buffer{}, initVectorBuffer{}, keyIdBuffer{}, subSamplesBuffer{};
    guint8 subSamplesData{0};
    auto subSamplesSize{sizeof(guint16) + sizeof(guint32)};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight, kFrameRate};
    GstMeta meta;
    mediaSegment.setEncrypted(true);
    mediaSegment.setMediaKeySessionId(kMediaKeySessionId);
    mediaSegment.setKeyId(kKeyId);
    mediaSegment.setInitVector(kInitVector);
    mediaSegment.addSubSample(kNumClearBytes, kNumEncryptedBytes);
    mediaSegment.setInitWithLast15(kInitWithLast15);
    mediaSegment.setCipherMode(kCipherMode);
    mediaSegment.setEncryptionPattern(kCrypt, kSkip);
    testing::InSequence s;
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr))
        .WillOnce(Return(&keyIdBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&keyIdBuffer, 0, _, mediaSegment.getKeyId().size()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr))
        .WillOnce(Return(&initVectorBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&initVectorBuffer, 0, _, mediaSegment.getInitVector().size()));
    EXPECT_CALL(*m_glibWrapperMock, gMalloc(subSamplesSize)).WillOnce(Return(&subSamplesData));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterInitWithData(_, &subSamplesData, subSamplesSize, FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint16Be(_, kNumClearBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint32Be(_, kNumEncryptedBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(&subSamplesData, subSamplesSize)).WillOnce(Return(&subSamplesBuffer));
    GstRialtoProtectionData data = {mediaSegment.getMediaKeySessionId(),
                                    static_cast<uint32_t>(mediaSegment.getSubSamples().size()),
                                    mediaSegment.getInitWithLast15(),
                                    &keyIdBuffer,
                                    &initVectorBuffer,
                                    &subSamplesBuffer,
                                    kCipherMode,
                                    kCrypt,
                                    kSkip,
                                    true,
                                    &m_decryptionServiceMock};
    EXPECT_CALL(*m_gstProtectionMetadataWrapperMock, addProtectionMetadata(&buffer, data)).WillOnce(Return(&meta));

    m_sut->createBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstGenericPlayerPrivateTest, shouldCreateCENCEncryptedGstBuffer)
{
    GstBuffer buffer{}, initVectorBuffer{}, keyIdBuffer{}, subSamplesBuffer{};
    guint8 subSamplesData{0};
    auto subSamplesSize{sizeof(guint16) + sizeof(guint32)};
    firebolt::rialto::CipherMode cipherMode{firebolt::rialto::CipherMode::CENC};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight, kFrameRate};
    GstMeta meta;
    mediaSegment.setEncrypted(true);
    mediaSegment.setMediaKeySessionId(kMediaKeySessionId);
    mediaSegment.setKeyId(kKeyId);
    mediaSegment.setInitVector(kInitVector);
    mediaSegment.addSubSample(kNumClearBytes, kNumEncryptedBytes);
    mediaSegment.setInitWithLast15(kInitWithLast15);
    mediaSegment.setCipherMode(cipherMode);
    testing::InSequence s;
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr))
        .WillOnce(Return(&keyIdBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&keyIdBuffer, 0, _, mediaSegment.getKeyId().size()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr))
        .WillOnce(Return(&initVectorBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&initVectorBuffer, 0, _, mediaSegment.getInitVector().size()));
    EXPECT_CALL(*m_glibWrapperMock, gMalloc(subSamplesSize)).WillOnce(Return(&subSamplesData));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterInitWithData(_, &subSamplesData, subSamplesSize, FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint16Be(_, kNumClearBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint32Be(_, kNumEncryptedBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(&subSamplesData, subSamplesSize)).WillOnce(Return(&subSamplesBuffer));
    GstRialtoProtectionData data = {mediaSegment.getMediaKeySessionId(),
                                    static_cast<uint32_t>(mediaSegment.getSubSamples().size()),
                                    mediaSegment.getInitWithLast15(),
                                    &keyIdBuffer,
                                    &initVectorBuffer,
                                    &subSamplesBuffer,
                                    cipherMode,
                                    0,
                                    0,
                                    false,
                                    &m_decryptionServiceMock};
    EXPECT_CALL(*m_gstProtectionMetadataWrapperMock, addProtectionMetadata(&buffer, data)).WillOnce(Return(&meta));

    m_sut->createBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstGenericPlayerPrivateTest, shouldFailToAddProtectionMetadata)
{
    GstBuffer buffer{}, initVectorBuffer{}, keyIdBuffer{}, subSamplesBuffer{};
    guint8 subSamplesData{0};
    auto subSamplesSize{sizeof(guint16) + sizeof(guint32)};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight, kFrameRate};
    mediaSegment.setEncrypted(true);
    mediaSegment.setMediaKeySessionId(kMediaKeySessionId);
    mediaSegment.setKeyId(kKeyId);
    mediaSegment.setInitVector(kInitVector);
    mediaSegment.addSubSample(kNumClearBytes, kNumEncryptedBytes);
    mediaSegment.setInitWithLast15(kInitWithLast15);
    mediaSegment.setCipherMode(kCipherMode);
    mediaSegment.setEncryptionPattern(kCrypt, kSkip);
    testing::InSequence s;
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr))
        .WillOnce(Return(&keyIdBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&keyIdBuffer, 0, _, mediaSegment.getKeyId().size()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr))
        .WillOnce(Return(&initVectorBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&initVectorBuffer, 0, _, mediaSegment.getInitVector().size()));
    EXPECT_CALL(*m_glibWrapperMock, gMalloc(subSamplesSize)).WillOnce(Return(&subSamplesData));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterInitWithData(_, &subSamplesData, subSamplesSize, FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint16Be(_, kNumClearBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint32Be(_, kNumEncryptedBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(&subSamplesData, subSamplesSize)).WillOnce(Return(&subSamplesBuffer));
    GstRialtoProtectionData data = {mediaSegment.getMediaKeySessionId(),
                                    static_cast<uint32_t>(mediaSegment.getSubSamples().size()),
                                    mediaSegment.getInitWithLast15(),
                                    &keyIdBuffer,
                                    &initVectorBuffer,
                                    &subSamplesBuffer,
                                    kCipherMode,
                                    kCrypt,
                                    kSkip,
                                    true,
                                    &m_decryptionServiceMock};
    EXPECT_CALL(*m_gstProtectionMetadataWrapperMock, addProtectionMetadata(&buffer, data)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&keyIdBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&initVectorBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&subSamplesBuffer));

    m_sut->createBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachAudioDataWhenBuffersAreEmpty)
{
    m_sut->attachAudioData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachAudioDataWhenItIsNotNeeded)
{
    GstBuffer buffer{};
    modifyContext([&](GenericPlayerContext &context) { context.audioBuffers.emplace_back(&buffer); });
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachAudioWhenSourceIsNotPresent)
{
    GstBuffer buffer{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.audioBuffers.emplace_back(&buffer);
            context.audioNeedData = true;
        });
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstGenericPlayerPrivateTest, shouldAttachAudioData)
{
    GstBuffer buffer{};
    GstAppSrc audioSrc{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.audioBuffers.emplace_back(&buffer);
            context.audioNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc);
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachAudioData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldCancelAudioUnderflowAndResume)
{
    GstBuffer buffer{};
    GstAppSrc audioSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    GenericPlayerContext *playerContext;
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            playerContext = &context;
            context.audioBuffers.emplace_back(&buffer);
            context.audioNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc);
            context.audioUnderflowOccured = true;
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));

    m_sut->attachAudioData();
    EXPECT_FALSE(playerContext->audioUnderflowOccured);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachVideoDataWhenBuffersAreEmpty)
{
    m_sut->attachVideoData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachVideoDataWhenItIsNotNeeded)
{
    GstBuffer buffer{};
    modifyContext([&](GenericPlayerContext &context) { context.videoBuffers.emplace_back(&buffer); });
    m_sut->attachVideoData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAttachVideoWhenSourceIsNotPresent)
{
    GstBuffer buffer{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.videoBuffers.emplace_back(&buffer);
            context.videoNeedData = true;
        });
    m_sut->attachVideoData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstGenericPlayerPrivateTest, shouldAttachVideoData)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.videoBuffers.emplace_back(&buffer);
            context.videoNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc);
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachVideoData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldCancelVideoUnderflowAndResume)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    GenericPlayerContext *playerContext;
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            playerContext = &context;
            context.videoBuffers.emplace_back(&buffer);
            context.videoNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc);
            context.videoUnderflowOccured = true;
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));

    m_sut->attachVideoData();
    EXPECT_FALSE(playerContext->videoUnderflowOccured);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotCancelVideoUnderflowWhenAudioUnderflowIsActive)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.videoBuffers.emplace_back(&buffer);
            context.videoNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc);
            context.audioUnderflowOccured = true;
            context.videoUnderflowOccured = true;
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachVideoData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldAttachAudioAndVideoData)
{
    GstBuffer audioBuffer{};
    GstBuffer videoBuffer{};
    GstAppSrc audioSrc{};
    GstAppSrc videoSrc{};
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            context.audioBuffers.emplace_back(&audioBuffer);
            context.videoBuffers.emplace_back(&videoBuffer);
            context.audioNeedData = true;
            context.videoNeedData = true;
            context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc);
            context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc);
        });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &audioBuffer));
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &videoBuffer));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(NetworkState::BUFFERED));
    m_sut->attachVideoData();
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateAudioCaps)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    GstBuffer buf;
    gpointer memory{nullptr};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*m_glibWrapperMock, gMemdup(arrayMatcher(kCodecDataWithBuffer->data), kCodecDataWithBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(memory, kCodecDataWithBuffer->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleBufferStub(&dummyCaps2, StrEq("codec_data"), GST_TYPE_BUFFER, &buf));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kSampleRate, kNumberOfChannels, kCodecDataWithBuffer);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateAudioCapsWithStringCodecData)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&dummyCaps2, StrEq("codec_data"), G_TYPE_STRING,
                                                              StrEq(kCodecDataStr.c_str())));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kSampleRate, kNumberOfChannels, kCodecDataWithString);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateAudioCapsSampleRateOnly)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kSampleRate, kInvalidNumberOfChannels, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateAudioCapsNumOfChannelsOnly)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kInvalidSampleRate, kNumberOfChannels, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateAudioCapsCodecDataOnly)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    GstBuffer buf;
    gpointer memory{nullptr};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_glibWrapperMock, gMemdup(arrayMatcher(kCodecDataWithBuffer->data), kCodecDataWithBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(memory, kCodecDataWithBuffer->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleBufferStub(&dummyCaps2, StrEq("codec_data"), GST_TYPE_BUFFER, &buf));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kInvalidSampleRate, kInvalidNumberOfChannels, kCodecDataWithBuffer);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotUpdateAudioCapsWhenValuesAreInvalid)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO].appSrc = GST_ELEMENT(&audioSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kInvalidSampleRate, kInvalidNumberOfChannels, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotUpdateAudioCapsWhenNoSrc)
{
    m_sut->updateAudioCaps(kSampleRate, kNumberOfChannels, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateVideoCaps)
{
    GstAppSrc videoSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    GstBuffer buf;
    gpointer memory{nullptr};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&videoSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleFractionStub(&dummyCaps2, CharStrMatcher("framerate"), GST_TYPE_FRACTION,
                                             kFrameRate.numerator, kFrameRate.denominator));
    EXPECT_CALL(*m_glibWrapperMock, gMemdup(arrayMatcher(kCodecDataWithBuffer->data), kCodecDataWithBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(memory, kCodecDataWithBuffer->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleBufferStub(&dummyCaps2, StrEq("codec_data"), GST_TYPE_BUFFER, &buf));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&videoSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateVideoCaps(kWidth, kHeight, kFrameRate, kCodecDataWithBuffer);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateVideoCapsWithStringCodecData)
{
    GstAppSrc videoSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&videoSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleFractionStub(&dummyCaps2, CharStrMatcher("framerate"), GST_TYPE_FRACTION,
                                             kFrameRate.numerator, kFrameRate.denominator));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&dummyCaps2, StrEq("codec_data"), G_TYPE_STRING,
                                                              StrEq(kCodecDataStr.c_str())));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&videoSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateVideoCaps(kWidth, kHeight, kFrameRate, kCodecDataWithString);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateVideoCapsWithoutCodecData)
{
    GstAppSrc videoSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&videoSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleFractionStub(&dummyCaps2, CharStrMatcher("framerate"), GST_TYPE_FRACTION,
                                             kFrameRate.numerator, kFrameRate.denominator));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&videoSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateVideoCaps(kWidth, kHeight, kFrameRate, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdateVideoCapsWithoutFrameRate)
{
    GstAppSrc videoSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    GstBuffer buf;
    gpointer memory{nullptr};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](GenericPlayerContext &context)
                  { context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO].appSrc = GST_ELEMENT(&videoSrc); });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&videoSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_glibWrapperMock, gMemdup(arrayMatcher(kCodecDataWithBuffer->data), kCodecDataWithBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(memory, kCodecDataWithBuffer->data.size())).WillOnce(Return(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleBufferStub(&dummyCaps2, StrEq("codec_data"), GST_TYPE_BUFFER, &buf));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buf));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&videoSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    Fraction frameRate{kUndefinedSize, kUndefinedSize};
    m_sut->updateVideoCaps(kWidth, kHeight, frameRate, kCodecDataWithBuffer);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotUpdateAudioVideoCapsWhenNoSrc)
{
    m_sut->updateVideoCaps(kWidth, kHeight, kFrameRate, kEmptyCodecData);
}

TEST_F(GstGenericPlayerPrivateTest, shouldFailToChangePlaybackStateWhenPipelineIsNull)
{
    GstElement *pipelineCopy; // to make generic test destructor happy :-)
    modifyContext(
        [&](GenericPlayerContext &context)
        {
            pipelineCopy = context.pipeline;
            context.pipeline = nullptr;
        });
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_sut->changePipelineState(GST_STATE_PLAYING));
    modifyContext([&](GenericPlayerContext &context) { context.pipeline = pipelineCopy; });
}

TEST_F(GstGenericPlayerPrivateTest, shouldFailToChangePlaybackStateWhenSetStateFails)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstGenericPlayerPrivateTest, shouldChangePlaybackState)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_TRUE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstGenericPlayerPrivateTest, shouldStartPositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kPositionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotStartPositionReportingTimerWhenItIsActive)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(true));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kPositionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldScheduleReportPositionWhenPositionReportingTimerIsFired)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> task2{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task2), execute());
    EXPECT_CALL(m_taskFactoryMock, createReportPosition(_)).WillOnce(Return(ByMove(std::move(task))));
    EXPECT_CALL(m_taskFactoryMock, createCheckAudioUnderflow(_, _)).WillOnce(Return(ByMove(std::move(task2))));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kPositionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Invoke(
            [&](const std::chrono::milliseconds &timeout, const std::function<void()> &callback, common::TimerType timerType)
            {
                callback();
                return std::move(timerMock);
            }));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldStopActivePositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(true));
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), cancel());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kPositionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotStopInactivePositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(false));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kPositionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotStopInactivePositionReportingTimerWhenThereIsNoTimer)
{
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstGenericPlayerPrivateTest, shouldStopWorkerThread)
{
    EXPECT_CALL(m_workerThreadMock, stop());
    m_sut->stopWorkerThread();
}

TEST_F(GstGenericPlayerPrivateTest, shouldUpdatePlaybackGroup)
{
    GstElement typefind;
    GstCaps caps;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUpdatePlaybackGroup(_, &typefind, &caps)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->updatePlaybackGroup(&typefind, &caps);
}

TEST_F(GstGenericPlayerPrivateTest, shouldAddAutoVideoSinkChildSink)
{
    GenericPlayerContext *context = getPlayerContext();

    GST_OBJECT_FLAG_SET(GST_OBJECT(m_realElement), GST_ELEMENT_FLAG_SINK);

    m_sut->addAutoVideoSinkChild(G_OBJECT(m_realElement));
    EXPECT_EQ(context->autoVideoChildSink, m_realElement);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotAddAutoVideoSinkChildIfNotASink)
{
    GenericPlayerContext *context = getPlayerContext();

    m_sut->addAutoVideoSinkChild(G_OBJECT(m_realElement));
    EXPECT_EQ(context->autoVideoChildSink, nullptr);
}

TEST_F(GstGenericPlayerPrivateTest, shouldAddAutoVideoSinkChildAndOverwrite)
{
    GenericPlayerContext *context = getPlayerContext();

    GstElement *realElement2 = initRealElement();
    GST_OBJECT_FLAG_SET(GST_OBJECT(m_realElement), GST_ELEMENT_FLAG_SINK);
    GST_OBJECT_FLAG_SET(GST_OBJECT(realElement2), GST_ELEMENT_FLAG_SINK);
    context->autoVideoChildSink = m_realElement;

    m_sut->addAutoVideoSinkChild(G_OBJECT(realElement2));
    EXPECT_EQ(context->autoVideoChildSink, realElement2);

    gst_object_unref(realElement2);
}

TEST_F(GstGenericPlayerPrivateTest, shouldRemoveAutoVideoSinkChildSink)
{
    GenericPlayerContext *context = getPlayerContext();

    GST_OBJECT_FLAG_SET(GST_OBJECT(m_realElement), GST_ELEMENT_FLAG_SINK);
    context->autoVideoChildSink = m_realElement;

    m_sut->removeAutoVideoSinkChild(G_OBJECT(m_realElement));
    EXPECT_EQ(context->autoVideoChildSink, nullptr);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotRemoveAutoVideoSinkChildIfDifferentSink)
{
    GenericPlayerContext *context = getPlayerContext();

    GstElement *realElement2 = initRealElement();
    GST_OBJECT_FLAG_SET(GST_OBJECT(m_realElement), GST_ELEMENT_FLAG_SINK);
    GST_OBJECT_FLAG_SET(GST_OBJECT(realElement2), GST_ELEMENT_FLAG_SINK);
    context->autoVideoChildSink = m_realElement;

    m_sut->removeAutoVideoSinkChild(G_OBJECT(realElement2));
    EXPECT_EQ(context->autoVideoChildSink, m_realElement);

    gst_object_unref(realElement2);
}

TEST_F(GstGenericPlayerPrivateTest, shouldNotRemoveAutoVideoSinkChildIfNotAdded)
{
    GenericPlayerContext *context = getPlayerContext();

    GST_OBJECT_FLAG_SET(GST_OBJECT(m_realElement), GST_ELEMENT_FLAG_SINK);

    m_sut->removeAutoVideoSinkChild(G_OBJECT(m_realElement));
    EXPECT_EQ(context->autoVideoChildSink, nullptr);
}
