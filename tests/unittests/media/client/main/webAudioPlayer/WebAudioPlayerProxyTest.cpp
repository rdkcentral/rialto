/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "WebAudioPlayerProxy.h"
#include "WebAudioPlayerAndControlClientMock.h"
#include "WebAudioPlayerTestBase.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::DoAll;
using ::testing::DoubleEq;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientWebAudioPlayerProxyTest : public WebAudioPlayerTestBase
{
public:
};

/**
 * Test that the proxy passes all methods through to the underlying object
 */
TEST_F(RialtoClientWebAudioPlayerProxyTest, TestPassthrough)
{
    EXPECT_CALL(*m_clientControllerMock, registerClient(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(ApplicationState::RUNNING), Return(true)));
    EXPECT_CALL(*m_clientControllerMock, unregisterClient(_)).WillOnce(Return(true));

    auto webAudioPlayerMock = std::make_shared<StrictMock<WebAudioPlayerAndControlClientMock>>();

    EXPECT_CALL(*webAudioPlayerMock, notifyApplicationState(ApplicationState::RUNNING));

    std::shared_ptr<WebAudioPlayerProxy> proxy;
    EXPECT_NO_THROW(proxy = std::make_shared<WebAudioPlayerProxy>(webAudioPlayerMock, *m_clientControllerMock));

    const uint32_t kPreferredFrames{3};
    const uint32_t kMaximumFrames{4};
    const uint32_t kDelayFrames{5};
    const uint32_t kAvailableFrames{6};
    const bool kSupportDeferredPlay{true};
    const double kTestLevel1{0.1};
    const double kTestLevel2{0.2};

    /////////////////////////////////////////////

    EXPECT_CALL(*webAudioPlayerMock, play()).WillOnce(Return(true));
    EXPECT_CALL(*webAudioPlayerMock, pause()).WillOnce(Return(true));
    EXPECT_CALL(*webAudioPlayerMock, setEos()).WillOnce(Return(true));
    EXPECT_CALL(*webAudioPlayerMock, getBufferAvailable(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(kAvailableFrames), Return(true)));
    EXPECT_CALL(*webAudioPlayerMock, setVolume(DoubleEq(kTestLevel1))).WillOnce(Return(true));
    EXPECT_CALL(*webAudioPlayerMock, getVolume(_)).WillOnce(DoAll(SetArgReferee<0>(kTestLevel2), Return(true)));
    EXPECT_CALL(*webAudioPlayerMock, getBufferDelay(_)).WillOnce(DoAll(SetArgReferee<0>(kDelayFrames), Return(true)));
    EXPECT_CALL(*webAudioPlayerMock, getDeviceInfo(_, _, _))
        .WillOnce(DoAll(SetArgReferee<0>(kPreferredFrames), SetArgReferee<1>(kMaximumFrames),
                        SetArgReferee<2>(kSupportDeferredPlay), Return(true)));
    EXPECT_CALL(*webAudioPlayerMock, writeBuffer(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*webAudioPlayerMock, getClient());

    /////////////////////////////////////////////

    EXPECT_TRUE(proxy->play());
    EXPECT_TRUE(proxy->pause());
    EXPECT_TRUE(proxy->setEos());
    {
        uint32_t availableFrames;
        std::shared_ptr<WebAudioShmInfo> webAudioShmInfo;
        EXPECT_TRUE(proxy->getBufferAvailable(availableFrames, webAudioShmInfo));
        EXPECT_EQ(availableFrames, kAvailableFrames);
    }
    EXPECT_TRUE(proxy->setVolume(kTestLevel1));
    {
        double retVolume;
        EXPECT_TRUE(proxy->getVolume(retVolume));
        EXPECT_EQ(retVolume, kTestLevel2);
    }
    {
        uint32_t delayFrames;
        EXPECT_TRUE(proxy->getBufferDelay(delayFrames));
        EXPECT_EQ(delayFrames, kDelayFrames);
    }
    proxy->writeBuffer(0, nullptr);
    {
        bool supportDeferredPlay;
        uint32_t preferredFrames;
        uint32_t maximumFrames;
        EXPECT_TRUE(proxy->getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlay));
        ASSERT_EQ(preferredFrames, kPreferredFrames);
        ASSERT_EQ(maximumFrames, kMaximumFrames);
        ASSERT_EQ(supportDeferredPlay, kSupportDeferredPlay);
    }
    proxy->getClient();
}
