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

include(ExternalProject)
include(GNUInstallDirs)

set( GOOGLETEST_FOUND TRUE )
set( GOOGLETEST_VERSION 1.12.1 )

if( CMAKE_CROSSCOMPILING )
    set( GOOGLETEST_EXTRA_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" )
endif()

ExternalProject_Add(
        googletest-project

        PREFIX deps/googletest-${GOOGLETEST_VERSION}

        URL      https://github.com/google/googletest/archive/release-${GOOGLETEST_VERSION}.tar.gz
        # URL_HASH SHA256=

        # GIT_REPOSITORY "https://github.com/google/googletest.git"
        # GIT_TAG        "origin/${GOOGLETEST_VERSION}"
        # GIT_SHALLOW    1

        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_VERBOSE_MAKEFILE=$(CMAKE_VERBOSE_MAKEFILE)
            # Build static lib but suitable to be included in a shared lib.
            -DCMAKE_POSITION_INDEPENDENT_CODE=On
            ${GOOGLETEST_EXTRA_CMAKE_ARGS}
        )


ExternalProject_Get_Property( googletest-project INSTALL_DIR )

set( GTEST_INCLUDE_DIRS ${INSTALL_DIR}/include )
set( GMOCK_INCLUDE_DIRS ${INSTALL_DIR}/include )
file( MAKE_DIRECTORY ${GTEST_INCLUDE_DIRS} )
file( MAKE_DIRECTORY ${GMOCK_INCLUDE_DIRS} )

set( GTEST_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX} )
set( GTEST_MAIN_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX} )
set( GMOCK_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX} )
set( GMOCK_MAIN_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX} )
set( GTEST_LIBRARIES ${GTEST_LIBRARY} )
set( GMOCK_LIBRARIES ${GMOCK_LIBRARY} )

add_library( GoogleTest::gtest STATIC IMPORTED )
set_property( TARGET GoogleTest::gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARIES} )
set_property( TARGET GoogleTest::gtest PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS} )
add_dependencies( GoogleTest::gtest googletest-project )

add_library( GoogleTest::gmock STATIC IMPORTED )
set_property( TARGET GoogleTest::gmock PROPERTY IMPORTED_LOCATION ${GMOCK_LIBRARIES} )
set_property( TARGET GoogleTest::gmock PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GMOCK_INCLUDE_DIRS} )
add_dependencies( GoogleTest::gmock googletest-project )

unset( INSTALL_DIR )



include( GoogleTest )

macro( add_gtests TESTNAME )

    # create an exectuable in which the tests will be stored
    add_executable( ${TESTNAME} ${ARGN} )

    # link the Google test infrastructure, mocking library, and a default main
    # function to the test executable.  Remove g_test_main if writing your own
    # main function.
    target_link_libraries( ${TESTNAME} ${GTEST_MAIN_LIBRARY} GoogleTest::gtest GoogleTest::gmock Threads::Threads )

    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests( ${TESTNAME}
            # set a working directory so your project root so that you can find
            # test data via paths relative to the project root
            WORKING_DIRECTORY ${PROJECT_DIR}
            )

    set_target_properties( ${TESTNAME} PROPERTIES FOLDER test )

endmacro()

