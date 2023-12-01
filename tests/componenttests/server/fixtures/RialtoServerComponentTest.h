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
#include <gtest/gtest.h>
#include <memory>

namespace firebolt::rialto::server::ct
{
class RialtoServerComponentTest : public ::testing::Test
{
public:
    RialtoServerComponentTest();
    ~RialtoServerComponentTest() override = default;

    void willConfigureSocket();
    void configureSutInActiveState();

private:
    void configureWrappers() const;
    void startSut();
    void initializeSut();

protected:
    ServerManagerStub m_serverManagerStub;
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

private:
    std::unique_ptr<IApplicationSessionServer> m_sut;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_RIALTO_SERVER_COMPONENT_TEST_H_
