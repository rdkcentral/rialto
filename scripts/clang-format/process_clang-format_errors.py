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

# Processes the clang-format_errors.log file and construct junit xml

import os, sys
import argparse
import xml.etree.ElementTree as ET
import csv

# Default variables
clangFile = "clang-format_errors"

def main ():
    # Check log file
    errorLogFile = clangFile + ".log"
    if not os.path.isfile(errorLogFile) or os.stat(errorLogFile).st_size == 0:
        # No errors to process
        return

    # Remove xml if exists
    xmlFile = clangFile + ".xml"
    if os.path.isfile(xmlFile):
        os.remove(xmlFile)

    list = createErrorList(errorLogFile)

    # Construct xml
    testsuiteElement = ET.Element("testsuite")
    testsuiteElement.set('errors', str(0))
    testsuiteElement.set('failures', str(len(list)))
    testsuiteElement.set('name', 'clang-format')
    testsuiteElement.set('tests', str(len(list)))

    for error in list:
        testcaseElement = ET.SubElement(testsuiteElement, "testcase")
        testcaseElement.set('name', error['fileName'])
        failureElement = ET.SubElement(testcaseElement, "failure")
        failureElement.text = error['errorInfo']

    tree = ET.ElementTree(testsuiteElement)
    tree.write(xmlFile, encoding='UTF-8', xml_declaration = True)

    sys.exit("Failure detected, code has not been correctly formatted")

# Processes the clang-format_errors.log and returns a list of all the errors
def createErrorList(errorLogFile):
    errorList = []

    with open(errorLogFile) as f:
        lines = f.readlines()
        for line in lines:
            if line.find(': error:') != -1 and line.find('[-Wclang-format-violations]') != -1:
                errorList.append({'fileName' : line[:line.find(': error:')], 'errorInfo' : ''})
            else:
                errorList[-1].update({'errorInfo' : errorList[-1]['errorInfo'] + line})

    return errorList

if __name__ == "__main__":
    main()
