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

#ifndef MEDIA_PIPELINE_MATCHERS_H_
#define MEDIA_PIPELINE_MATCHERS_H_

#include "mediapipelinemodule.pb.h"
#include "MediaCommon.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

MATCHER_P2(createSessionRequestMatcher, maxWidth, maxHeight, "")
{
    const ::firebolt::rialto::CreateSessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateSessionRequest *>(arg);
    return ((kRequest->max_width() == maxWidth) && (kRequest->max_height() == maxHeight));
}

MATCHER_P4(loadRequestMatcher, sessionId, type, mimeType, url, "")
{
    const ::firebolt::rialto::LoadRequest *kRequest = dynamic_cast<const ::firebolt::rialto::LoadRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->type() == type) &&
            (kRequest->mime_type() == mimeType) && (kRequest->url() == url));
}


#endif // MEDIA_PIPELINE_MATCHERS_H_
