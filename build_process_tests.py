#!/usr/bin/env python3

# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Sky UK
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

import argparse
from scripts.gtest.build_and_run_tests import getGenericArguments, buildAndRunGTests
from scripts.gtest.utils import getSuitesToRun, getOutputFile

suiteInfo = {
    "process": {"suite": "RialtoProcessTests", "path": "/tests/processtests/"},
}

if __name__ == "__main__":
    argParser = argparse.ArgumentParser(description="Run Rialto process tests")
    getGenericArguments(argParser, suiteInfo)
    args = vars(argParser.parse_args())
    suitesToRun = getSuitesToRun(args["suites"], suiteInfo)
    outputFile = getOutputFile(args["file"], args["clean"])
    buildDefines = ["-DCMAKE_BUILD_FLAG=ProcessTests", "-DRIALTO_ENABLE_CONFIG_FILE=1", "-DRIALTO_BUILD_TYPE=Debug"]
    buildAndRunGTests(args, outputFile, buildDefines, suitesToRun)
