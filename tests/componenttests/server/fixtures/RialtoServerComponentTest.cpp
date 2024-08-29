/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "RialtoServerComponentTest.h"
#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "IFactoryAccessor.h"
#include "Matchers.h"
#include "MessageBuilders.h"
#include "servermanagermodule.pb.h"
#include <string>

using testing::_;
using testing::AtLeast;
using testing::Return;
using testing::StrEq;

namespace
{
template <typename T> class GListWrapper
{
public:
    explicit GListWrapper(std::initializer_list<T> elements)
    {
        for (T element : elements)
        {
            m_list = g_list_append(m_list, element);
        }
    }
    ~GListWrapper() { g_list_free(m_list); }
    GList *get() { return m_list; }

private:
    GList *m_list = nullptr;
};

GstStaticPadTemplate createSinkPadTemplate()
{
    GstStaticPadTemplate decoderPadTemplate;
    decoderPadTemplate.direction = GST_PAD_SINK;
    return decoderPadTemplate;
}
} // namespace

namespace firebolt::rialto::server::ct
{
RialtoServerComponentTest::RialtoServerComponentTest()
{
    configureWrappers();
    startSut();
    initializeSut();
}

RialtoServerComponentTest::~RialtoServerComponentTest()
{
    disconnectClient();

    wrappers::IFactoryAccessor::instance().glibWrapperFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().linuxWrapperFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().ocdmFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().ocdmSystemFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().rdkGstreamerUtilsWrapperFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().textTrackPluginWrapperFactory() = nullptr;
    wrappers::IFactoryAccessor::instance().thunderWrapperFactory() = nullptr;
}

void RialtoServerComponentTest::willConfigureSocket()
{
    EXPECT_CALL(*m_linuxWrapperMock, chmod(StrEq(kSocketName), kDefaultPermissions)).WillOnce(Return(0));
    EXPECT_CALL(*m_linuxWrapperMock, getpwnam_r(StrEq(kOwnerName), _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_linuxWrapperMock, getgrnam_r(StrEq(kOwnerName), _, _, _, _)).WillOnce(Return(0));
}

void RialtoServerComponentTest::configureSutInActiveState()
{
    ::rialto::SetConfigurationRequest request{createGenericSetConfigurationReq()};
    request.set_initialsessionserverstate(::rialto::SessionServerState::ACTIVE);

    ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

    ConfigureAction<SetConfiguration>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedMessage = expectedMessage.getMessage();
    ASSERT_TRUE(receivedMessage);
    EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::ACTIVE);
}

void RialtoServerComponentTest::connectClient()
{
    EXPECT_TRUE(m_clientStub.connect());
}

void RialtoServerComponentTest::disconnectClient()
{
    m_clientStub.disconnect();
}

void RialtoServerComponentTest::setStateActive()
{
    ::rialto::SetStateRequest request{createSetStateRequest(::rialto::SessionServerState::ACTIVE)};

    ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

    ConfigureAction<::firebolt::rialto::server::ct::SetState>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedMessage = expectedMessage.getMessage();
    ASSERT_TRUE(receivedMessage);
    EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::ACTIVE);
}

void RialtoServerComponentTest::setStateInactive()
{
    ::rialto::SetStateRequest request{createSetStateRequest(::rialto::SessionServerState::INACTIVE)};

    ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

    ConfigureAction<::firebolt::rialto::server::ct::SetState>(m_serverManagerStub).send(request).expectSuccess();

    auto receivedMessage = expectedMessage.getMessage();
    ASSERT_TRUE(receivedMessage);
    EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::INACTIVE);
}

