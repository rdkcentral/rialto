/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include <iostream>
#include <vector>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"
#include "WebAudioTestCommon.h"

#include <iostream>
using namespace std; // todo remove

using testing::_;
using testing::Return;
using testing::StrEq;
using testing::AtLeast;
using testing::Invoke;

using ::google::protobuf::int32;
using ::google::protobuf::uint32;

namespace firebolt::rialto::server::ct
{
class WebAudioTest : public RialtoServerComponentTest
{
public:
    WebAudioTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    virtual ~WebAudioTest() {}

    void willCreateWebAudioPlayer();
    void createWebAudioPlayer();
    
protected:
    GstElement m_pipeline{};
    GstElement m_appSrc{};
    GstRegistry m_reg{};
    GstObject m_feature{};
    GstElement m_sink{};

    const uint32 m_kPcmRate{41000};
    const uint32 m_kPcmChannels{2};
    const uint32 m_kPcmSampleSize{16};
    const bool m_kPcmIsBigEndian{true};
    const bool m_kPcmIsSigned{true};
    const bool m_kPcmIsFloat{false};
    const std::string m_kAudioMimeType{"audio/x-raw"};
};

void WebAudioTest::willCreateWebAudioPlayer()
{

#if 0
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(_)).WillOnce(Invoke(
            [&](gpointer object) -> GstStateChangeReturn
            {
                abort();
            }));
#endif

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(_,_,_,_))
        .WillOnce(Invoke(
            [&](GstPlugin *plugin, const gchar *name, guint rank, GType type) -> gboolean
            {
                cout << "@@@" << name << endl;
                return true;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(StrEq("webaudiopipeline3"))).WillOnce(Return(&m_pipeline));

    // Similar to tests in the class GstWebAudioPlayerTestCommon
    
    // GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(&m_appSrc), 10 * 1024));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_appSrc), StrEq("format")));

    // GstWebAudioPlayerTestCommon::expectMakeAmlhalaSink()
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, StrEq("amlhalasink")))
        .WillOnce(Return(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("amlhalasink"), StrEq("webaudiosink")))
        .WillOnce(Return(&m_sink));

    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq("direct-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_PLUGIN_FEATURE(&m_feature)));

    // From GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("audsrc"))).WillOnce(Return(&m_appSrc));


    // From GstWebAudioPlayerTestCommon::expectLinkElements()
    GstElement convert{};
    GstElement resample{};
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioconvert"), _)).WillOnce(Return(&convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioresample"), _)).WillOnce(Return(&resample));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_appSrc)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_sink)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_appSrc, &convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&convert, &resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&resample, &m_sink)).WillOnce(Return(TRUE));

    // Similar to GenericTasksTestsBase::shouldAttachAudioSourceWithChannelsAndRate()
    GstCaps gstCaps1{};
    GstCaps gstCaps2{};
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq(m_kAudioMimeType)))
        .WillOnce(Return(&gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&gstCaps1, StrEq("channels"), G_TYPE_INT, m_kPcmChannels));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&gstCaps1, StrEq("rate"), G_TYPE_INT, m_kPcmRate));

    ///////////////////////////////////////////////
    // The following EXPECTS are generated from GstDispatcherThread::gstBusEventHandler()
    // GstWebAudioPlayerTestCommon::expectTermPipeline ??
    GstBus m_bus{};
    constexpr int kGetBusNumberOfCalls{2};
    GstMessage m_message{};
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).Times(kGetBusNumberOfCalls).WillRepeatedly(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillRepeatedly(Return(&m_message));

    // WebAudioTasksTestsBase::shouldBuildPcmCaps ??
    std::string formatStr{testcommon::getPcmFormat(m_kPcmIsFloat, m_kPcmIsSigned, m_kPcmSampleSize, m_kPcmIsBigEndian)};
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&gstCaps1, StrEq("format"), G_TYPE_STRING, StrEq(formatStr)));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&gstCaps1, StrEq("layout"), G_TYPE_STRING, StrEq("interleaved")));

    constexpr uint64_t kChannelMask{5};
    EXPECT_CALL(*m_gstWrapperMock, gstAudioChannelGetFallbackMask(m_kPcmChannels))
        .WillOnce(Return(kChannelMask));

    // WebAudioTasksTestsBase::shouldBuildPcmCaps()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleBitMaskStub(&gstCaps1, StrEq("channel-mask"),
                                                               GST_TYPE_BITMASK, kChannelMask));

    // WebAudioTasksTestsBase::shouldGetCapsStr()
    gchar m_capsStr{};
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_capsStr));

    // WebAudioTasksTestsBase::shouldNotSetCapsWhenCapsEqual()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&gstCaps2, &gstCaps1))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc)))
        .WillOnce(Return(&gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(_)).Times(AtLeast(1)); // Todo - work out where the passed in pointer comes from
    EXPECT_CALL(*m_gstWrapperMock, gstMessageUnref(_)).Times(AtLeast(1)); // Todo - work out where the passed in pointer comes from

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus)).Times(kGetBusNumberOfCalls);
    // The above EXPECTS were generated from GstDispatcherThread::gstBusEventHandler()
    ///////////////////////////////////////////////

    // Generated from GstWebAudioPlayer::changePipelineState
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, _)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
    }
    
void WebAudioTest::createWebAudioPlayer()
{
    constexpr uint32 kPriority{3};
    auto request = createCreateWebAudioPlayerRequest(m_kPcmRate, m_kPcmChannels, m_kPcmSampleSize, m_kPcmIsBigEndian, m_kPcmIsSigned, m_kPcmIsFloat, m_kAudioMimeType, kPriority);
    
    ConfigureAction<CreateWebAudioPlayer>(m_clientStub)
        .send(request)
        .expectSuccess();
}

    
TEST_F(WebAudioTest, testAllApisWithMultipleQueries)
{

    willCreateWebAudioPlayer();    
    createWebAudioPlayer();    
}
} // namespace firebolt::rialto::server::ct
