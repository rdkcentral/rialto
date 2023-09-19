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

#ifndef RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_MOCK_H_
#define RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_MOCK_H_

#include "IJsonCppWrapper.h"
#include <gmock/gmock.h>
#include <memory>

namespace rialto::servermanager::service
{
class JsonValueWrapperMock : public IJsonValueWrapper
{
public:
    MOCK_METHOD(bool, isMember, (const JSONCPP_STRING &key), (const, override));
    MOCK_METHOD(std::shared_ptr<IJsonValueWrapper>, at, (const JSONCPP_STRING &key), (const, override));
    MOCK_METHOD(std::shared_ptr<IJsonValueWrapper>, at, (Json::ArrayIndex index), (const, override));
    MOCK_METHOD(Json::ArrayIndex, size, (), (const, override));
    MOCK_METHOD(bool, isArray, (), (const, override));
    MOCK_METHOD(bool, isString, (), (const, override));
    MOCK_METHOD(bool, isUInt, (), (const, override));
    MOCK_METHOD(JSONCPP_STRING, asString, (), (const, override));
    MOCK_METHOD(unsigned int, asUInt, (), (const, override));
};

class JsonCppWrapperMock : public IJsonCppWrapper
{
public:
    MOCK_METHOD(bool, parseFromStream,
                (Json::CharReader::Factory const &, std::istream &, std::shared_ptr<IJsonValueWrapper> &root,
                 JSONCPP_STRING *errs),
                (override));
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_MOCK_H_
