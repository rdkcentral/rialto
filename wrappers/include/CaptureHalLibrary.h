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

#ifndef FIREBOLT_RIALTO_WRAPPERS_CAPTURE_HAL_LIBRARY_H_
#define FIREBOLT_RIALTO_WRAPPERS_CAPTURE_HAL_LIBRARY_H_

#include "ICaptureHalLibrary.h"

#include <memory>
#include <mutex>
#include <string>

namespace firebolt::rialto::wrappers
{
/**
 * @brief Returns the process-wide CaptureHalLibrary while any user holds it, creating a new one otherwise.
 */
class CaptureHalLibraryFactory : public ICaptureHalLibraryFactory
{
public:
    std::shared_ptr<ICaptureHalLibrary> getCaptureHalLibrary() const override;

private:
    mutable std::mutex m_mutex;
    mutable std::weak_ptr<ICaptureHalLibrary> m_library;
};

/**
 * @brief dlopen-based loader for the vendor capture HAL library.
 *
 * Every session holds a reference to the library handle, so the library is unloaded only after this object and
 * every session created from it have been destroyed (SoC capture spec 4.6).
 */
class CaptureHalLibrary : public ICaptureHalLibrary
{
public:
    /** Default library name (SoC capture spec 8.1). */
    static constexpr const char *kDefaultLibraryName{"lib_capture_hal.so"};
    /** Environment variable that overrides the library name. */
    static constexpr const char *kLibraryNameEnv{"RIALTO_CAPTURE_HAL_LIBRARY"};
    /** Exported factory symbol. */
    static constexpr const char *kFactorySymbol{"createCaptureSession"};

    CaptureHalLibrary() = default;
    ~CaptureHalLibrary() override = default;

    bool isAvailable() override;
    CaptureSessionPtr createSession() override;

private:
    /** Loads the library once; later calls return the first result. Must be called with m_mutex held. */
    bool loadLocked();

    std::mutex m_mutex;
    bool m_loadAttempted{false};
    std::shared_ptr<void> m_handle;
    capture::CreateCaptureSessionFunction m_factory{nullptr};
};

} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_CAPTURE_HAL_LIBRARY_H_
