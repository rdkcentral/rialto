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
#include "DecryptionServiceMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::DoAll;
using ::testing::SetArgPointee;

MATCHER_P(CharStrMatcher, expectedStr, "")
{
    std::string actualStr = (const char *)arg;
    return expectedStr == actualStr;
}

class RialtoServerDecryptorPrivateDecryptTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::unique_ptr<GstRialtoDecryptorPrivate> m_gstRialtoDecryptorPrivate;
    std::shared_ptr<StrictMock<DecryptionServiceMock>> m_decryptionServiceMock;

    GstBaseTransform m_decryptorBase = {};
    GstBuffer m_buffer = {};
    GstProtectionMeta m_protectionMeta = {};
    uint32_t m_keySessionId = 1u;
    uint32_t m_subsampleCount = 2u;
    uint32_t m_initWithLast15 = 3u;
    const GValue m_keyValue = {};
    const GValue m_ivValue = {};
    const GValue m_subsamplesValue = {};
    GstBuffer m_key = {};
    GstBuffer m_iv = {};
    GstBuffer m_subsamples = {};

    RialtoServerDecryptorPrivateDecryptTest()
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();

        m_decryptionServiceMock = std::make_shared<StrictMock<DecryptionServiceMock>>();

        createDecryptorPrivate();

        m_gstRialtoDecryptorPrivate->setDecryptionService(m_decryptionServiceMock.get());
    }

    void createDecryptorPrivate()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));

        EXPECT_NO_THROW(m_gstRialtoDecryptorPrivate = std::make_unique<GstRialtoDecryptorPrivate>(&m_decryptorBase, m_gstWrapperFactoryMock););
        EXPECT_NE(m_gstRialtoDecryptorPrivate, nullptr);
    }

    void expectGetInfoFromProtectionMeta()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
            .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
            .WillOnce(DoAll(SetArgPointee<2>(m_subsampleCount), Return(TRUE)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
            .WillOnce(DoAll(SetArgPointee<2>(m_initWithLast15), Return(TRUE)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("kid")))
            .WillOnce(Return(&m_keyValue));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("iv")))
            .WillOnce(Return(&m_ivValue));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("subsamples")))
            .WillOnce(Return(&m_subsamplesValue));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));
    }

    void expectExtractValueData()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_keyValue))
            .WillOnce(Return(&m_key));
        EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_ivValue))
            .WillOnce(Return(&m_iv));
        EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_subsamplesValue))
            .WillOnce(Return(&m_subsamples));
    }
};

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for an encrypted sample.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessEncrypted)
{
    expectGetInfoFromProtectionMeta();
    expectExtractValueData();

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_subsamples, m_subsampleCount, &m_iv, &m_key, m_initWithLast15))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for an encrypted sample with no subsamples.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessEncryptedNoSubsamples)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(DoAll(SetArgPointee<2>(0), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_initWithLast15), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("kid")))
        .WillOnce(Return(&m_keyValue));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("iv")))
        .WillOnce(Return(&m_ivValue));
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_keyValue))
        .WillOnce(Return(&m_key));
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_ivValue))
        .WillOnce(Return(&m_iv));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, nullptr, 0, &m_iv, &m_key, m_initWithLast15))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for a clear sample.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessClear)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(nullptr));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no decryption service set.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoDecryptionService)
{
    m_gstRialtoDecryptorPrivate->setDecryptionService(nullptr);

    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no key session id set in the protection meta.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoKeySessionId)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no subsample count set in the protection meta.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoSubsampleCount)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no init with last 15 set in the protection meta.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoInitWithLast15)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_subsampleCount), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no kid set in the protection meta.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoKid)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_subsampleCount), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_initWithLast15), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("kid")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no iv set in the protection meta.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoIv)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_subsampleCount), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_initWithLast15), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("kid")))
        .WillOnce(Return(&m_keyValue));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("iv")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when no subsamples set in the protection meta and subsamples count greater than 0.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoSubsamples)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBufferGetProtectionMeta(&m_buffer)).WillOnce(Return(&m_protectionMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("key_session_id"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_keySessionId), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("subsample_count"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_subsampleCount), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint(m_protectionMeta.info, CharStrMatcher("init_with_last_15"),_))
        .WillOnce(DoAll(SetArgPointee<2>(m_initWithLast15), Return(TRUE)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("kid")))
        .WillOnce(Return(&m_keyValue));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("iv")))
        .WillOnce(Return(&m_ivValue));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetValue(m_protectionMeta.info, CharStrMatcher("subsamples")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferRemoveMeta(&m_buffer, reinterpret_cast<GstMeta*>(&m_protectionMeta)));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when the kid set in the protection meta is invalid.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, InvalidKid)
{
    expectGetInfoFromProtectionMeta();
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_keyValue))
        .WillOnce(Return(nullptr));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when the iv set in the protection meta is invalid.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, InvalidIv)
{
    expectGetInfoFromProtectionMeta();
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_keyValue))
        .WillOnce(Return(&m_key));
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_ivValue))
        .WillOnce(Return(nullptr));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error when the subsamples set in the protection meta is invalid.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, InvalidSubsamples)
{
    expectGetInfoFromProtectionMeta();
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_keyValue))
        .WillOnce(Return(&m_key));
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_ivValue))
        .WillOnce(Return(&m_iv));
    EXPECT_CALL(*m_gstWrapperMock, gstValueGetBuffer(&m_subsamplesValue))
        .WillOnce(Return(nullptr));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns error if the decryption service decrypt fails.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, DecryptionServiceDecryptFailure)
{
    expectGetInfoFromProtectionMeta();
    expectExtractValueData();

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_subsamples, m_subsampleCount, &m_iv, &m_key, m_initWithLast15))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(GST_FLOW_ERROR, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer));
}
