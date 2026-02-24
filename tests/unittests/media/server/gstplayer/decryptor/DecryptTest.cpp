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
#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstDecryptorPrivate.h"
#include "GstProtectionMetadataHelperMock.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::StrictMock;

static const char *toString(const firebolt::rialto::CipherMode &cipherMode)
{
    switch (cipherMode)
    {
    case firebolt::rialto::CipherMode::CBCS:
    {
        return "cbcs";
    }
    case firebolt::rialto::CipherMode::CENC:
    {
        return "cenc";
    }
    case firebolt::rialto::CipherMode::CBC1:
    {
        return "cbc1";
    }
    case firebolt::rialto::CipherMode::CENS:
    {
        return "cens";
    }
    case firebolt::rialto::CipherMode::UNKNOWN:
    default:
    {
        return "unknown";
    }
    }
}

class RialtoServerDecryptorPrivateDecryptTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock;
    std::unique_ptr<GstRialtoDecryptorPrivate> m_gstRialtoDecryptorPrivate;
    std::shared_ptr<StrictMock<DecryptionServiceMock>> m_decryptionServiceMock;
    StrictMock<GstProtectionMetadataHelperMock> *m_protectionMetadataWrapperMock;
    std::unique_ptr<StrictMock<GstProtectionMetadataHelperMock>> protectionMetadataWrapperMock;

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
    GstStructure m_structure{};
    std::vector<uint8_t> m_keyVector{1, 2, 3, 4};
    GstMapInfo m_keyMap{nullptr, GST_MAP_READ, m_keyVector.data(), m_keyVector.size(), m_keyVector.size(), nullptr};
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
    GError m_gError = {};
    GstMessage m_message = {};

    RialtoServerDecryptorPrivateDecryptTest()
        : m_gstWrapperFactoryMock(std::make_shared<StrictMock<GstWrapperFactoryMock>>()),
          m_glibWrapperFactoryMock(std::make_shared<StrictMock<GlibWrapperFactoryMock>>()),
          m_gstWrapperMock(std::make_shared<StrictMock<GstWrapperMock>>()),
          m_glibWrapperMock(std::make_shared<StrictMock<GlibWrapperMock>>()),
          m_decryptionServiceMock(std::make_shared<StrictMock<DecryptionServiceMock>>())
    {
        createDecryptorPrivate();

        m_gstRialtoDecryptorPrivate->setDecryptionService(m_decryptionServiceMock.get());

        protectionMetadataWrapperMock = std::make_unique<StrictMock<GstProtectionMetadataHelperMock>>();
        m_protectionMetadataWrapperMock = protectionMetadataWrapperMock.get();
        m_gstRialtoDecryptorPrivate->setProtectionMetadataWrapper(std::move(protectionMetadataWrapperMock));
    }

    void createDecryptorPrivate()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));

        EXPECT_NO_THROW(m_gstRialtoDecryptorPrivate =
                            std::make_unique<GstRialtoDecryptorPrivate>(&m_decryptorBase, m_gstWrapperFactoryMock,
                                                                        m_glibWrapperFactoryMock));
        EXPECT_NE(m_gstRialtoDecryptorPrivate, nullptr);
    }

    void expectGetInfoFromProtectionMeta()
    {
        EXPECT_CALL(*m_protectionMetadataWrapperMock, getProtectionMetadataData(&m_buffer))
            .WillOnce(Return(&m_protectionData));
        EXPECT_CALL(*m_protectionMetadataWrapperMock, removeProtectionMetadata(&m_buffer));
    }

    void expectWidevineKeySystem()
    {
        EXPECT_CALL(*m_decryptionServiceMock, isExtendedInterfaceUsed(m_keySessionId)).WillOnce(Return(false));
    }

    void expectPlayreadyKeySystem()
    {
        EXPECT_CALL(*m_decryptionServiceMock, isExtendedInterfaceUsed(m_keySessionId)).WillOnce(Return(true));
    }

    void expectKeyMappingFailure()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferMap(&m_key, _, GST_MAP_READ)).WillOnce(Return(FALSE));
    }

    void expectSuccessfulKeyIdSelection()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferMap(&m_key, _, GST_MAP_READ))
            .WillOnce(DoAll(SetArgPointee<1>(m_keyMap), Return(TRUE)));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferUnmap(&m_key, _));
        EXPECT_CALL(*m_decryptionServiceMock, selectKeyId(m_keySessionId, m_keyVector));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&m_key));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferNew()).WillOnce(Return(&m_key));
    }

    void expectAddGstProtectionMeta(bool encryptionPatternSet)
    {
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("kid"), GST_TYPE_BUFFER, &m_key))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("iv"), GST_TYPE_BUFFER, &m_iv))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("subsample_count"),
                                                               G_TYPE_UINT, m_subsampleCount))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("subsamples"),
                                                                 GST_TYPE_BUFFER, &m_subsamples))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("encryption_scheme"), G_TYPE_UINT, 0))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("init_with_last_15"),
                                                               G_TYPE_UINT, m_initWithLast15))
            .WillOnce(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewStringStub(StrEq("application/x-cenc"), StrEq("cipher-mode"),
                                                                 G_TYPE_STRING, StrEq(toString(m_cipherMode))))
            .WillOnce(Return(&m_structure));

        if (encryptionPatternSet)
        {
            EXPECT_CALL(*m_gstWrapperMock,
                        gstStructureSetUintStub(&m_structure, StrEq("crypt_byte_block"), G_TYPE_UINT, m_crypt));
            EXPECT_CALL(*m_gstWrapperMock,
                        gstStructureSetUintStub(&m_structure, StrEq("skip_byte_block"), G_TYPE_UINT, m_skip));
        }

        EXPECT_CALL(*m_gstWrapperMock, gstBufferAddProtectionMeta(&m_buffer, &m_structure))
            .WillOnce(Return(&m_protectionMeta));
    }

    void expectDecryptWarning()
    {
        EXPECT_CALL(*m_glibWrapperMock, gErrorNewLiteral(GST_STREAM_ERROR, GST_STREAM_ERROR_DECRYPT, _))
            .WillOnce(Return(&m_gError));
        EXPECT_CALL(*m_gstWrapperMock, gstMessageNewWarning(GST_OBJECT_CAST(&m_decryptorBase), &m_gError, _))
            .WillOnce(Return(&m_message));
        EXPECT_CALL(*m_gstWrapperMock, gstElementPostMessage(GST_ELEMENT_CAST(&m_decryptorBase), &m_message))
            .WillOnce(Return(TRUE));
        EXPECT_CALL(*m_glibWrapperMock, gErrorFree(&m_gError));
    }
};

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for an encrypted sample.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessEncrypted)
{
    expectGetInfoFromProtectionMeta();
    expectWidevineKeySystem();
    expectAddGstProtectionMeta(true);

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for an encrypted sample with no encryption pattern.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, SuccessEncryptedNoEncryptionPattern)
{
    m_protectionData.encryptionPatternSet = false;

    expectGetInfoFromProtectionMeta();
    expectWidevineKeySystem();
    expectAddGstProtectionMeta(false);

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns OK if the decryption service decrypt fails.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, DecryptionServiceDecryptFailure)
{
    expectGetInfoFromProtectionMeta();
    expectWidevineKeySystem();
    expectAddGstProtectionMeta(true);
    expectDecryptWarning();

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for a playready encrypted sample with mapping problem.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, PlayreadySuccessEncrypted)
{
    expectGetInfoFromProtectionMeta();
    expectPlayreadyKeySystem();
    expectKeyMappingFailure();
    expectAddGstProtectionMeta(true);

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}

/**
 * Test GstRialtoDecryptorPrivate decrypt returns success for a playready encrypted sample
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, PlayreadySuccessEncryptedMappingFail)
{
    expectGetInfoFromProtectionMeta();
    expectPlayreadyKeySystem();
    expectSuccessfulKeyIdSelection();
    expectAddGstProtectionMeta(true);

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
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
    expectDecryptWarning();

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
}
