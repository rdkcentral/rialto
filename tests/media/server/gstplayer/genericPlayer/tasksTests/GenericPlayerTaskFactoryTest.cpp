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

#include "tasks/generic/GenericPlayerTaskFactory.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include "tasks/IPlayerTask.h"
#include "tasks/generic/AttachSamples.h"
#include "tasks/generic/AttachSource.h"
#include "tasks/generic/CheckAudioUnderflow.h"
#include "tasks/generic/DeepElementAdded.h"
#include "tasks/generic/EnoughData.h"
#include "tasks/generic/Eos.h"
#include "tasks/generic/FinishSetupSource.h"
#include "tasks/generic/HandleBusMessage.h"
#include "tasks/generic/NeedData.h"
#include "tasks/generic/Pause.h"
#include "tasks/generic/Play.h"
#include "tasks/generic/ReadShmDataAndAttachSamples.h"
#include "tasks/generic/RemoveSource.h"
#include "tasks/generic/ReportPosition.h"
#include "tasks/generic/SetPlaybackRate.h"
#include "tasks/generic/SetPosition.h"
#include "tasks/generic/SetVideoGeometry.h"
#include "tasks/generic/SetVolume.h"
#include "tasks/generic/SetupElement.h"
#include "tasks/generic/SetupSource.h"
#include "tasks/generic/Shutdown.h"
#include "tasks/generic/Stop.h"
#include "tasks/generic/Underflow.h"
#include "tasks/generic/UpdatePlaybackGroup.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

class GenericPlayerTaskFactoryTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock> m_rdkGstreamerUtilsWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock>>()};
    firebolt::rialto::server::GenericPlayerTaskFactory m_sut{&m_gstPlayerClient, m_gstWrapper, m_glibWrapper,
                                                             m_rdkGstreamerUtilsWrapper};
};

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateAttachSamples)
{
    auto task = m_sut.createAttachSamples(m_context, m_gstPlayer, {});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::AttachSamples &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateAttachSource)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(-1, "video/mpeg");
    auto task = m_sut.createAttachSource(m_context, m_gstPlayer, source);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::AttachSource &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateDeepElementAdded)
{
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(nullptr));
    auto task = m_sut.createDeepElementAdded(m_context, m_gstPlayer, nullptr, nullptr, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::DeepElementAdded &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateEnoughData)
{
    auto task = m_sut.createEnoughData(m_context, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::EnoughData &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateEos)
{
    auto task = m_sut.createEos(m_context, m_gstPlayer, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Eos &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateFinishSetupSource)
{
    auto task = m_sut.createFinishSetupSource(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::FinishSetupSource &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateHandleBusMessage)
{
    auto task = m_sut.createHandleBusMessage(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::HandleBusMessage &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateNeedData)
{
    auto task = m_sut.createNeedData(m_context, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::NeedData &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreatePause)
{
    auto task = m_sut.createPause(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Pause &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreatePlay)
{
    auto task = m_sut.createPlay(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Play &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateReadShmDataAndAttachSamples)
{
    auto task = m_sut.createReadShmDataAndAttachSamples(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::ReadShmDataAndAttachSamples &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateRemoveSource)
{
    auto task = m_sut.createRemoveSource(m_context, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::RemoveSource &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateReportPosition)
{
    auto task = m_sut.createReportPosition(m_context);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::ReportPosition &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateCheckAudioUnderflow)
{
    auto task = m_sut.createCheckAudioUnderflow(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::CheckAudioUnderflow &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetPosition)
{
    auto task = m_sut.createSetPosition(m_context, m_gstPlayer, 0);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetPosition &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetupElement)
{
    auto task = m_sut.createSetupElement(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetupElement &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetupSource)
{
    auto task = m_sut.createSetupSource(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetupSource &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetVideoGeometry)
{
    auto task = m_sut.createSetVideoGeometry(m_context, m_gstPlayer, firebolt::rialto::server::Rectangle{});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetVideoGeometry &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetVolume)
{
    constexpr double kVolume{0.7};
    auto task = m_sut.createSetVolume(m_context, kVolume);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetVolume &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateShutdown)
{
    auto task = m_sut.createShutdown(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Shutdown &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateStop)
{
    auto task = m_sut.createStop(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Stop &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateUnderflow)
{
    bool flag{false};
    bool enabled{false};
    auto task = m_sut.createUnderflow(m_gstPlayer, flag, enabled);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::Underflow &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateSetPlaybackRate)
{
    auto task = m_sut.createSetPlaybackRate(m_context, 1.25);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::SetPlaybackRate &>(*task));
}

TEST_F(GenericPlayerTaskFactoryTest, ShouldCreateUpdatePlaybackGroup)
{
    auto task = m_sut.createUpdatePlaybackGroup(m_context, nullptr, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::generic::UpdatePlaybackGroup &>(*task));
}
