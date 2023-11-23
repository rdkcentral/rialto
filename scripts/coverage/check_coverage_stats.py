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

import sys

def main():
    if len(sys.argv) < 3:
        write_output("Can't check coverage stats - Wrong number of script arguments")
        sys.exit("Wrong number of script arguments")
    stats = parse_statistics(sys.argv[1])
    percentage_failure = sys.argv[2]
    comparison_output = check_coverage(stats, percentage_failure)
    write_output(comparison_output)

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
        write_output("Can't compare coverage stats - Could not open statistics file")
        return (0.0, 0.0)

def check_coverage(stats, percentage_failure):
    is_failure = False

    if stats[0] < percentage_failure:
        print("Line coverage " + str(stats[0]) + " is less than minimum failure percentage " + str(percentage_failure))
        is_failure = True

    if stats[1] < percentage_failure:
        print("Function coverage " + str(stats[1]) + " is less than minimum failure percentage " + str(percentage_failure))
        is_failure = True

    if is_failure:
        sys.exit("Coverage stats failed, Line coverage: " + str(stats[0]) + ", Function coverage: " + str(stats[1]))
    
    return output_text

if __name__ == "__main__":
    main()
