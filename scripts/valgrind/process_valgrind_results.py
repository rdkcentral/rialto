#!/usr/bin/env python3

#
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

# Processes any valgrind xml files in /build and summarises the results in a csv

import os
import argparse
import xml.etree.ElementTree as ET
import csv

# Default variables
valgrindOutput = "valgrind_report"
outputDir = "build"
possibleErrors = ['Leak_PossiblyLost', 'Leak_IndirectlyLost', 'Leak_DefinitelyLost', 'UninitCondition', 'InvalidWrite', 'InvalidRead', 'UnknownError']

def main ():
    # Create csv
    csv_file = valgrindOutput + ".csv"
    if os.path.isfile(csv_file):
        os.remove(csv_file)
    f = open(csv_file, 'w')
    errorWriter = csv.writer(f, delimiter=',')
    errorWriter.writerow(["Suites"] + possibleErrors)

    # Find all the valgrind xml files and add the errors to the csv
    directory = os.fsencode(outputDir)
    for file in os.listdir(directory):
        filename = os.fsdecode(file)
        if filename.endswith(valgrindOutput + ".xml"):
            hasFailed, results = ParseGtestXml(outputDir + "/" + filename)

            # if failed write the results to the csv
            if hasFailed:
                # Get suite
                prefixLen = len("_" + valgrindOutput + ".xml")
                suite = filename[:-prefixLen]

                errorWriter.writerow([suite] + results)
            continue
        else:
            continue

    f.close()

# Parses the xml and returns an array of the number of failures
def ParseGtestXml(xml):
    hasFailed = False
    results = [0 for x in range(len(possibleErrors))]

    # Parse the xml for all the errors
    root = ET.parse(xml).getroot()
    for errors in root.findall('error'):

        # Leak_StillReachable expected, accounts for static and global objects
        # Do not add to the error table
        kind = errors.find('kind').text
        if kind == "Leak_StillReachable":
            continue

        hasFailed = True

        # Find which error to increment
        errorWritten = False
        for i in range(len(possibleErrors)):
            if possibleErrors[i] == kind:
                results[i] += 1
                errorWritten = True
                break

        # Add as unknown error if not known
        if not errorWritten:
            print ("Unknown error '" + kind + "'")
            results[-1] += 1

    return hasFailed, results

if __name__ == "__main__":
    main()
