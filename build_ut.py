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
    outputFile = getOutputFile(args['file'])

    # Build and run tests
    buildDefines = ["-DCMAKE_BUILD_FLAG=UnitTests", "-DRIALTO_ENABLE_DECRYPT_BUFFER=1", "-DRIALTO_ENABLE_CONFIG_FILE=1", "-DRIALTO_BUILD_TYPE=Debug"]
    buildAndRunGTests(args, outputFile, buildDefines, suitesToRun)

    for key in suites:
        executeCmd = []

        # Valgrind command must come before the test executable
        if valgrind == True:
            executeCmd.extend(AddValgrind(key, resultsFile != None, xmlFile != None))

        # Add test executable + any optionals
        executeCmd.append("." + suites[key]["path"] + suites[key]["suite"])
        if  doListTests == True:
            executeCmd.append('--gtest_list_tests')
        elif gtestFilter != None:
            executeCmd.append('--gtest_filter=' + gtestFilter)

        # Dont output as xml for valgrind
        # Googletest xml output flags an UninitCondition error when converting to xml
        if xmlFile != None and valgrind == False:
            executeCmd.append('--gtest_output=xml:' + key + "_" + xmlFile)

        # Run the command
        if resultsFile != None:
            status = runcmd(executeCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
        else:
            status = runcmd(executeCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)

        # Check for error code
        if status.returncode == valgrindErrorCode:
            print("Valgrind failed for suite '" + key +"'")
            hasFailed = True
        elif status.returncode != 0:
            print("Command failed for suite '" + key +"' with error code '" + str(status.returncode) + "'")
            hasFailed = True

    if hasFailed:
        raise Exception( f"runTests has failed, exiting" )
    elif coverage:
        generateCoverageReport(outputDir, resultsFile, suites)

# Returns the valgrind command arguments
def AddValgrind(suite, outputToFile, outputToXml):
    executeCmd = ["valgrind", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes", "--verbose", "--error-exitcode=" + str(valgrindErrorCode)]

    # Xml redirects the output to xml, cannot output to logfile at the same time
    if outputToXml:
        executeCmd.extend(["--xml=yes", "--xml-file=" + suite + "_" + valgrindOutput + ".xml"])
    elif outputToFile:
        executeCmd.append("--log-file=" + suite + "_" + valgrindOutput + ".log")

    # Some shared libraries throw errors for 'possible leaks', these usually occur when
    # incrementing and decrementing pointers as valgrind thinks you have lost the start of
    # the buffer. Supress these errors.
    #executeCmd.append("--gen-suppressions=all")
    filePath = os.path.realpath(os.path.dirname(__file__))
    executeCmd.append("--suppressions=" + filePath + "/" + valgrindIgnore)

    return executeCmd

def generateCoverageReport(outputDir, resultsFile, suites):
    lcovCommon = [];
    lcovCommon.extend(["--exclude", "/usr/*"]);
    lcovCommon.extend(["--exclude", "*build/*", "--exclude", "*tests/*", "--exclude", "*wrappers/*", "--filter", "brace,function,trivial"])
    lcovCommon.extend(["--parallel",  str(multiprocessing.cpu_count())])
    
    # the following line tells lcov to ignore any errors caused by include/exclude/erase/omit/substitute pattern which did not match any file pathnames
    lcovCommon.extend(["--ignore-errors", "unused"]);
    
    lcovCommon.extend(["--exclude", "*Wrapper.cpp", "--exclude", "LinuxWrapper.h",  "--exclude", "JsonCppWrapperFactory.cpp", "--exclude", "JsonCppWrapperFactory.h", "--exclude", "JsonCppWrapper.h"])
    lcovCommon.extend(["--exclude", "GstProtectionMetadataWrapper.h", "--exclude", "GstProtectionMetadataWrapperFactory.h", "--exclude", "GstWrapper.h", "--exclude", "GlibWrapper.h"])
    
    lcovBaseCmd = ["lcov", "-c", "-i", "-d", ".", "--output-file", "coverage_base.info"]
    lcovBaseCmd.extend(lcovCommon);
    
    if resultsFile:
        lcovBaseStatus = runcmd(lcovBaseCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovBaseStatus = runcmd(lcovBaseCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovBaseStatus:
        return False
    lcovTestCmd = ["lcov", "-c", "-d", ".", "--output-file", "coverage_test.info"] 
    lcovTestCmd.extend(lcovCommon);

    if resultsFile:
        lcovTestStatus = runcmd(lcovTestCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovTestStatus = runcmd(lcovTestCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovTestStatus:
        return False
    lcovCombineCmd = ["lcov", "-a", "coverage_base.info", "-a", "coverage_test.info", "-o", "coverage.info", "--filter",
                      "brace,function,trivial"]
    lcovCombineCmd.extend(["--ignore-errors", "empty"]);
    if resultsFile:
        lcovCombineStatus = runcmd(lcovCombineCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        lcovCombineStatus = runcmd(lcovCombineCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    if not lcovCombineStatus:
        return False
    genHtmlCmd = ["genhtml", "coverage.info", "--output-directory", "gh_pages/coverage_report", "--filter", "brace,function,trivial"]
    genHtmlCmd.extend(["--ignore-errors", "empty"]);
    if resultsFile:
        genHtmlStatus = runcmd(genHtmlCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        genHtmlStatus = runcmd(genHtmlCmd, cwd=os.getcwd() + '/' + outputDir, stderr=subprocess.STDOUT)
    genStatsCmd = ["lcov", "--summary", "coverage.info", "--filter", "brace,function,trivial"]
    genStatsCmd.extend(["--ignore-errors", "empty"]);
    statsFile = open(os.getcwd() + '/' + outputDir + '/' + "coverage_statistics.txt", "w")
    if statsFile:
        genStatsStatus = runcmd(genStatsCmd, cwd=os.getcwd() + '/' + outputDir, stdout=statsFile, stderr=subprocess.STDOUT)
    else:
        genStatsStatus = False
    statsFile.close()
    return genHtmlStatus and genStatsStatus


if __name__ == "__main__":
    main()
