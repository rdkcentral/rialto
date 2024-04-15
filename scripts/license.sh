#!/bin/sh

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


# Run with:
#  licence.sh scripts/licence.py Apache_2_0
#  licence.sh scripts/licence.py Lesser_GPL_2_1

licenceCommand=$1
expect=$2

returnStatus=0
for f in `find . \
            \( -type d \( -name build -o -name third-party -o -name .git \) -prune \) -o \
            \( -type f \
            \( -iname \*.h -o -iname \*.cpp -o -iname \*.cmake -o \
            -iname CMakeLists.txt -o -name \*.yml -o -name \*.proto -o \
            -name \*.py -o -name \*.sh \) \) \
            -print`
do
    r=`python $licenceCommand $f`
    if [ $? != 0 -o "$r" != "$expect" ]
    then
        returnStatus=1
        echo "File   : $f"
        echo "License: $r"
        echo
    fi
done
exit $returnStatus
