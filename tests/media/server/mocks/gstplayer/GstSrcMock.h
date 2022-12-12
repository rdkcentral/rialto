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

#ifndef FIREBOLT_RIALTO_SERVER_GST_SRC_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_SRC_MOCK_H_

#include "IGstSrc.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::server
{
class GstSrcMock : public IGstSrc
{
public:
    GstSrcMock() = default;
    virtual ~GstSrcMock() = default;

    MOCK_METHOD(void, initSrc, (), (override));
    MOCK_METHOD(void, setupAndAddAppArc,
                (IDecryptionService *decryptionService, GstElement * element, GstElement *appsrc, GstAppSrcCallbacks *callbacks, gpointer userData,
                 firebolt::rialto::MediaSourceType type),
                (override));
    MOCK_METHOD(void, allAppSrcsAdded, (GstElement * element), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_SRC_MOCK_H_
