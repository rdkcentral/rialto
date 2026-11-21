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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_CAPTURE_HAL_LIBRARY_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_CAPTURE_HAL_LIBRARY_H_

#include "ICaptureSession.h"

#include <functional>
#include <memory>

namespace firebolt::rialto::wrappers
{
class ICaptureHalLibrary;

/**
 * @brief A capture HAL session owned by Rialto.
 *
 * The deleter destroys the session through ICaptureSession's virtual destructor and keeps the vendor library loaded
 * until then.
 */
using CaptureSessionPtr = std::unique_ptr<capture::ICaptureSession, std::function<void(capture::ICaptureSession *)>>;

/**
 * @brief ICaptureHalLibraryFactory factory class, returns the process-wide ICaptureHalLibrary.
 */
class ICaptureHalLibraryFactory
{
public:
    ICaptureHalLibraryFactory() = default;
    virtual ~ICaptureHalLibraryFactory() = default;

    /**
     * @brief Gets the ICaptureHalLibraryFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<ICaptureHalLibraryFactory> createFactory();

    /**
     * @brief Gets the capture HAL library.
     *
     * @retval the library, or null on error.
     */
    virtual std::shared_ptr<ICaptureHalLibrary> getCaptureHalLibrary() const = 0;
};

/**
 * @brief Loads the vendor capture HAL library and creates sessions from it (SoC capture spec 8.1).
 *
 * The library is lib_capture_hal.so unless RIALTO_CAPTURE_HAL_LIBRARY names another one. It is loaded on first use
 * and stays loaded until this object and every session created from it are gone.
 */
class ICaptureHalLibrary
{
public:
    ICaptureHalLibrary() = default;
    virtual ~ICaptureHalLibrary() = default;

    ICaptureHalLibrary(const ICaptureHalLibrary &) = delete;
    ICaptureHalLibrary &operator=(const ICaptureHalLibrary &) = delete;
    ICaptureHalLibrary(ICaptureHalLibrary &&) = delete;
    ICaptureHalLibrary &operator=(ICaptureHalLibrary &&) = delete;

    /**
     * @brief Checks whether capture is available, loading the library on first use.
     *
     * @retval false if the library or its createCaptureSession factory cannot be found.
     */
    virtual bool isAvailable() = 0;

    /**
     * @brief Creates a new capture session.
     *
     * @retval the session, or null if capture is unavailable or the factory returned no session.
     */
    virtual CaptureSessionPtr createSession() = 0;
};

} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_CAPTURE_HAL_LIBRARY_H_
