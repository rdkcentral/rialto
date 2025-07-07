#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2025 Sky UK
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

find_path(EthanLog_INCLUDE_DIR
  NAMES ethanlog.h
  )

find_library(EthanLog_LIBRARY
  NAMES ethanlog
  )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EthanLog DEFAULT_MSG EthanLog_INCLUDE_DIR EthanLog_LIBRARY)

mark_as_advanced(EthanLog_INCLUDE_DIR EthanLog_LIBRARY)

if (ETHANLOG_FOUND OR EthanLog_FOUND)
  set(EthanLog_INCLUDE_DIRS "${EthanLog_INCLUDE_DIR}")
  set(EthanLog_LIBRARIES "${EthanLog_LIBRARY}")
  set(EthanLog_FOUND TRUE)
endif()

if (EthanLog_FOUND AND NOT TARGET EthanLog::EthanLog)
  add_library(EthanLog::EthanLog INTERFACE IMPORTED)
  set_property(TARGET EthanLog::EthanLog PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${EthanLog_INCLUDE_DIR}")
  set_property(TARGET EthanLog::EthanLog PROPERTY INTERFACE_LINK_LIBRARIES "${EthanLog_LIBRARY}")
endif()
