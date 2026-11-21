/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "CaptureAdmissionMock.h"
#include "CaptureSessionMock.h"
#include "ICaptureAdmission.h"

#include <gtest/gtest.h>

using firebolt::rialto::server::CaptureAdmissionMock;
using firebolt::rialto::server::CaptureSessionHandle;
using firebolt::rialto::server::ICaptureAdmission;
using firebolt::rialto::server::UnavailableCaptureAdmission;
using ::testing::_;
using ::testing::Invoke;

namespace
{
CaptureSessionHandle handleWithSession(bool &deleted)
{
    CaptureSessionHandle handle;
    handle.sessionId = 4;
    handle.captureId = 9;
    handle.session =
        firebolt::rialto::wrappers::CaptureSessionPtr{new ::testing::NiceMock<firebolt::rialto::capture::CaptureSessionMock>(),
                                                      [&deleted](firebolt::rialto::capture::ICaptureSession *session)
                                                      {
                                                          deleted = true;
                                                          delete session;
                                                      }};
    return handle;
}
} // namespace

TEST(CaptureAdmissionTest, MockAdmissionReceivesTheConfiguredSession)
{
    bool deleted{false};
    CaptureSessionHandle handle{handleWithSession(deleted)};
    std::unique_ptr<ICaptureAdmission> admission{std::make_unique<CaptureAdmissionMock>()};
    EXPECT_CALL(static_cast<CaptureAdmissionMock &>(*admission), admit(_))
        .WillOnce(Invoke(
            [](CaptureSessionHandle &admitted)
            {
                EXPECT_EQ(admitted.sessionId, 4);
                EXPECT_EQ(admitted.captureId, 9U);
                EXPECT_NE(admitted.session, nullptr);
                return true;
            }));
    EXPECT_TRUE(admission->admit(handle));
    EXPECT_FALSE(deleted); // the handle still owns the session
}

TEST(CaptureAdmissionTest, UnavailableAdmissionRefusesAndLeavesTheSessionToTheCaller)
{
    bool deleted{false};
    {
        CaptureSessionHandle handle{handleWithSession(deleted)};
        UnavailableCaptureAdmission admission;
        EXPECT_FALSE(admission.admit(handle));
        EXPECT_NE(handle.session, nullptr);
    }
    EXPECT_TRUE(deleted);
}

TEST(CaptureAdmissionTest, DefaultFactoryCreatesTheUnavailableAdmission)
{
    auto admission = firebolt::rialto::server::ICaptureAdmissionFactory::instance()->createCaptureAdmission();
    ASSERT_NE(admission, nullptr);
    EXPECT_NE(dynamic_cast<UnavailableCaptureAdmission *>(admission.get()), nullptr);
}
