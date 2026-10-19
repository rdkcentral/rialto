/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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
#include "GstBatchDecryptorPrivate.h"

class RialtoServerDecryptorPrivateDecryptMultiTest
    : public RialtoServerDecryptorPrivateTestBase<GstRialtoBatchDecryptorPrivate>
{
protected:
    GstBuffer m_buffer2 = {};
    GstProtectionMeta m_protectionMeta2 = {};
    GstRialtoProtectionData m_protectionData2 = m_protectionData;
    const int32_t m_keySessionId2 = static_cast<int32_t>(m_keySessionId + 1);
    std::vector<GstBuffer *> m_bufferVec{&m_buffer, &m_buffer2};

    std::vector<GstBuffer *> &getBufferVec() { return m_bufferVec; }

    void expectGetProtectionData(GstBuffer *buffer, GstRialtoProtectionData *protectionData)
    {
        EXPECT_CALL(*m_protectionMetadataWrapperMock, getProtectionMetadataData(buffer)).WillOnce(Return(protectionData));
    }

    void expectCreateProtectionMetaInfo(int times)
    {
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("kid"), GST_TYPE_BUFFER, &m_key))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("iv"), GST_TYPE_BUFFER, &m_iv))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("subsample_count"),
                                                               G_TYPE_UINT, m_subsampleCount))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewBufferStub(StrEq("application/x-cenc"), StrEq("subsamples"),
                                                                 GST_TYPE_BUFFER, &m_subsamples))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("encryption_scheme"), G_TYPE_UINT, 0))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewUintStub(StrEq("application/x-cenc"), StrEq("init_with_last_15"),
                                                               G_TYPE_UINT, m_initWithLast15))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureNewStringStub(StrEq("application/x-cenc"), StrEq("cipher-mode"),
                                                                 G_TYPE_STRING, StrEq(toString(m_cipherMode))))
            .Times(times)
            .WillRepeatedly(Return(&m_structure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureSetUintStub(&m_structure, StrEq("crypt_byte_block"), G_TYPE_UINT, m_crypt))
            .Times(times);
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureSetUintStub(&m_structure, StrEq("skip_byte_block"), G_TYPE_UINT, m_skip))
            .Times(times);
    }

    void expectAddProtectionMeta(GstBuffer *buffer, GstProtectionMeta *meta)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferAddProtectionMeta(buffer, &m_structure)).WillOnce(Return(meta));
    }

    void expectKeyMapping(int times)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferMap(&m_key, _, GST_MAP_READ))
            .Times(times)
            .WillRepeatedly(DoAll(SetArgPointee<1>(m_keyMap), Return(TRUE)));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferUnmap(&m_key, _)).Times(times);
    }

    void expectBothBuffersEncrypted()
    {
        expectGetProtectionData(&m_buffer, &m_protectionData);
        expectGetProtectionData(&m_buffer2, &m_protectionData2);
        expectCreateProtectionMetaInfo(2);
        expectAddProtectionMeta(&m_buffer, &m_protectionMeta);
        expectAddProtectionMeta(&m_buffer2, &m_protectionMeta2);
        expectKeyMapping(2);
    }

    void expectExtendedInterfaceUsed(int32_t keySessionId, bool isUsed)
    {
        EXPECT_CALL(*m_decryptionServiceMock, isExtendedInterfaceUsed(keySessionId)).WillOnce(Return(isUsed));
    }

    void expectRemoveProtectionMetadata()
    {
        EXPECT_CALL(*m_protectionMetadataWrapperMock, removeProtectionMetadata(&m_buffer));
        EXPECT_CALL(*m_protectionMetadataWrapperMock, removeProtectionMetadata(&m_buffer2));
    }

    void expectDecryptMulti(firebolt::rialto::MediaKeyErrorStatus status)
    {
        expectExtendedInterfaceUsed(m_keySessionId, false);
        EXPECT_CALL(*m_decryptionServiceMock,
                    decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer, &m_buffer2), &m_caps))
            .WillOnce(Return(status));
    }
};

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti returns success for clear samples without decryption.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, SuccessClear)
{
    expectGetProtectionData(&m_buffer, nullptr);
    expectGetProtectionData(&m_buffer2, nullptr);
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti returns success for encrypted samples.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, SuccessEncrypted)
{
    expectBothBuffersEncrypted();
    expectDecryptMulti(firebolt::rialto::MediaKeyErrorStatus::OK);
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti passes only encrypted samples to decryption.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, SuccessClearAndEncrypted)
{
    expectGetProtectionData(&m_buffer, nullptr);
    expectGetProtectionData(&m_buffer2, &m_protectionData2);
    expectCreateProtectionMetaInfo(1);
    expectAddProtectionMeta(&m_buffer2, &m_protectionMeta2);
    expectKeyMapping(1);
    expectExtendedInterfaceUsed(m_keySessionId, false);
    EXPECT_CALL(*m_decryptionServiceMock, decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer2), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti drops samples if the decryption service decryptMulti fails.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, DecryptionServiceDecryptFailure)
{
    expectBothBuffersEncrypted();
    expectDecryptMulti(firebolt::rialto::MediaKeyErrorStatus::FAIL);
    expectRemoveProtectionMetadata();
    expectDecryptWarning();

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti drops samples and posts HDCP message when output is restricted.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, OutputRestricted)
{
    GstStructure hdcpFailureStructure{};
    GstMessage hdcpFailureMessage{};

    expectBothBuffersEncrypted();
    expectDecryptMulti(firebolt::rialto::MediaKeyErrorStatus::OUTPUT_RESTRICTED);
    expectRemoveProtectionMetadata();
    EXPECT_CALL(*m_gstWrapperMock, gstStructureNewStringStub(StrEq("HDCPProtectionFailure"), StrEq("message"),
                                                             G_TYPE_STRING, StrEq("HDCP Output Protection Error")))
        .WillOnce(Return(&hdcpFailureStructure));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageNewApplication(GST_OBJECT_CAST(&m_decryptorElement), &hdcpFailureStructure))
        .WillOnce(Return(&hdcpFailureMessage));
    EXPECT_CALL(*m_gstWrapperMock, gstElementPostMessage(GST_ELEMENT_CAST(&m_decryptorElement), &hdcpFailureMessage))
        .WillOnce(Return(TRUE));

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti drops samples when no decryption service set.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, NoDecryptionService)
{
    m_gstRialtoDecryptorPrivate->setDecryptionService(nullptr);

    expectBothBuffersEncrypted();
    expectRemoveProtectionMetadata();
    expectDecryptWarning();

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti drops samples when protection meta cannot be added.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, AddProtectionMetaFailure)
{
    expectGetProtectionData(&m_buffer, &m_protectionData);
    expectCreateProtectionMetaInfo(1);
    expectAddProtectionMeta(&m_buffer, nullptr);
    expectRemoveProtectionMetadata();
    expectDecryptWarning();

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti decrypts samples of different key sessions separately.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, DifferentKeySessions)
{
    m_protectionData2.keySessionId = m_keySessionId2;

    expectBothBuffersEncrypted();
    expectExtendedInterfaceUsed(m_keySessionId, false);
    expectExtendedInterfaceUsed(m_keySessionId2, false);
    EXPECT_CALL(*m_decryptionServiceMock, decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_decryptionServiceMock, decryptMulti(m_keySessionId2, ::testing::ElementsAre(&m_buffer2), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti stops decryption and drops samples when one of key sessions fails.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, DifferentKeySessionsFirstFailure)
{
    m_protectionData2.keySessionId = m_keySessionId2;

    expectBothBuffersEncrypted();
    expectExtendedInterfaceUsed(m_keySessionId, false);
    EXPECT_CALL(*m_decryptionServiceMock, decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::FAIL));
    expectRemoveProtectionMetadata();
    expectDecryptWarning();

    EXPECT_EQ(GST_BASE_TRANSFORM_FLOW_DROPPED, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti selects the key id before decryption, when extended interface is used.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, SuccessWithKeyIdSelection)
{
    expectBothBuffersEncrypted();
    expectExtendedInterfaceUsed(m_keySessionId, true);
    EXPECT_CALL(*m_decryptionServiceMock, selectKeyId(m_keySessionId, m_keyVector))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_decryptionServiceMock,
                decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer, &m_buffer2), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}

/**
 * Test GstRialtoBatchDecryptorPrivate decryptMulti decrypts samples without key id selection, when key id cannot be mapped.
 */
TEST_F(RialtoServerDecryptorPrivateDecryptMultiTest, SuccessWhenKeyMappingFails)
{
    expectGetProtectionData(&m_buffer, &m_protectionData);
    expectGetProtectionData(&m_buffer2, nullptr);
    expectCreateProtectionMetaInfo(1);
    expectAddProtectionMeta(&m_buffer, &m_protectionMeta);
    expectKeyMappingFailure();
    EXPECT_CALL(*m_decryptionServiceMock, decryptMulti(m_keySessionId, ::testing::ElementsAre(&m_buffer), &m_caps))
        .WillOnce(Return(firebolt::rialto::MediaKeyErrorStatus::OK));
    expectRemoveProtectionMetadata();

    EXPECT_EQ(GST_FLOW_OK, m_gstRialtoDecryptorPrivate->decryptMulti(getBufferVec(), &m_caps));
}
