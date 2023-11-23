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

# Utility functions for test scripts

import subprocess
import sys

# Default variables
resultOutput = "gtest_result"
valgrindErrorCode = 101

# Get a dict of the suites to run
# Returns all possible suites if none requested
def getSuitesToRun (suitesRequested, allSuites):
    suitesToRun = {}
    # Get a dict of information of the suites to run
    if suitesRequested != None:
        for suite in suitesRequested:
            if allSuites[suite] != None:
                suitesToRun[suite] = allSuites[suite]
            else:
                print("Could not find suite: " + suite)
    else:
        suitesToRun = allSuites
    return suitesToRun

def getOutputFile(argInput):
    # Get output file name if any
    if argInput != None:
        if argInput == "":
            f = open(resultOutput + ".log", "w")
        else:
            f = open(argInput, "w")
    else:
        f = None

    return f

# Executes a command
def runcmd(*args, **kwargs):
    status = subprocess.run(*args, **kwargs)
    if status.returncode == 0 or status.returncode == valgrindErrorCode:
        return status
    else:
        args = ' '.join(status.args) if type(status.args) == list else status.args
        sys.exit(f'Command: "{args}" returned with {status.returncode} error code!')


# Gets the list of keys as a csv string
def strOfKeys (directory):
    str = ""
    for key in directory :
        if str != "":
            str += ", "
        str += key
    return str

def getDefaultResultsOutputFileName():
    return resultOutput

def getvalgrindErrorCode():
    return valgrindErrorCode
