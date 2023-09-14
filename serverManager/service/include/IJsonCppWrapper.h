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

#ifndef RIALTO_SERVERMANAGER_SERVICE_IJSON_CPP_WRAPPER_H_
#define RIALTO_SERVERMANAGER_SERVICE_IJSON_CPP_WRAPPER_H_

#include <json/json.h>
#include <memory>


namespace rialto::servermanager::service
{
class IJsonValueWrapper
{
public:
    virtual ~IJsonValueWrapper() = default;
    virtual bool isMember(const JSONCPP_STRING& key) const = 0;
    virtual std::unique_ptr<IJsonValueWrapper> operator[](const JSONCPP_STRING& key) const = 0;
    virtual std::unique_ptr<IJsonValueWrapper> operator[](Json::ArrayIndex index) const = 0;
    virtual Json::ArrayIndex size() const = 0;
    virtual bool isArray() const = 0;
    virtual bool isString() const = 0;
    virtual bool isUInt() const = 0;
    virtual JSONCPP_STRING asString() const = 0;
    virtual unsigned int asUInt() const = 0;
};

class IJsonCppWrapper
{
public:
    virtual ~IJsonCppWrapper() = default;
    virtual bool parseFromStream(Json::CharReader::Factory const &, std::istream &, std::unique_ptr<IJsonValueWrapper> &root, JSONCPP_STRING *errs) = 0;
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_IJSON_CPP_WRAPPER_H_
