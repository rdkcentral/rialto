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
    find_path(SYSTEMD_INCLUDE_DIR NAMES systemd/sd-journal.h)
    find_library(SYSTEMD_LIBRARY NAMES systemd)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(SYSTEMD DEFAULT_MSG SYSTEMD_LIBRARY SYSTEMD_INCLUDE_DIR)

    if(SYSTEMD_FOUND)
      set(SYSTEMD_LIBRARIES ${SYSTEMD_LIBRARY})
      set(SYSTEMD_INCLUDE_DIRS ${SYSTEMD_INCLUDE_DIR})
      message(STATUS "Found systemd: ${SYSTEMD_INCLUDE_DIR} ${SYSTEMD_LIBRARY}")
    else()
      set(SYSTEMD_LIBRARIES "")
      set(SYSTEMD_INCLUDE_DIRS "")
      message(STATUS "Could NOT find systemd")
    endif()
else()
    set(SYSTEMD_INCLUDE_DIRS "")
    set(SYSTEMD_LIBRARIES "")
endif()