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
#include "DecryptorPrivateTestBase.h"
#include "GstDecryptorPrivate.h"

using RialtoServerDecryptorPrivateDecryptTest = RialtoServerDecryptorPrivateTestBase<GstRialtoDecryptorPrivate>;

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
 * Test GstRialtoDecryptorPrivate posts an HDCP protection message after decryption recovers.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptTest, ShouldPostOutputProtectionMessageAfterRestrictedDecryptRecovers)
{
    GstStructure hdcpFailureStructure{};
    GstMessage hdcpFailureMessage{};

    expectGetInfoFromProtectionMeta();
    expectWidevineKeySystem();
    expectAddGstProtectionMeta(true);

    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OUTPUT_RESTRICTED));

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));

    expectGetInfoFromProtectionMeta();
    expectWidevineKeySystem();
    expectAddGstProtectionMeta(true);
    EXPECT_CALL(*m_decryptionServiceMock, decrypt(m_keySessionId, &m_buffer, &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureNewStringStub(StrEq("HDCPProtectionFailure"), StrEq("message"),
                                                             G_TYPE_STRING, StrEq("HDCP Output Protection Error")))
        .WillOnce(Return(&hdcpFailureStructure));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageNewApplication(GST_OBJECT_CAST(&m_decryptorElement), &hdcpFailureStructure))
        .WillOnce(Return(&hdcpFailureMessage));
    EXPECT_CALL(*m_gstWrapperMock, gstElementPostMessage(GST_ELEMENT_CAST(&m_decryptorElement), &hdcpFailureMessage))
        .WillOnce(Return(TRUE));

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decrypt(&m_buffer, &m_caps));
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
