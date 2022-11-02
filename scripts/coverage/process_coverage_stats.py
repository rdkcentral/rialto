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
        write_output("Can't compare coverage stats - Wrong number of script arguments")
        sys.exit("Wrong number of script arguments")
    master_stats = parse_statistics(sys.argv[1])
    current_stats = parse_statistics(sys.argv[2])
    comparison_output = compare_coverage(master_stats, current_stats)
    write_output(comparison_output)

def parse_statistics(file_path):
    try:
        file = open(file_path, "r")
        lines = file.readlines()
        covered_lines_str = lines[2][15:lines[2].find('%')]
        covered_functions_str = lines[3][15:lines[3].find('%')]
        file.close()
        return (float(covered_lines_str), float(covered_functions_str))
    except:
        write_output("Can't compare coverage stats - Could not open statistics file")
        sys.exit("Could not open statistics file...")

def compare_coverage(master_stats, current_stats):
    output_text = "Coverage statistics of your commit:\n"
    if current_stats[0] < master_stats[0]:
        output_text += "WARNING: Lines coverage decreased from: " + str(master_stats[0]) + "% to "
        output_text += str(current_stats[0]) + "%\n"

    elif current_stats[0] == master_stats[0]:
        output_text += "Lines coverage stays unchanged and is: " + str(current_stats[0]) + "%\n"
    else:
        output_text += "Congratulations, your commit improved lines coverage from: " + str(master_stats[0])
        output_text += "% to " + str(current_stats[0]) + "%\n"

    if current_stats[1] < master_stats[1]:
        output_text += "WARNING: Functions coverage decreased from: " + str(master_stats[1]) + "% to "
        output_text += str(current_stats[1]) + "%\n"
    elif current_stats[1] == master_stats[1]:
        output_text += "Functions coverage stays unchanged and is: " + str(current_stats[1]) + "%\n"
    else:
        output_text += "Congratulations, your commit improved functions coverage from: " + str(master_stats[1])
        output_text += "% to " + str(current_stats[1]) + "%\n"
    return output_text

def write_output(output_text):
    output_file = open("comparison_output.txt", "w")
    output_file.write(output_text)
    output_file.close()

if __name__ == "__main__":
    main()
