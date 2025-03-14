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

#ifndef FIREBOLT_RIALTO_SERVER_CT_RIALTO_SERVER_COMPONENT_TEST_H_
#define FIREBOLT_RIALTO_SERVER_CT_RIALTO_SERVER_COMPONENT_TEST_H_

#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>

#include "ClientStub.h"
#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IApplicationSessionServer.h"
#include "LinuxWrapperFactoryMock.h"
#include "LinuxWrapperMock.h"
#include "OcdmFactoryMock.h"
#include "OcdmMock.h"
#include "OcdmSystemFactoryMock.h"
#include "OcdmSystemMock.h"
#include "RdkGstreamerUtilsWrapperFactoryMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include "ServerManagerStub.h"
#include "TextTrackPluginWrapperFactoryMock.h"
#include "TextTrackPluginWrapperMock.h"
#include "TextTrackWrapperMock.h"
#include "ThunderWrapperFactoryMock.h"
#include "ThunderWrapperMock.h"

namespace firebolt::rialto::server::ct
{
class RialtoServerComponentTest : public ::testing::Test
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

public:
    RialtoServerComponentTest();
    ~RialtoServerComponentTest() override;

    void configureSutInActiveState();
    void connectClient();
    void disconnectClient();
    void setStateActive();
    void setStateInactive();

protected:
    void configureWrappers() const;
    void startSut();
    void initialiseSut();
    void initialiseGstreamer();

protected:
    ServerManagerStub m_serverManagerStub;
    ClientStub m_clientStub;
    std::shared_ptr<testing::StrictMock<wrappers::GlibWrapperFactoryMock>> m_glibWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::GlibWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::GlibWrapperMock>> m_glibWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::GlibWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::GstWrapperFactoryMock>> m_gstWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::GstWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::GstWrapperMock>> m_gstWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::GstWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::LinuxWrapperFactoryMock>> m_linuxWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::LinuxWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::LinuxWrapperMock>> m_linuxWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::LinuxWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::OcdmFactoryMock>> m_ocdmFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::OcdmFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::OcdmMock>> m_ocdmMock{
        std::make_shared<testing::StrictMock<wrappers::OcdmMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::OcdmSystemFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::OcdmSystemMock>> m_ocdmSystemMock{
        std::make_shared<testing::StrictMock<wrappers::OcdmSystemMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::RdkGstreamerUtilsWrapperFactoryMock>> m_rdkGstreamerUtilsWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::RdkGstreamerUtilsWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::RdkGstreamerUtilsWrapperMock>> m_rdkGstreamerUtilsWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::RdkGstreamerUtilsWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::TextTrackPluginWrapperFactoryMock>> m_textTrackPluginWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::TextTrackPluginWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::TextTrackPluginWrapperMock>> m_textTrackPluginWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::TextTrackPluginWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::TextTrackWrapperMock>> m_textTrackWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::TextTrackWrapperMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::ThunderWrapperFactoryMock>> m_thunderWrapperFactoryMock{
        std::make_shared<testing::StrictMock<wrappers::ThunderWrapperFactoryMock>>()};
    std::shared_ptr<testing::StrictMock<wrappers::ThunderWrapperMock>> m_thunderWrapperMock{
        std::make_shared<testing::StrictMock<wrappers::ThunderWrapperMock>>()};

    // GstCapabilities data
    GstCaps m_audioCaps;
    GstCaps m_videoCaps;
    GstCaps m_supportedCaps;
    GstCaps m_unsupportedCaps;
    GstStaticPadTemplate m_audioSinkPadTemplate;
    GstStaticPadTemplate m_videoSinkPadTemplate;
    char m_dummy{0};
    GstElementFactory *m_decoderFactory{
        reinterpret_cast<GstElementFactory *>(&m_dummy)}; // just dummy address is needed, will not be dereferenced
    GListWrapper<GstStaticPadTemplate *> m_listPadTemplates;
    GListWrapper<GstElementFactory *> m_listDecoders;

    std::unique_ptr<IApplicationSessionServer> m_sut;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_RIALTO_SERVER_COMPONENT_TEST_H_
