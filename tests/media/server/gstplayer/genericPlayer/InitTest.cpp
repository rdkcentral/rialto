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

#include "GstWrapperMock.h"
#include "GstInit.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::StrictMock;

class RialtoServerInitGstPlayerTest : public ::testing::Test
{
protected:
    int argc = 2;
    char *argv[3] = {"a", "b", nullptr};

    virtual void SetUp() {}

    virtual void TearDown() {}
};

/**
 * Test the initialisation of gstreamer.
 */
TEST_F(RialtoServerInitGstPlayerTest, Init)
{
    bool status = false;
    EXPECT_NO_THROW(status = gstInitalise(argc, static_cast<char **>(argv)););
    EXPECT_EQ(status, true);
}
