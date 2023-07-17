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

#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstDecryptorElementFactoryMock.h"
#include "GstSrc.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerCreateGstSrcTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock;
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock;
    std::shared_ptr<StrictMock<GstDecryptorElementFactoryMock>> m_decryptorFactoryMock;

    virtual void SetUp()
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();

        m_glibWrapperFactoryMock = std::make_shared<StrictMock<GlibWrapperFactoryMock>>();
        m_glibWrapperMock = std::make_shared<StrictMock<GlibWrapperMock>>();

        m_decryptorFactoryMock = std::make_shared<StrictMock<GstDecryptorElementFactoryMock>>();
    }

    virtual void TearDown() {}
};

/**
 * Test that a GstSrc object can be created successfully.
 */
TEST_F(RialtoServerCreateGstSrcTest, Create)
{
    std::unique_ptr<IGstSrc> gstSrc;

    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
    EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));

    EXPECT_NO_THROW(
        gstSrc = std::make_unique<GstSrc>(m_gstWrapperFactoryMock, m_glibWrapperFactoryMock, m_decryptorFactoryMock));
    EXPECT_NE(gstSrc, nullptr);
}
