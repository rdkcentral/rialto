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

#ifndef RIALTO_SERVERMANAGER_HTTP_REQUEST_H_
#define RIALTO_SERVERMANAGER_HTTP_REQUEST_H_

#include <mongoose.h>
#include <string>
#include <vector>

namespace rialto::servermanager
{
class HttpRequest
{
public:
    HttpRequest(mg_connection *conn, const mg_request_info *request_info);
    ~HttpRequest() = default;

    std::string getMethod() const;
    std::string getCommand() const;
    std::vector<std::string> getParams() const;

    void reply(const std::string &message) const;

private:
    mg_connection *m_connection;
    std::string m_method;
    std::string m_command;
    std::vector<std::string> m_params;
};
} // namespace rialto::servermanager

#endif // RIALTO_SERVERMANAGER_HTTP_REQUEST_H_
