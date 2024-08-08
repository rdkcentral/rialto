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

#include "MediaPipelineProxy.h"
#include "MediaPipelineTestBase.h"

MATCHER(NotNull, "")
{
    return (nullptr != arg.lock());
}

MATCHER_P(VideoRequirementsMatcher, expectedVidReq, "")
{
    return ((arg.maxWidth == expectedVidReq.maxWidth) && (arg.maxHeight == expectedVidReq.maxHeight));
}

MATCHER_P(controlClientMatcher, cc, "")
{
    const std::weak_ptr<firebolt::rialto::IControlClient> a{arg};
    return a.lock() == cc;
}

class RialtoClientCreateMediaPipelineTest : public MediaPipelineTestBase
{
protected:
    VideoRequirements m_videoReq = {};

    virtual void SetUp() { MediaPipelineTestBase::SetUp(); }

    virtual void TearDown() { MediaPipelineTestBase::TearDown(); }
};

/**
 * Test that a MediaPipeline object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaPipelineTest, Create)
{
    std::unique_ptr<IMediaPipeline> mediaPipeline;
    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();

    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(mediaPipelineIpcMock))));

    EXPECT_NO_THROW(mediaPipeline = std::make_unique<MediaPipeline>(m_mediaPipelineClientMock, m_videoReq,
                                                                    m_mediaPipelineIpcFactoryMock,
                                                                    m_mediaFrameWriterFactoryMock,
                                                                    *m_clientControllerMock));
    EXPECT_NE(mediaPipeline, nullptr);
}

/**
 * Test that a MediaPipeline proxy object can be used successfully
 */
TEST_F(RialtoClientCreateMediaPipelineTest, CreateMediaPipelineProxy)
{
    std::shared_ptr<MediaPipeline> mediaPipeline;
    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();

    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(mediaPipelineIpcMock))));

    EXPECT_NO_THROW(mediaPipeline = std::make_unique<MediaPipeline>(m_mediaPipelineClientMock, m_videoReq,
                                                                    m_mediaPipelineIpcFactoryMock,
                                                                    m_mediaFrameWriterFactoryMock,
                                                                    *m_clientControllerMock));
    EXPECT_NE(mediaPipeline, nullptr);

    EXPECT_CALL(*m_clientControllerMock, registerClient(controlClientMatcher(mediaPipeline), _)).WillOnce(Return(true));
    EXPECT_CALL(*m_clientControllerMock, unregisterClient(controlClientMatcher(mediaPipeline))).WillOnce(Return(true));
    std::unique_ptr<IMediaPipeline> proxy;
    EXPECT_NO_THROW(proxy = std::make_unique<MediaPipelineProxy>(mediaPipeline, *m_clientControllerMock));
    EXPECT_NE(proxy, nullptr);
}

/**
 * Test the factory
 */
TEST_F(RialtoClientCreateMediaPipelineTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::MediaPipelineFactory> factory =
        std::dynamic_pointer_cast<firebolt::rialto::MediaPipelineFactory>(
            firebolt::rialto::IMediaPipelineFactory::createFactory());
    EXPECT_NE(factory, nullptr);

    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();
    EXPECT_CALL(*m_clientControllerMock, registerClient(NotNull(), _)).WillOnce(Return(true));
    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(mediaPipelineIpcMock))));

    std::unique_ptr<IMediaPipeline> mediaPipeline;
    EXPECT_NO_THROW(mediaPipeline = factory->createMediaPipeline(m_mediaPipelineClientMock, m_videoReq,
                                                                 m_mediaPipelineIpcFactoryMock, m_clientControllerMock));
    EXPECT_NE(mediaPipeline, nullptr);

    // Unregister client on destroy
    EXPECT_CALL(*m_clientControllerMock, unregisterClient(NotNull())).WillOnce(Return(true));
}

/**
 * Test factory returns a nullptr if the creation of the object fails.
 * Caused by failure in createMediaPipelineIpc
 */
