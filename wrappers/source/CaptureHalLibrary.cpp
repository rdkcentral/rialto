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

#include "CaptureHalLibrary.h"
#include "RialtoLogging.h"

#include <dlfcn.h>

#include <cstdlib>

namespace firebolt::rialto::wrappers
{
std::shared_ptr<ICaptureHalLibrary> CaptureHalLibraryFactory::getCaptureHalLibrary() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    std::shared_ptr<ICaptureHalLibrary> library = m_library.lock();
    if (!library)
    {
        library = std::make_shared<CaptureHalLibrary>();
        m_library = library;
    }
    return library;
}

bool CaptureHalLibrary::isAvailable()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return loadLocked();
}

CaptureSessionPtr CaptureHalLibrary::createSession()
{
    capture::CreateCaptureSessionFunction factory{nullptr};
    std::shared_ptr<void> handle;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!loadLocked())
        {
            return CaptureSessionPtr{nullptr, [](capture::ICaptureSession *) {}};
        }
        factory = m_factory;
        handle = m_handle;
    }

    capture::ICaptureSession *session{nullptr};
    try
    {
        session = factory();
    }
    catch (...)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "%s threw an exception", kFactorySymbol);
        session = nullptr;
    }

    if (!session)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "%s returned no capture session", kFactorySymbol);
        return CaptureSessionPtr{nullptr, [](capture::ICaptureSession *) {}};
    }

    // The deleter owns a reference to the library handle, so the library stays loaded until after the vendor
    // destructor has run, even if this loader is destroyed first.
    return CaptureSessionPtr{session, [handle](capture::ICaptureSession *toDelete) { delete toDelete; }};
}

bool CaptureHalLibrary::loadLocked()
{
    if (m_loadAttempted)
    {
        return m_factory != nullptr;
    }
    m_loadAttempted = true;

    const char *kOverride = std::getenv(kLibraryNameEnv);
    const char *kName = (kOverride && kOverride[0] != '\0') ? kOverride : kDefaultLibraryName;

    void *handle = dlopen(kName, RTLD_NOW | RTLD_LOCAL);
    if (!handle)
    {
        const char *kError = dlerror();
        RIALTO_LOG_MIL(RIALTO_COMPONENT_SERVER, "Video frame capture unavailable: cannot load %s (%s)", kName,
                       kError ? kError : "unknown error");
        return false;
    }

    dlerror(); // clear any stale error before dlsym
    void *symbol = dlsym(handle, kFactorySymbol);
    if (!symbol)
    {
        const char *kError = dlerror();
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "Video frame capture unavailable: %s has no %s (%s)", kName,
                         kFactorySymbol, kError ? kError : "unknown error");
        dlclose(handle);
        return false;
    }

    m_handle = std::shared_ptr<void>(handle, [](void *toClose) { dlclose(toClose); });
    m_factory = reinterpret_cast<capture::CreateCaptureSessionFunction>(symbol);
    RIALTO_LOG_MIL(RIALTO_COMPONENT_SERVER, "Loaded video frame capture library %s", kName);
    return true;
}

} // namespace firebolt::rialto::wrappers
