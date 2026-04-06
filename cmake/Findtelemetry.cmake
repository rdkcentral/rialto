#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Sky UK
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
  find_path(TELEMETRY_INCLUDE_DIR NAMES telemetry_busmessage_sender.h)
  find_library(TELEMETRY_LIBRARY NAMES telemetry_msgsender)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TELEMETRY DEFAULT_MSG TELEMETRY_LIBRARY TELEMETRY_INCLUDE_DIR)

  mark_as_advanced(TELEMETRY_INCLUDE_DIR TELEMETRY_LIBRARY)

  if(TELEMETRY_FOUND)
    set(TELEMETRY_LIBRARIES ${TELEMETRY_LIBRARY})
    set(TELEMETRY_INCLUDE_DIRS ${TELEMETRY_INCLUDE_DIR})
  else()
    set(TELEMETRY_LIBRARIES "")
    set(TELEMETRY_INCLUDE_DIRS "")
  endif()
else()
  set(TELEMETRY_INCLUDE_DIRS "")
  set(TELEMETRY_LIBRARIES "")
endif()
