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

# Functions for building and running rialto googletests

import subprocess
import os
import argparse
import multiprocessing
from .utils import *

# Default variables
valgrindOutput = "valgrind_report"
valgrindIgnore = "rialto.supp"

# Get the arguments supported by the googletest script
def getGenericArguments(argParser, suiteInfo):
    # Get arguments
    argParser.add_argument("-o", "--output", default="build",
                        help="Location to write the build files to (default 'build').")
    argParser.add_argument("-f", "--file", nargs='?', const="",
                        help="Write the build and test output to a file (default '" + getDefaultResultsOutputFileName() + ".log') \n" \
                             + "Valgrind output also written to file, default file name only \n" \
                             + "'*suite_name*_" + valgrindOutput + ".txt' (if -xml not specified).")
    argParser.add_argument("-xml", "--xml", nargs='?', const="",
                        help="Convert the test results to xml (default '*suite_name*_" + getDefaultResultsOutputFileName() + ".xml') \n" \
                             + "Valgrind output also converted to xml, default file name only \n" \
                             + "'*suite_name*_" + valgrindOutput + ".xml'.")
    argParser.add_argument("-s", "--suites", nargs='*',
                        help="The test suites to run (default all tests). \n" \
                             + "Options: " + strOfKeys(suiteInfo))
    argParser.add_argument("-gf", "--googletestFilter",
                        help="The filter to apply when running the googletest. \n" \
                             + "Use filter option to run specific tests or groups of tests. \n" \
                             + "E.g. FooTest* - runs all tests prefixed with FooTest \n" \
                             + "FooTest.* - runs all FooTest test cases \n" \
                             + "-FooTest.* - runs all tests apart from the FooTest cases \n" \
                             + "FooTest.*:BarTest.* - runs all FooTest and BarTest cases")
    argParser.add_argument("-l", "--listTests", action='store_true', help="Lists the tests in the given suites")
    argParser.add_argument("-c", "--clean", action='store_true', help="Clean the directory before running")
    argParser.add_argument("-nb", "--noBuild", action='store_true', help="Do not build")
    argParser.add_argument("-nt", "--noTest", action='store_true', help="Do not test")
    argParser.add_argument("-val", "--valgrind", action='store_true', help="Run the googletest with valgrind \n" \
                             + "Output written to file if -f option specified '*suite_name*_" + valgrindOutput + ".log' \n" \
                             + "Output written to xml if -xml option specified '*suite_name*_" + valgrindOutput + ".xml' \n" \
                             + "Note: Valgrind can only write output to one source (log or xml). \n" \
                             + "Note: Requires version valgrind 3.17.0+ installed. \n")
    argParser.add_argument("-cov", "--coverage", action='store_true', help="Generates the full coverage report")

# Builds and runs googletests for the given suites
def buildAndRunGTests(args, f, buildDefines, suitesToRun):
    # Set env variable
    os.environ["RIALTO_SOCKET_PATH"] = "/tmp/rialto-0"
    # Set env variable to disable journald logging
    os.environ["RIALTO_CONSOLE_LOG"] = "1"

    # Clean if required
    if args['clean'] == True:
        executeCmd = ["rm", "-rf", args['output'], getDefaultResultsOutputFileName() + ".log", valgrindOutput + ".log"]
        runcmd(executeCmd, cwd=os.getcwd())

    # Get xml output file name if any
    if args['xml'] != None:
        if args['xml'] == "":
            xml = getDefaultResultsOutputFileName() + ".xml"
        else:
            xml = args['xml']
    else:
        xml = None

    # Build the test executables
    if args['noBuild'] == False:
        buildTargets(suitesToRun, buildDefines, args['output'], f, args['valgrind'], args['coverage'])

    # Run the tests with the optional settings
    if args['noTest'] == False:
        runTests(suitesToRun, args['listTests'], args['googletestFilter'], args['output'], f, xml, args['valgrind'],
                 args['coverage'])


# Build the target executables
def buildTargets (suites, buildDefines, outputDir, resultsFile, debug, coverage):
    # Run cmake
    cmakeCmd = ["cmake", "-B", outputDir]
    for define in buildDefines:
        cmakeCmd.append(define)

    # Coverage
    if coverage:
        cmakeCmd.append("-DCOVERAGE_ENABLED=1")
    runcmd(cmakeCmd, cwd=os.getcwd())

    # Make targets
    jarg = "-j" + str(multiprocessing.cpu_count())
    makeCmd = ["make", jarg]
    for key in suites:
        makeCmd.append(suites[key]["suite"])

    print(f"+ {' '.join(makeCmd)}")
    if resultsFile != None:
        runcmd(makeCmd, cwd=os.getcwd() + '/' + outputDir, stdout=resultsFile, stderr=subprocess.STDOUT)
    else:
        runcmd(makeCmd, cwd=os.getcwd() + '/' + outputDir )

# Run the googletests
def runTests (suites, doListTests, gtestFilter, outputDir, resultsFile, xmlFile, valgrind, coverage):
    hasFailed = False

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
        if status.returncode == getvalgrindErrorCode():
            print("Valgrind failed for suite '" + key +"'")
            hasFailed = True
        elif status.returncode != 0:
            print("Command failed for suite '" + key +"' with error code '" + str(status.returncode) + "'")
            hasFailed = True

    if hasFailed:
        raise Exception( f"runTests has failed, exiting" )

# Returns the valgrind command arguments
def AddValgrind(suite, outputToFile, outputToXml):
    executeCmd = ["valgrind", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes", "--verbose", "--error-exitcode=" + str(getvalgrindErrorCode())]

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
    executeCmd.append("--suppressions=" + filePath + "/../../" + valgrindIgnore)

    return executeCmd
