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

#ifndef FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_CLIENT_MOCK_H_

#include "IGstGenericPlayerClient.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::server
{
class GstGenericPlayerClientMock : public IGstGenericPlayerClient
{
public:
    GstGenericPlayerClientMock() = default;
    virtual ~GstGenericPlayerClientMock() = default;

    MOCK_METHOD(void, notifyPlaybackState, (PlaybackState state), (override));
    MOCK_METHOD(bool, notifyNeedMediaData, (MediaSourceType mediaSourceType), (override));
    MOCK_METHOD(void, notifyPosition, (std::int64_t position), (override));
    MOCK_METHOD(void, notifyNetworkState, (NetworkState state), (override));
    MOCK_METHOD(void, clearActiveRequestsCache, (), (override));
    MOCK_METHOD(void, invalidateActiveRequests, (const MediaSourceType &type), (override));
    MOCK_METHOD(void, notifyQos, (MediaSourceType mediaSourceType, const QosInfo &qosInfo), (override));
    MOCK_METHOD(void, notifyBufferUnderflow, (MediaSourceType mediaSourceType), (override));
    MOCK_METHOD(void, notifyPlaybackError, (MediaSourceType mediaSourceType, const PlaybackError &error), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_CLIENT_MOCK_H_
