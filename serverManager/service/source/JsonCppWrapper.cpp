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

#include "JsonCppWrapper.h"

namespace rialto::servermanager::service
{
// bool JsonValueWrapper::isMember(const JSONCPP_STRING &key) const
// {
//     return m_value.isMember(key);
// }

// std::unique_ptr<IJsonValueWrapper> JsonValueWrapper::operator[](const JSONCPP_STRING &key) const
// {
//     return std::make_unique<JsonValueWrapper>(m_value[key]);
// }

// Json::ArrayIndex JsonValueWrapper::size() const
// {
//     return m_value.size();
// }


bool JsonCppWrapper::parseFromStream(Json::CharReader::Factory const &factory, std::istream &file, std::unique_ptr<IJsonValueWrapper> &root, JSONCPP_STRING *errs)
{
    Json::Value value;

    if (!Json::parseFromStream(factory, file, &value, errs))
    {
        return false;
    }

    root = std::make_unique<JsonValueWrapper<Json::Value>>(value);

    return true;
}

} // namespace rialto::servermanager::service
