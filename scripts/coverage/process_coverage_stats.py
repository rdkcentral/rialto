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
    
    # if "WARNING" in comparison_output:
    #     write_output(comparison_output)
    #     sys.exit("Coverage decreased for both lines and functions")

    # Check if both lines and functions coverage unchanged, if yes, exit
    if "unchanged" in comparison_output:
        write_output(comparison_output)
        sys.exit("Coverage stays unchanged for both lines and functions. Exiting...")   

    # exit_status=0
    
    #  # Check if both line and function coverage decreased
    # if current_stats[0] < master_stats[0] and current_stats[1] < master_stats[1]:
    #     sys.exit("Both line and function coverage decreased.")
    # elif current_stats[0] < master_stats[0]:
    #         sys.exit("Only line coverage decreased.")
    # elif current_stats[1] < master_stats[1]:
    #     sys.exit("Only function coverage decreased.")

    # # Check if both line and function coverage are unchanged
    # if current_stats[0] == master_stats[0] and current_stats[1] == master_stats[1]:
    #     sys.exit("Both line and function coverage remain unchanged.")
    # elif current_stats[0] == master_stats[0]:
    #     sys.exit("Only line coverage remains unchanged.")
    # elif current_stats[1] == master_stats[1]:
    #     sys.exit("Only function coverage remains unchanged.")
    
    # exit_status = 1
    write_output(comparison_output)
    # sys.exit(exit_status)
    # # if current_stats[0] <= master_stats[0] or current_stats[1] <= master_stats[1]:
    # #        sys.exit("Code coverage decreased or remained the same. Exiting with a non-zero status code.")
    # lines_output, functions_output = compare_coverage(master_stats, current_stats)

    # # Print the outputs
    # print(lines_output)
    # print(functions_output)

    # # Exit if lines coverage has decreased
    # if "WARNING" in lines_output:
    #     sys.exit("Lines coverage has decreased. Exiting...")
    # # if "Unchanged" in lines_output:
    # #     sys.exit("Lines coverage unchanged. Exiting... ")

    # # Exit if functions coverage has decreased
    # if "WARNING" in functions_output:
    #     sys.exit("Functions coverage has decreased. Exiting...")
    # # if "Unchanged" in lines_output:
    # #     sys.exit("Functions coverage unchanged. Exiting... ")

    # write_output(lines_output + functions_output)

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

# def compare_coverage(master_stats, current_stats):

#     output_text = "Coverage statistics of your commit:\n"
#     if current_stats[0] < master_stats[0]:
#         output_text += "WARNING: Lines coverage decreased from: " + str(master_stats[0]) + "% to "
#         output_text += str(current_stats[0]) + "%\n"
#         sys.exit(1)

#     elif current_stats[0] == master_stats[0]:
#         output_text += "Lines coverage stays unchanged and is: " + str(current_stats[0]) + "%\n"
#     else:
#         output_text += "Congratulations, your commit improved lines coverage from: " + str(master_stats[0])
#         output_text += "% to " + str(current_stats[0]) + "%\n"

#     if current_stats[1] < master_stats[1]:
#         output_text += "WARNING: Functions coverage decreased from: " + str(master_stats[1]) + "% to "
#         output_text += str(current_stats[1]) + "%\n"
#     elif current_stats[1] == master_stats[1]:
#         output_text += "Functions coverage stays unchanged and is: " + str(current_stats[1]) + "%\n"
#     else:
#         output_text += "Congratulations, your commit improved functions coverage from: " + str(master_stats[1])
#         output_text += "% to " + str(current_stats[1]) + "%\n"
#     return output_text
# def compare_coverage(master_stats, current_stats):
    

#     # Check for lines coverage
#     if current_stats[0] < master_stats[0]:
#         lines_output += "WARNING: Decreased from {}% to {}%\n".format(master_stats[0], current_stats[0])
#     elif current_stats[0] == master_stats[0]:
#         lines_output += "Unchanged at {}%\n".format(current_stats[0])
#     else:
#         lines_output += "Improved from {}% to {}%\n".format(master_stats[0], current_stats[0])

#     # Check for functions coverage
#     if current_stats[1] < master_stats[1]:
#         functions_output += "WARNING: Decreased from {}% to {}%\n".format(master_stats[1], current_stats[1])
#     elif current_stats[1] == master_stats[1]:
#         functions_output += "Unchanged at {}%\n".format(current_stats[1])
#     else:
#         functions_output += "Improved from {}% to {}%\n".format(master_stats[1], current_stats[1])

#     return lines_output, functions_output




# def compare_coverage(master_stats, current_stats):
#     output_text = "Coverage statistics of your commit:\n"
    
#     lines_message = ""
#     functions_message = ""

#     if current_stats[0] < master_stats[0]:
#         lines_message = "WARNING: Lines coverage decreased from: " + str(master_stats[0]) + "% to " + str(current_stats[0]) + "%\n"
#     elif current_stats[0] == master_stats[0]:
#         lines_message = "Lines coverage stays unchanged and is: " + str(current_stats[0]) + "%\n"
#     else:
#         lines_message = "Congratulations, your commit improved lines coverage from: " + str(master_stats[0]) + "% to " + str(current_stats[0]) + "%\n"

#     if current_stats[1] < master_stats[1]:
#         functions_message = "WARNING: Functions coverage decreased from: " + str(master_stats[1]) + "% to " + str(current_stats[1]) + "%\n"
#     elif current_stats[1] == master_stats[1]:
#         functions_message = "Functions coverage stays unchanged and is: " + str(current_stats[1]) + "%\n"
#     else:
#         functions_message = "Congratulations, your commit improved functions coverage from: " + str(master_stats[1]) + "% to " + str(current_stats[1]) + "%\n"

#     output_text += lines_message
#     output_text += functions_message

#     return output_text




def compare_coverage(master_stats, current_stats):
    output_text = "Coverage statistics of your commit:\n"
    
    lines_message = ""
    functions_message = ""

    if current_stats[0] < master_stats[0]:
        lines_message = "WARNING: Lines coverage decreased from: " + str(master_stats[0]) + "% to " + str(current_stats[0]) + "%\n"
    elif current_stats[0] == master_stats[0]:
        lines_message = "Lines coverage stays unchanged and is: " + str(current_stats[0]) + "%\n"
    else:
        lines_message = "Congratulations, your commit improved lines coverage from: " + str(master_stats[0]) + "% to " + str(current_stats[0]) + "%\n"

    if current_stats[1] < master_stats[1]:
        functions_message = "WARNING: Functions coverage decreased from: " + str(master_stats[1]) + "% to " + str(current_stats[1]) + "%\n"
    elif current_stats[1] == master_stats[1]:
        functions_message = "Functions coverage stays unchanged and is: " + str(current_stats[1]) + "%\n"
    else:
        functions_message = "Congratulations, your commit improved functions coverage from: " + str(master_stats[1]) + "% to " + str(current_stats[1]) + "%\n"

    output_text += lines_message
    output_text += functions_message

    return output_text



def write_output(output_text):
    output_file = open("comparison_output.txt", "w")
    output_file.write(output_text)
    output_file.close()

if __name__ == "__main__":
    main()
