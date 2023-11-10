# Copyright (C) 2023 Sky UK
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

if ( COVERAGE_ENABLED )
    add_compile_options(-coverage)
endif()

add_gtests (
        RialtoCommonUnitTests

        # gtest code
        TimerTests.cpp
        EventThreadTests.cpp
        main.cpp
        )

target_include_directories(
        RialtoCommonUnitTests

        PUBLIC
        interface
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
        RialtoCommonUnitTests
        RialtoCommon
)

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoCommonUnitTests

        gcov
        )
endif()
