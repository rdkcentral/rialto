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

#ifndef WEB_AUDIO_TASKS_TESTS_CONTEXT_H_
#define WEB_AUDIO_TASKS_TESTS_CONTEXT_H_

#include "WebAudioPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include <memory>

/**
 * @brief WebAudioTasksTests context
 *
 * Stores all objects and non-const variables so that constuction and destruction can be managed.
 */
class WebAudioTasksTestsContext
{
public:
    firebolt::rialto::server::WebAudioPlayerContext m_context;

    // Mocks
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};

    // Gstreamer members
    GstElement m_src{};

    // Glib members
};

#endif // WEB_AUDIO_TASKS_TESTS_CONTEXT_H_
