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

if(NOT NATIVE_BUILD)

    find_path(RDKPERF_INCLUDE_DIR NAMES rdk_perf.h)
    find_library(RDKPERF_LIBRARY NAMES rdkperf) 

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(RDKPERF DEFAULT_MSG RDKPERF_LIBRARY RDKPERF_INCLUDE_DIR)
 
    if(RDKPERF_FOUND)
      set(RDKPERF_LIBRARIES ${RDKPERF_LIBRARY})
      set(RDKPERF_INCLUDE_DIRS ${RDKPERF_INCLUDE_DIR})
      message(STATUS "Found rdkperf: ${RDKPERF_INCLUDE_DIR} ${RDKPERF_LIBRARY}")
    else()
      set(RDKPERF_LIBRARIES "")
      set(RDKPERF_INCLUDE_DIRS "")
      message(STATUS "Could NOT find rdkperf")
    endif()
else()
    set(RDKPERF_INCLUDE_DIRS "")
    set(RDKPERF_LIBRARIES "")
endif()