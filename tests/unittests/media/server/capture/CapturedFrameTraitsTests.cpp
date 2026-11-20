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

#include "ICaptureSession.h"

#include <gtest/gtest.h>

#include <type_traits>

using firebolt::rialto::capture::CapturedFrame;
using firebolt::rialto::capture::ICaptureSession;

// SoC capture spec 5.1: a CapturedFrame is neither copyable nor movable, so a record and its release
// obligation can never be duplicated or silently moved. These checks fail the build if that changes.
static_assert(!std::is_copy_constructible_v<CapturedFrame>, "CapturedFrame must not be copy-constructible");
static_assert(!std::is_copy_assignable_v<CapturedFrame>, "CapturedFrame must not be copy-assignable");
static_assert(!std::is_move_constructible_v<CapturedFrame>, "CapturedFrame must not be move-constructible");
static_assert(!std::is_move_assignable_v<CapturedFrame>, "CapturedFrame must not be move-assignable");
static_assert(std::is_default_constructible_v<CapturedFrame>, "CapturedFrame must be default-constructible");

// Sessions are deleted through the interface (spec 4.6), so the destructor must be virtual.
static_assert(std::has_virtual_destructor_v<ICaptureSession>, "ICaptureSession needs a virtual destructor");
static_assert(std::is_abstract_v<ICaptureSession>, "ICaptureSession must be an interface");

TEST(CapturedFrameTraitsTest, TraitsAreCheckedAtCompileTime)
{
    SUCCEED();
}