TEST_F(RialtoClientCreateMediaPipelineTest, FactoryFailsToCreateObjectDueToPipelineIpcFail)
{
    std::shared_ptr<firebolt::rialto::MediaPipelineFactory> factory =
        std::dynamic_pointer_cast<firebolt::rialto::MediaPipelineFactory>(
            firebolt::rialto::IMediaPipelineFactory::createFactory());
    EXPECT_NE(factory, nullptr);

    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(std::unique_ptr<IMediaPipelineIpc>{}));

    std::unique_ptr<IMediaPipeline> mediaPipeline;
    EXPECT_NO_THROW(mediaPipeline = factory->createMediaPipeline(m_mediaPipelineClientMock, m_videoReq,
                                                                 m_mediaPipelineIpcFactoryMock, m_clientControllerMock));
    EXPECT_EQ(mediaPipeline, nullptr);
}

/**
 * Test factory returns a nullptr if the creation of the object fails.
 * Caused by failure in registerClient
 */
TEST_F(RialtoClientCreateMediaPipelineTest, FactoryFailsToCreateObjectDueToRegistrationFail)
{
    std::shared_ptr<firebolt::rialto::MediaPipelineFactory> factory =
        std::dynamic_pointer_cast<firebolt::rialto::MediaPipelineFactory>(
            firebolt::rialto::IMediaPipelineFactory::createFactory());
    EXPECT_NE(factory, nullptr);

    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();
    EXPECT_CALL(*m_clientControllerMock, registerClient(NotNull(), _)).WillOnce(Return(false));
    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(mediaPipelineIpcMock))));

    std::unique_ptr<IMediaPipeline> mediaPipeline;
    EXPECT_NO_THROW(mediaPipeline = factory->createMediaPipeline(m_mediaPipelineClientMock, m_videoReq,
                                                                 m_mediaPipelineIpcFactoryMock, m_clientControllerMock));
    EXPECT_EQ(mediaPipeline, nullptr);
}

/**
 * Test that a MediaPipeline proxy object throws an exeption if failure occurs during construction.
 * In this case, MediaPipeline proxy fails to register a client with the ClientController.
 */
TEST_F(RialtoClientCreateMediaPipelineTest, RegisterClientProxyFailure)
{
    std::shared_ptr<MediaPipeline> mediaPipeline;
    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock =
        std::make_unique<StrictMock<MediaPipelineIpcMock>>();

    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(mediaPipelineIpcMock))));
    EXPECT_CALL(*m_clientControllerMock, registerClient(NotNull(), _)).WillOnce(Return(false));

    EXPECT_NO_THROW(mediaPipeline = std::make_unique<MediaPipeline>(m_mediaPipelineClientMock, m_videoReq,
                                                                    m_mediaPipelineIpcFactoryMock,
                                                                    m_mediaFrameWriterFactoryMock,
                                                                    *m_clientControllerMock));
    EXPECT_NE(mediaPipeline, nullptr);

    std::unique_ptr<IMediaPipeline> proxy;
    EXPECT_THROW(proxy = std::make_unique<MediaPipelineProxy>(mediaPipeline, *m_clientControllerMock),
                 std::runtime_error);
}

/**
 * Test that a MediaPipeline object throws an exeption if failure occurs during construction.
 * In this case, createMediaPipelineIpc fails, returning a nullptr.
 */
TEST_F(RialtoClientCreateMediaPipelineTest, CreateMediaPipelineIpcFailure)
{
    std::unique_ptr<IMediaPipeline> mediaPipeline;

    EXPECT_CALL(*m_mediaPipelineIpcFactoryMock, createMediaPipelineIpc(_, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(nullptr)));

    EXPECT_THROW(mediaPipeline = std::make_unique<MediaPipeline>(m_mediaPipelineClientMock, m_videoReq,
                                                                 m_mediaPipelineIpcFactoryMock,
                                                                 m_mediaFrameWriterFactoryMock, *m_clientControllerMock),
                 std::runtime_error);
}
