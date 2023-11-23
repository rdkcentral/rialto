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

# Build script for running rialto googletests

import subprocess
import os
import multiprocessing
import sys
from .utils import runcmd

def generateCoverageReport(outputDir, resultsFile, suites, excludeFiles):
    lcovCommon = []
    lcovCommon.extend(["--exclude", "/usr/*"])
    lcovCommon.extend(["--exclude", "*build/*", "--exclude", "*tests/*", "--filter", "brace,function,trivial"])
    lcovCommon.extend(["--parallel",  str(multiprocessing.cpu_count())])
    
    # the following line tells lcov to ignore any errors caused by include/exclude/erase/omit/substitute pattern which did not match any file pathnames
    lcovCommon.extend(["--ignore-errors", "unused"])
    
    for file in excludeFiles:
        lcovCommon.extend(["--exclude", file])
    
    lcovBaseCmd = ["lcov", "-c", "-i", "-d", ".", "--output-file", "coverage_base.info"]
    lcovBaseCmd.extend(lcovCommon)
    
    if resultsFile:
        lcovBaseStatus = runcmd(lcovBaseCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovBaseStatus = runcmd(lcovBaseCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovBaseStatus:
        return False
    lcovTestCmd = ["lcov", "-c", "-d", ".", "--output-file", "coverage_test.info"] 
    lcovTestCmd.extend(lcovCommon)

    if resultsFile:
        lcovTestStatus = runcmd(lcovTestCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovTestStatus = runcmd(lcovTestCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovTestStatus:
        return False
    lcovCombineCmd = ["lcov", "-a", "coverage_base.info", "-a", "coverage_test.info", "-o", "coverage.info", "--filter",
                      "brace,function,trivial"]
    lcovCombineCmd.extend(["--ignore-errors", "empty"])
    if resultsFile:
        lcovCombineStatus = runcmd(lcovCombineCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovCombineStatus = runcmd(lcovCombineCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovCombineStatus:
        return False
    genHtmlCmd = ["genhtml", "coverage.info", "--output-directory", "gh_pages/coverage_report", "--filter", "brace,function,trivial"]
    genHtmlCmd.extend(["--ignore-errors", "empty"])
    if resultsFile:
        genHtmlStatus = runcmd(genHtmlCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        genHtmlStatus = runcmd(genHtmlCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    genStatsCmd = ["lcov", "--summary", "coverage.info", "--filter", "brace,function,trivial"]
    genStatsCmd.extend(["--ignore-errors", "empty"])
    statsFile = open(os.getcwd() + '/' + outputDir + '/' + "coverage_statistics.txt", "w")
    if statsFile:
        genStatsStatus = runcmd(genStatsCmd, cwd=os.getcwd() + '/' + outputDir, stdout=statsFile, stderr=subprocess.STDOUT)
    else:
        genStatsStatus = False
    statsFile.close()
    return genHtmlStatus and genStatsStatus

def generateSpecificCoverageStats(outputDir, resultsFile, includeFiles, statsFileName):
    genStatsCmd = ["lcov", "--summary", "coverage.info", "--filter", "brace,function,trivial"]
    genStatsCmd.extend(["--ignore-errors", "empty"])
    
    for file in includeFiles:
        genStatsCmd.extend(["--include", file])
    
    statsFile = open(os.getcwd() + '/' + outputDir + '/' + statsFileName + '.txt', "w")
    if statsFile:
        genStatsStatus = runcmd(genStatsCmd, cwd=os.getcwd() + '/' + outputDir, stdout=statsFile, stderr=subprocess.STDOUT)
    else:
        genStatsStatus = False
    statsFile.close()
    return genStatsStatus
