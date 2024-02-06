#!/usr/bin/env python

#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 Sky UK
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

import re
import sys

# python a.py myfile.txt

if len(sys.argv) != 2:
    print("Wrong arguments")
    exit(1)

file1 = open(sys.argv[1], 'r')

res = ""
initial = True
regMatch = re.compile('^(\*|#)(| .*)$')
while line := file1.readline():
    line = line.strip()
    if line == "*/":
        break
    elif initial and (line == '/*' or line == "#" or line == "" or line.startswith('#!')):
        pass # First or initial lines
    else:
        initial = False
        m = regMatch.match(line)
        if m:
            res += m.groups()[1]
        else:
            break

file1.close()
res = re.sub(' +', ' ', res) # remove double spaces

if re.match("^ If not stated otherwise in this file or this component's LICENSE file the following copyright and licenses apply: Copyright 202[2-4] Sky UK Licensed under the Apache License, Version 2\.0 \(the \"License\"\); you may not use this file except in compliance with the License\. You may obtain a copy of the License at http://www\.apache\.org/licenses/LICENSE-2\.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied\. See the License for the specific language governing permissions and limitations under the License\.", res):
    print("Apache_2_0");
    exit(0)


if re.match("^ Copyright \(C\) 202[2-4] Sky UK This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; version 2\.1 of the License\. This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\. See the GNU Lesser General Public License for more details\. You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to the Free Software Foundation, Inc\., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA", res):
    print("Lesser_GPL_2_1");
    exit(0)

if re == "":
    print("No license comment found")
    exit(1)
    
print("Unknown license comment: " + res)
exit(1)
