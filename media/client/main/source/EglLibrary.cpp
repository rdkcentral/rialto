/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "EglLibrary.h"
#include "RialtoClientLogging.h"

#include <dlfcn.h>

namespace firebolt::rialto::client
{
std::shared_ptr<IEglLibrary> IEglLibrary::instance()
{
    static std::shared_ptr<IEglLibrary> library{std::make_shared<EglLibrary>()};
    return library;
}

EglLibrary::~EglLibrary()
{
    if (m_handle)
    {
        dlclose(m_handle);
    }
}

bool EglLibrary::load()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_loadAttempted)
    {
        m_loadAttempted = true;
        m_handle = dlopen(kLibraryName, RTLD_NOW | RTLD_LOCAL);
        if (!m_handle)
        {
            const char *kError = dlerror();
            RIALTO_CLIENT_LOG_WARN("Cannot load %s: %s", kLibraryName, kError ? kError : "unknown error");
        }
    }
    return m_handle != nullptr;
}
} // namespace firebolt::rialto::client
