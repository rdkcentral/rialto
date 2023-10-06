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

#ifndef RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_H_
#define RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_H_

#include "IJsonCppWrapper.h"
#include <memory>

namespace rialto::servermanager::service
{

template <typename T> class JsonValueWrapper : public IJsonValueWrapper
{
public:
    explicit JsonValueWrapper(const Json::Value &value) : m_value(value) {}
    bool isMember(const JSONCPP_STRING &key) const override { return m_value.isMember(key); }
    std::shared_ptr<IJsonValueWrapper> at(const JSONCPP_STRING &key) const override
    {
        return std::make_unique<JsonValueWrapper<const Json::Value &>>(m_value[key]);
    }

    std::shared_ptr<IJsonValueWrapper> at(Json::ArrayIndex index) const override
    {
        return std::make_unique<JsonValueWrapper<const Json::Value &>>(m_value[index]);
    }

    Json::ArrayIndex size() const override { return m_value.size(); }
    bool isArray() const override { return m_value.isArray(); }
    bool isString() const override { return m_value.isString(); }
    bool isUInt() const override { return m_value.isUInt(); }
    JSONCPP_STRING asString() const override { return m_value.asString(); }
    unsigned int asUInt() const override { return m_value.asUInt(); }

private:
    /*const*/ T m_value;
};

class JsonCppWrapper : public IJsonCppWrapper
{
public:
    bool parseFromStream(Json::CharReader::Factory const &, std::istream &, std::shared_ptr<IJsonValueWrapper> &root,
                         JSONCPP_STRING *errs);
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_JSON_CPP_WRAPPER_H_
