#!/usr/bin/env python3

# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Sky UK
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

# Entry script for running rialto componenttests

import argparse
import os
from scripts.gtest.build_and_run_tests import getGenericArguments, buildAndRunGTests
from scripts.gtest.utils import getSuitesToRun, getOutputFile
from scripts.gtest.generate_coverage import generateCoverageReport, generateSpecificCoverageStats

# Rialto Component Tests & Paths
# {Component Name : {Test Suite, Test Path}}
suiteInfo = {
    "client" : {"suite" : "RialtoClientComponentTests", "path" : "/tests/componenttests/client/"},
    "server" : {"suite" : "RialtoServerComponentTests", "path" : "/tests/componenttests/server/tests/"},
}

if __name__ == "__main__":
    # Parse arguments
    argParser = argparse.ArgumentParser(description='Run the rialto Component Tests.', formatter_class=argparse.RawTextHelpFormatter)
    getGenericArguments(argParser, suiteInfo)
    args = vars(argParser.parse_args())

    # Get suites
    suitesToRun = getSuitesToRun(args['suites'], suiteInfo)

    # Get output file
    outputFile = getOutputFile(args['file'], args['clean'])

    # Build and run tests
    buildDefines = ["-DCMAKE_BUILD_FLAG=ComponentTests", "-DRIALTO_ENABLE_CONFIG_FILE=1", "-DRIALTO_BUILD_TYPE=Debug"]
    buildAndRunGTests(args, outputFile, buildDefines, suitesToRun)

    # Generate coverage
    if args['coverage'] == True:
        generateCoverageReport(os.getcwd(), args['output'], outputFile)

        # Also generate coverage stats for public interfaces source
        files = ["*/main/source/*"]
        generateSpecificCoverageStats(os.getcwd(), args['output'], outputFile, files, "coverage_statistics_public_apis")
