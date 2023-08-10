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

#include "WebAudioTasksTestsBase.h"
#include "Matchers.h"
#include "WebAudioTasksTestsContext.h"
#include "tasks/webAudio/Eos.h"
#include <gst/gst.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<WebAudioTasksTestsContext> testContext;
} // namespace

WebAudioTasksTestsBase::WebAudioTasksTestsBase()
{
    testContext = std::make_shared<GenericTasksTestsContext>();

    gst_init(nullptr, nullptr);

    testContext->m_context.source = &testContext->m_src;
}

WebAudioTasksTestsBase::~WebAudioTasksTestsBase()
{
    testContext.reset();
}
