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
#include "DecryptionServiceMock.h"
#include "GstDecryptorPrivate.h"
#include "GstProtectionMetadataWrapperMock.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

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
    StrictMock<GstProtectionMetadataWrapperMock> *m_protectionMetadataWrapperMock;

    GstBaseTransform m_decryptorBase = {};
    GstBuffer m_buffer = {};
    GstProtectionMeta m_protectionMeta = {};
    uint32_t m_keySessionId = 1u;
    uint32_t m_subsampleCount = 2u;
    uint32_t m_initWithLast15 = 3u;
    uint32_t m_crypt = 4u;
    uint32_t m_skip = 5u;
    firebolt::rialto::CipherMode m_cipherMode = firebolt::rialto::CipherMode::CENC;
    const GValue m_keyValue = {};
    const GValue m_ivValue = {};
    const GValue m_subsamplesValue = {};
    GstBuffer m_key = {};
    GstBuffer m_iv = {};
    GstBuffer m_subsamples = {};
    GstCaps m_caps{};
    GstRialtoProtectionData m_protectionData = {static_cast<int32_t>(m_keySessionId),
                                                m_subsampleCount,
                                                m_initWithLast15,
                                                &m_key,
                                                &m_iv,
                                                &m_subsamples,
                                                m_cipherMode,
                                                m_crypt,
                                                m_skip,
                                                true};

    RialtoServerDecryptorPrivateDecryptTest()
        : m_gstWrapperFactoryMock(std::make_shared<StrictMock<GstWrapperFactoryMock>>()),
          m_gstWrapperMock(std::make_shared<StrictMock<GstWrapperMock>>()),
          m_decryptionServiceMock(std::make_shared<StrictMock<DecryptionServiceMock>>())
    {
        createDecryptorPrivate();

        m_gstRialtoDecryptorPrivate->setDecryptionService(m_decryptionServiceMock.get());

        std::unique_ptr<StrictMock<GstProtectionMetadataWrapperMock>> protectionMetadataWrapperMock =
            std::make_unique<StrictMock<GstProtectionMetadataWrapperMock>>();
        m_protectionMetadataWrapperMock = protectionMetadataWrapperMock.get();
        m_gstRialtoDecryptorPrivate->setProtectionMetadataWrapper(std::move(protectionMetadataWrapperMock));
    }

    void createDecryptorPrivate()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));

        EXPECT_NO_THROW(m_gstRialtoDecryptorPrivate =
                            std::make_unique<GstRialtoDecryptorPrivate>(&m_decryptorBase, m_gstWrapperFactoryMock););
        EXPECT_NE(m_gstRialtoDecryptorPrivate, nullptr);
    }

    void expectGetInfoFromProtectionMeta()
    {
        EXPECT_CALL(*m_protectionMetadataWrapperMock, getProtectionMetadataData(&m_buffer))
            .WillOnce(Return(&m_protectionData));
        EXPECT_CALL(*m_protectionMetadataWrapperMock, removeProtectionMetadata(&m_buffer));
    }
};

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for an encrypted sample.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessEncrypted)
{
    expectGetInfoFromProtectionMeta();

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_subsamples, m_subsampleCount, &m_iv,
                                                  &m_key, m_initWithLast15, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for a clear sample.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessClear)
{
    EXPECT_CALL(*m_protectionMetadataWrapperMock, getProtectionMetadataData(&m_buffer)).WillOnce(Return(nullptr));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns OK when no decryption service set.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, NoDecryptionService)
{
    m_gstRialtoDecryptorPrivate->setDecryptionService(nullptr);

    expectGetInfoFromProtectionMeta();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns OK if the decryption service decrypt fails.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, DecryptionServiceDecryptFailure)
{
    expectGetInfoFromProtectionMeta();

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_subsamples, m_subsampleCount, &m_iv,
                                                  &m_key, m_initWithLast15, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}
