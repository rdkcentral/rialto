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

#include "HttpRequest.h"
#include <cstring>

namespace
{
std::vector<std::string> splitUri(std::string uri)
{
    std::vector<std::string> result;
    size_t pos = 0;
    while ((pos = uri.find("/")) != std::string::npos)
    {
        const std::string token = uri.substr(0, pos);
        if (!token.empty())
        {
            result.push_back(std::move(token));
        }
        uri.erase(0, pos + 1);
    }
    if (!uri.empty())
    {
        result.push_back(std::move(uri));
    }
    return result;
}
} // namespace

namespace rialto::servermanager
{
HttpRequest::HttpRequest(mg_connection *conn, const mg_request_info *request_info)
    : m_connection{conn}, m_method{request_info->request_method},
      m_postData{request_info->post_data ? request_info->post_data : ""}
{
    auto uri = splitUri(request_info->uri);
    if (uri.size() > 0)
    {
        m_command = uri[0];
    }
    if (uri.size() > 1)
    {
        m_params = std::vector<std::string>(uri.begin() + 1, uri.end());
    }
}

std::string HttpRequest::getMethod() const
{
    return m_method;
}

std::string HttpRequest::getCommand() const
{
    return m_command;
}

std::string HttpRequest::getPostData() const
{
    return m_postData;
}

std::vector<std::string> HttpRequest::getParams() const
{
    return m_params;
}

void HttpRequest::reply(const std::string &message) const
{
    mg_write(m_connection, message.data(), message.size());
}
} // namespace rialto::servermanager
