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

#ifndef FIREBOLT_RIALTO_CLIENT_EGL_LIBRARY_H_
#define FIREBOLT_RIALTO_CLIENT_EGL_LIBRARY_H_

#include <memory>
#include <mutex>

namespace firebolt::rialto::client
{
/**
 * @brief Loads libEGL at run time for GL video frame capture, so libRialtoClient has no link-time EGL dependency.
 */
class IEglLibrary
{
public:
    IEglLibrary() = default;
    virtual ~IEglLibrary() = default;

    IEglLibrary(const IEglLibrary &) = delete;
    IEglLibrary &operator=(const IEglLibrary &) = delete;
    IEglLibrary(IEglLibrary &&) = delete;
    IEglLibrary &operator=(IEglLibrary &&) = delete;

    /** The process-wide EGL loader. */
    static std::shared_ptr<IEglLibrary> instance();

    /**
     * @brief Loads libEGL.so.1 on first call.
     *
     * @retval false if it cannot be loaded; later calls return the same result.
     */
    virtual bool load() = 0;
};

class EglLibrary : public IEglLibrary
{
public:
    static constexpr const char *kLibraryName{"libEGL.so.1"};

    EglLibrary() = default;
    ~EglLibrary() override;

    bool load() override;

private:
    std::mutex m_mutex;
    bool m_loadAttempted{false};
    void *m_handle{nullptr};
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_EGL_LIBRARY_H_
