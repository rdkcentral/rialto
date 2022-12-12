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
#include "GstDecryptorPrivate.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerCreateDecryptorPrivateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    GstBaseTransform m_decryptorBase = {};

    RialtoServerCreateDecryptorPrivateTest()
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();
    }
};

/**
 * Test that a GstRialtoDecryptorPrivate object can be created successfully.
 */
TEST_F(RialtoServerCreateDecryptorPrivateTest, Create)
{
    std::unique_ptr<GstRialtoDecryptorPrivate> gstRialtoDecryptorPrivate;

    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));

    EXPECT_NO_THROW(gstRialtoDecryptorPrivate = std::make_unique<GstRialtoDecryptorPrivate>(&m_decryptorBase, m_gstWrapperFactoryMock););
    EXPECT_NE(gstRialtoDecryptorPrivate, nullptr);
}

/**
 * Test that GstRialtoDecryptorPrivate throws and execption if getGstWrapper fails.
 */
TEST_F(RialtoServerCreateDecryptorPrivateTest, getGstWrapperFails)
{
    std::unique_ptr<GstRialtoDecryptorPrivate> gstRialtoDecryptorPrivate;

    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(nullptr));

    EXPECT_THROW(gstRialtoDecryptorPrivate = std::make_unique<GstRialtoDecryptorPrivate>(&m_decryptorBase, m_gstWrapperFactoryMock),
                 std::runtime_error);
}
