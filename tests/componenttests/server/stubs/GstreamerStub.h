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

#ifndef FIREBOLT_RIALTO_SERVER_CT_GSTREAMER_STUB_H_
#define FIREBOLT_RIALTO_SERVER_CT_GSTREAMER_STUB_H_

#include "GlibWrapperMock.h"
#include "GstWrapperMock.h"
#include <condition_variable>
#include <gmock/gmock.h>
#include <gst/gst.h>
#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::ct
{
class GstreamerStub
{
public:
    GstreamerStub(const std::shared_ptr<testing::StrictMock<wrappers::GlibWrapperMock>> &glibWrapperMock,
                  const std::shared_ptr<testing::StrictMock<wrappers::GstWrapperMock>> &gstWrapperMock,
                  GstElement *pipeline, GstBus *bus, GstElement *rialtoSource);
    ~GstreamerStub() = default;

    void setupPipeline();
    void setupRialtoSource();
    void setupAppSrcCallbacks(GstAppSrc *appSrc);
    void sendStateChanged(GstState oldState, GstState newState, GstState pendingState);
    void needData(GstAppSrc *appSrc, guint dataLength);

private:
    std::shared_ptr<testing::StrictMock<wrappers::GlibWrapperMock>> m_glibWrapperMock;
    std::shared_ptr<testing::StrictMock<wrappers::GstWrapperMock>> m_gstWrapperMock;
    GstElement *m_pipeline;
    GstBus *m_bus;
    GstElement *m_rialtoSource;
    GstMessage *m_message;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    gpointer m_setupSourceUserData{nullptr};
    GCallback m_setupSourceFunc{};
    gpointer m_setupElementUserData{nullptr};
    GCallback m_setupElementFunc{};
    gpointer m_deepElementAddedUserData{nullptr};
    GCallback m_deepElementAddedFunc{};
    gulong m_setupSourceSignalId{0};
    gulong m_setupElementSignalId{1};
    gulong m_deepElementAddedSignalId{2};
    std::map<GstAppSrc *, GstAppSrcCallbacks> m_appSrcCallbacks;
    std::map<GstAppSrc *, gpointer> m_appSrcCallbacksUserDatas;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_GSTREAMER_STUB_H_
