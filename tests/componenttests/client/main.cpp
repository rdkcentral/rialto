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

#include "RialtoLogging.h"
#include <google/protobuf/stubs/common.h>
#include <gtest/gtest.h>

class ProtobufCleaner : public testing::Environment
{
public:
    virtual ~ProtobufCleaner() {}
    virtual void TearDown() { google::protobuf::ShutdownProtobufLibrary(); }
};

int main(int argc, char **argv) // NOLINT(build/filename_format)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ProtobufCleaner);

    RIALTO_DEBUG_LEVEL allLogs = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR |
                                                    RIALTO_DEBUG_LEVEL_WARNING | RIALTO_DEBUG_LEVEL_MILESTONE |
                                                    RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_DEBUG);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_DEFAULT, allLogs);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_CLIENT, allLogs);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER, allLogs);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_IPC, allLogs);
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_COMMON, allLogs);

    return RUN_ALL_TESTS();
}
