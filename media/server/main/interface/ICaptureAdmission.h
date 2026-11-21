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

#ifndef FIREBOLT_RIALTO_SERVER_I_CAPTURE_ADMISSION_H_
#define FIREBOLT_RIALTO_SERVER_I_CAPTURE_ADMISSION_H_

#include "ICaptureHalLibrary.h"

#include <cstdint>
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief A HAL session configured up to READY by the capture commit, with the pipeline and capture it belongs to.
 */
struct CaptureSessionHandle
{
    int sessionId{-1};                   /**< The pipeline session. */
    std::uint32_t captureId{0};          /**< The attached capture. */
    wrappers::CaptureSessionPtr session; /**< The HAL session, whose sink has reached configured READY. */
    std::uint32_t poolId{0};             /**< Set by admission: the admitted pool's ID. */
    capture::CapturePool pool{};         /**< Set by admission: the admitted pool. */
};

/**
 * @brief The hand-over from the capture commit to pool admission (proposal 3.3 step 7 onwards).
 *
 * Admission queries capabilities, chooses the slot count, creates the pool and records the commit. It is
 * implemented by R8; until then UnavailableCaptureAdmission refuses every session.
 */
class ICaptureAdmission
{
public:
    ICaptureAdmission() = default;
    virtual ~ICaptureAdmission() = default;

    ICaptureAdmission(const ICaptureAdmission &) = delete;
    ICaptureAdmission &operator=(const ICaptureAdmission &) = delete;
    ICaptureAdmission(ICaptureAdmission &&) = delete;
    ICaptureAdmission &operator=(ICaptureAdmission &&) = delete;

    /**
     * @brief Admits a configured READY session. Called on the server main thread.
     *
     * @retval false if the session cannot be admitted; the commit is then rolled back and the session deleted.
     */
    virtual bool admit(CaptureSessionHandle &handle) = 0;
};

/**
 * @brief Creates the admission each new pipeline uses.
 */
class ICaptureAdmissionFactory
{
public:
    ICaptureAdmissionFactory() = default;
    virtual ~ICaptureAdmissionFactory() = default;

    /**
     * @brief The factory used for new pipelines. Pool admission (R8) installs its own; component tests replace it.
     *        Until then it creates UnavailableCaptureAdmission.
     */
    static std::shared_ptr<ICaptureAdmissionFactory> &instance();

    virtual std::shared_ptr<ICaptureAdmission> createCaptureAdmission() const = 0;
};

/**
 * @brief The admission used until pool admission (R8) exists: it refuses every session, so capture commits fail
 *        cleanly and are rolled back.
 */
class UnavailableCaptureAdmission : public ICaptureAdmission
{
public:
    bool admit(CaptureSessionHandle &handle) override;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_CAPTURE_ADMISSION_H_
