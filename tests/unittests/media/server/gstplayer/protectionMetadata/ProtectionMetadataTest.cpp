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

#include "GstProtectionMetadataWrapper.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

class RialtoProtectionMetadataTest : public ::testing::Test
{
protected:
    RialtoProtectionMetadataTest()
        : m_gstWrapperMock(std::make_shared<StrictMock<GstWrapperMock>>()), m_sut(m_gstWrapperMock)

    {
    }

    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    GstProtectionMetadataWrapper m_sut;
};

TEST_F(RialtoProtectionMetadataTest, addProtectionMetadata)
{
    GstBuffer buffer = {};
    GstRialtoProtectionData data = {};
    GstMeta meta = {};
    EXPECT_CALL(*m_gstWrapperMock, gstBufferAddMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_INFO, &data))
        .WillOnce(Return(&meta));

    EXPECT_EQ(&meta, m_sut.addProtectionMetadata(&buffer, data));
}

TEST_F(RialtoProtectionMetadataTest, getProtectionMetadataDataSuccess)
{
    GstBuffer buffer = {};
    GstRialtoProtectionMetadata meta = {};

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_GET_TYPE))
        .WillOnce(Return(reinterpret_cast<GstMeta *>(&meta)));

    EXPECT_EQ(&meta.data, m_sut.getProtectionMetadataData(&buffer));
}

TEST_F(RialtoProtectionMetadataTest, getProtectionMetadataDataNoMeta)
{
    GstBuffer buffer = {};

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_GET_TYPE))
        .WillOnce(Return(nullptr));

    EXPECT_EQ(nullptr, m_sut.getProtectionMetadataData(&buffer));
}

TEST_F(RialtoProtectionMetadataTest, removeProtectionMetadataSuccess)
{
    GstBuffer buffer = {};
    GstMeta meta = {};

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_GET_TYPE))
        .WillOnce(Return(&meta));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&buffer, &meta)).WillOnce(Return(true));

    m_sut.removeProtectionMetadata(&buffer);
}

TEST_F(RialtoProtectionMetadataTest, removeProtectionMetadataFail)
{
    GstBuffer buffer = {};
    GstMeta meta = {};

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_GET_TYPE))
        .WillOnce(Return(&meta));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&buffer, &meta)).WillOnce(Return(false));

    m_sut.removeProtectionMetadata(&buffer);
}

TEST_F(RialtoProtectionMetadataTest, removeProtectionMetadataNoMeta)
{
    GstBuffer buffer = {};

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetMeta(&buffer, GST_RIALTO_PROTECTION_METADATA_GET_TYPE))
        .WillOnce(Return(nullptr));

    m_sut.removeProtectionMetadata(&buffer);
}