void RialtoServerComponentTest::configureWrappers() const
{
    EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillRepeatedly(Return(m_glibWrapperMock));
    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillRepeatedly(Return(m_gstWrapperMock));
    EXPECT_CALL(*m_linuxWrapperFactoryMock, createLinuxWrapper()).WillRepeatedly(Return(m_linuxWrapperMock));
    EXPECT_CALL(*m_ocdmFactoryMock, getOcdm()).Times(AtLeast(0)).WillRepeatedly(Return(m_ocdmMock));
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(_)).Times(AtLeast(0)).WillRepeatedly(Return(m_ocdmSystemMock));
    EXPECT_CALL(*m_rdkGstreamerUtilsWrapperFactoryMock, createRdkGstreamerUtilsWrapper())
        .Times(AtLeast(0))
        .WillRepeatedly(Return(m_rdkGstreamerUtilsWrapperMock));
    EXPECT_CALL(*m_textTrackPluginWrapperFactoryMock, getTextTrackPluginWrapper())
        .Times(AtLeast(0))
        .WillRepeatedly(Return(m_textTrackPluginWrapperMock));
    EXPECT_CALL(*m_thunderWrapperFactoryMock, getThunderWrapper())
        .Times(AtLeast(0))
        .WillRepeatedly(Return(m_thunderWrapperMock));
    wrappers::IFactoryAccessor::instance().glibWrapperFactory() = m_glibWrapperFactoryMock;
    wrappers::IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock;
    wrappers::IFactoryAccessor::instance().linuxWrapperFactory() = m_linuxWrapperFactoryMock;
    wrappers::IFactoryAccessor::instance().ocdmFactory() = m_ocdmFactoryMock;
    wrappers::IFactoryAccessor::instance().ocdmSystemFactory() = m_ocdmSystemFactoryMock;
    wrappers::IFactoryAccessor::instance().rdkGstreamerUtilsWrapperFactory() = m_rdkGstreamerUtilsWrapperFactoryMock;
    wrappers::IFactoryAccessor::instance().textTrackPluginWrapperFactory() = m_textTrackPluginWrapperFactoryMock;
    wrappers::IFactoryAccessor::instance().thunderWrapperFactory() = m_thunderWrapperFactoryMock;
}

void RialtoServerComponentTest::startSut()
{
    char dummy{0};
    GstElementFactory *decoderFactory =
        reinterpret_cast<GstElementFactory *>(&dummy); // just dummy address is needed, will not be dereferenced
    GstCaps audioCaps;
    GstCaps videoCaps;
    GstCaps supportedCaps;
    GstCaps unsupportedCaps;
    GListWrapper<GstElementFactory *> listDecoders({decoderFactory});
    auto audioSinkPadTemplate{createSinkPadTemplate()};
    auto videoSinkPadTemplate{createSinkPadTemplate()};
    GListWrapper<GstStaticPadTemplate *> listPadTemplates({&audioSinkPadTemplate, &videoSinkPadTemplate});

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders.get()));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(listPadTemplates.get()));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&audioSinkPadTemplate.static_caps)).WillRepeatedly(Return(&audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&videoSinkPadTemplate.static_caps)).WillRepeatedly(Return(&videoCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&videoCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&videoCaps, &audioCaps)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsFromString(_)).WillRepeatedly(Return(&unsupportedCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsFromString(PtrStrMatcher("audio/mpeg, mpegversion=(int)4")))
        .WillOnce(Return(&supportedCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsFromString(PtrStrMatcher("video/x-h264"))).WillOnce(Return(&supportedCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&supportedCaps)).Times(AtLeast(1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&unsupportedCaps)).Times(AtLeast(1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&supportedCaps, &audioCaps)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&supportedCaps, &videoCaps)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&unsupportedCaps, &audioCaps)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&unsupportedCaps, &videoCaps)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders.get()));

    m_sut = IApplicationSessionServerFactory::getFactory()->createApplicationSessionServer();
}

void RialtoServerComponentTest::initializeSut()
{
    constexpr int kArgc{2};
    std::string binaryName{"RialtoServer"};
    std::string socketStr{std::to_string(m_serverManagerStub.getServerSocket())};
    char *argv[2]{binaryName.data(), socketStr.data()};

    ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);
    m_sut->init(kArgc, argv);

    auto receivedMessage = expectedMessage.getMessage();
    ASSERT_TRUE(receivedMessage);
    EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::UNINITIALIZED);
}
} // namespace firebolt::rialto::server::ct
