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

find_package( PkgConfig REQUIRED )
pkg_check_modules( RialtoMetricsGStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0 )

add_library(
        RialtoMetrics
        STATIC

        source/RialtoMetrics.cpp
)

target_include_directories(
        RialtoMetrics

        PUBLIC
        include
)

set_property(
        TARGET RialtoMetrics
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
        RialtoMetrics

        PUBLIC
        RialtoPlayerPublic

        PRIVATE
        PkgConfig::RialtoMetricsGStreamer
        protobuf::libprotobuf
        RialtoLogging
)

install(
        TARGETS RialtoMetrics
        DESTINATION lib
)