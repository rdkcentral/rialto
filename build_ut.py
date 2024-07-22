#!/usr/bin/env python3

# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Sky UK
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Entry script for running rialto unittests

import argparse
from scripts.gtest.build_and_run_tests import getGenericArguments, buildAndRunGTests
from scripts.gtest.utils import getSuitesToRun, getOutputFile
from scripts.gtest.generate_coverage import generateCoverageReport

# Rialto Component Tests & Paths
# {Component Name : {Test Suite, Test Path}}
suiteInfo = {
    "servermain" : {"suite" : "RialtoServerMainUnitTests", "path" : "/tests/unittests/media/server/main/"},
    "servergstplayer" : {"suite" : "RialtoServerGstPlayerUnitTests", "path" : "/tests/unittests/media/server/gstplayer/"},
    "serveripc" : {"suite" : "RialtoServerIpcUnitTests", "path" : "/tests/unittests/media/server/ipc/"},
    "serverservice" : {"suite" : "RialtoServerServiceUnitTests", "path" : "/tests/unittests/media/server/service/"},
    "client" : {"suite" : "RialtoClientUnitTests", "path" : "/tests/unittests/media/client/main/"},
    "clientipc" : {"suite" : "RialtoClientIpcUnitTests", "path" : "/tests/unittests/media/client/ipc/"},
    "playercommon" : {"suite" : "RialtoPlayerCommonUnitTests", "path" : "/tests/unittests/media/common/"},
    "common" : {"suite" : "RialtoCommonUnitTests", "path" : "/tests/unittests/common/unittests/"},
    "logging" : {"suite" : "RialtoLoggingUnitTests", "path" : "/tests/unittests/logging/"},
    "manager" : {"suite" : "RialtoServerManagerUnitTests", "path" : "/tests/unittests/serverManager/"},
    "ipc" : {"suite" : "RialtoIpcUnitTests", "path" : "/tests/unittests/ipc/"},
}

if __name__ == "__main__":
    # Parse arguments
    argParser = argparse.ArgumentParser(description='Run the rialto Unittests.', formatter_class=argparse.RawTextHelpFormatter)
    getGenericArguments(argParser, suiteInfo)
    args = vars(argParser.parse_args())

    # Get suites
    suitesToRun = getSuitesToRun(args['suites'], suiteInfo)

    # Get output file
    outputFile = getOutputFile(args['file'], args['clean'])

    # Build and run tests
    buildDefines = ["-DCMAKE_BUILD_FLAG=UnitTests", "-DRIALTO_ENABLE_DECRYPT_BUFFER=1", "-DRIALTO_ENABLE_CONFIG_FILE=1", "-DRIALTO_BUILD_TYPE=Debug"]
    buildAndRunGTests(args, outputFile, buildDefines, suitesToRun)

    if args['coverage'] == True:
        generateCoverageReport(args['output'], outputFile, suitesToRun)
