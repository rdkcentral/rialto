#!/usr/bin/env python3

#
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

# Checks the given coverage stats file for failure based on the minimum coverage percentage

import sys

def main():
    if len(sys.argv) < 3:
        print("Can't check coverage stats - Wrong number of script arguments")
        sys.exit("Wrong number of script arguments")
    stats = parse_statistics(sys.argv[1])
    percentage_failure = sys.argv[2]
    check_coverage(stats, percentage_failure)

# Parse the stats file a file_path
def parse_statistics(file_path):
    try:
        file = open(file_path, "r")
        lines = file.readlines()
        covered_lines_line = [i for i in lines if "lines......" in i][0]
        covered_functions_line = [i for i in lines if "functions.." in i][0]
        covered_lines_str = covered_lines_line[15:covered_lines_line.find('%')]
        covered_functions_str = covered_functions_line[15:covered_functions_line.find('%')]
        file.close()
        return (float(covered_lines_str), float(covered_functions_str))
    except:
        print("Can't compare coverage stats - Could not open statistics file")
        return (0.0, 0.0)

# Checks the coverage stats agains the minimum coverage percentage
def check_coverage(stats, percentage_failure):
    is_failure = False

    # We only care about the function coverage as we are checking all APIs are covered, not error cases
    if stats[1] < float(percentage_failure):
        print("Function coverage " + str(stats[1]) + " is less than minimum failure percentage " + percentage_failure)
        is_failure = True

    if is_failure:
        sys.exit("Coverage stats failed, Line coverage: " + str(stats[0]) + ", Function coverage: " + str(stats[1]))

if __name__ == "__main__":
    main()
