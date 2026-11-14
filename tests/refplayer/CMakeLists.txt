cmake_minimum_required(VERSION 3.16)
project(rialto_reference_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REFERENCE_PLAYER_USE_MOCKS "Build against the mock Rialto headers in test/mocks/rialto instead of the installed Rialto client." OFF)
option(REFERENCE_PLAYER_BUILD_JS_BINDING "Build libRialtoPlayerJSBindings.so for the WPE WebProcess (needs JavaScriptCore and GLib)." OFF)
option(REFERENCE_PLAYER_BUILD_TESTS "Build the host test for the player library (mock mode only)." OFF)
set(REFERENCE_PLAYER_JSC_PKG "javascriptcoregtk-4.1" CACHE STRING "pkg-config name of the JavaScriptCore package used by the JS binding.")

set(RIALTO_INCLUDE_HINTS
    "${CMAKE_SOURCE_DIR}/../git/rialto-master/rialto-master/media/public/include"
    "${CMAKE_SOURCE_DIR}/../git/rialto-master/rialto-master"
    "/usr/local/include"
    "/usr/include"
)

if(NOT REFERENCE_PLAYER_USE_MOCKS)
    find_path(RIALTO_INCLUDE_DIR NAMES IControl.h PATHS ${RIALTO_INCLUDE_HINTS} PATH_SUFFIXES rialto media/public/include include)
    find_library(RIALTO_CLIENT_LIB NAMES RialtoClient RialtoPlayer)
    if(NOT RIALTO_INCLUDE_DIR)
        message(WARNING "Rialto headers were not found; building against the mock Rialto headers.")
        set(REFERENCE_PLAYER_USE_MOCKS ON)
    endif()
    if(NOT RIALTO_CLIENT_LIB)
        message(WARNING "Rialto client library was not found; building against the mock Rialto headers.")
        set(REFERENCE_PLAYER_USE_MOCKS ON)
    endif()
endif()

find_package(Threads REQUIRED)

# The Rialto API headers come from the installed client, or from the mocks in mock mode.
if(REFERENCE_PLAYER_USE_MOCKS)
    set(RIALTO_API_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/mocks/rialto")
    set(RIALTO_CLIENT_LIB "")
else()
    set(RIALTO_API_INCLUDE_DIR "${RIALTO_INCLUDE_DIR}")
endif()

# The sources test this macro, not the CMake option, to pick the mock build paths.
if(REFERENCE_PLAYER_USE_MOCKS)
    add_compile_definitions(REFERENCE_PLAYER_USE_MOCKS=1)
endif()

# Core library and demuxer. Object library so the host test can link the internal
# RefPlayer symbols, which are hidden in the shared library.
set(REFERENCE_PLAYER_CORE_SOURCES
    src/core/Logger.cpp
    src/core/EventBus.cpp
    src/core/PositionTracker.cpp
    src/core/PlayerStateMachine.cpp
    src/core/CommandWorker.cpp
    src/core/RialtoSession.cpp
    src/core/SourceManager.cpp
    src/core/MediaFeeder.cpp
    src/core/PipelineCallbacks.cpp
    src/core/RefPlayer.cpp
    src/demux/DemuxerRegistry.cpp
    src/demux/Mp4Demuxer.cpp
)

add_library(RefPlayerCore OBJECT ${REFERENCE_PLAYER_CORE_SOURCES})
set_target_properties(RefPlayerCore PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
target_include_directories(RefPlayerCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/demux ${RIALTO_API_INCLUDE_DIR})

# libRialtoRefPlayer: the only symbol family it exports is createRefPlayer().
add_library(RialtoRefPlayer SHARED $<TARGET_OBJECTS:RefPlayerCore>)
set_target_properties(RialtoRefPlayer PROPERTIES OUTPUT_NAME RialtoRefPlayer)
target_include_directories(RialtoRefPlayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${RIALTO_API_INCLUDE_DIR})
target_link_libraries(RialtoRefPlayer PRIVATE Threads::Threads ${RIALTO_CLIENT_LIB})

# CLI: a debugging aid that runs outside a browser. It is not the acceptance gate.
add_executable(rialto_reference_player
    src/frontend/cli/main.cpp
    src/frontend/cli/CliTestSupport.cpp
    src/frontend/cli/ContractValidator.cpp
    src/frontend/cli/NegativeChecks.cpp
)
target_include_directories(rialto_reference_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/cli ${RIALTO_API_INCLUDE_DIR})
target_link_libraries(rialto_reference_player PRIVATE RialtoRefPlayer Threads::Threads ${RIALTO_CLIENT_LIB})

# JS binding: loaded into the WPE WebProcess by the injected-bundle hook.
if(REFERENCE_PLAYER_BUILD_JS_BINDING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSC REQUIRED IMPORTED_TARGET ${REFERENCE_PLAYER_JSC_PKG})
    pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

    add_library(RialtoPlayerJSBindings SHARED
        src/frontend/jsbinding/RialtoPlayerJSBinding.cpp
        src/frontend/jsbinding/JSEventListener.cpp
        src/frontend/jsbinding/JSUtils.cpp
    )
    set_target_properties(RialtoPlayerJSBindings PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_link_options(RialtoPlayerJSBindings PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/jsbinding/RialtoPlayerJSBindings.map")
    target_link_libraries(RialtoPlayerJSBindings PRIVATE
        RialtoRefPlayer PkgConfig::JSC PkgConfig::GLIB Threads::Threads)
    install(TARGETS RialtoPlayerJSBindings LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# Host test: drives the library against the scripted mock pipeline, no device needed.
if(REFERENCE_PLAYER_BUILD_TESTS)
    if(NOT REFERENCE_PLAYER_USE_MOCKS)
        message(FATAL_ERROR "REFERENCE_PLAYER_BUILD_TESTS needs REFERENCE_PLAYER_USE_MOCKS=ON")
    endif()
    enable_testing()
    add_executable(refplayer_host_test test/unit/RefPlayerTest.cpp)
    target_include_directories(refplayer_host_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/demux ${RIALTO_API_INCLUDE_DIR})
    target_link_libraries(refplayer_host_test PRIVATE RefPlayerCore Threads::Threads)
    add_test(NAME refplayer_host_test COMMAND refplayer_host_test)

    add_executable(refplayer_cli_test test/unit/CliTest.cpp src/frontend/cli/CliTestSupport.cpp)
    target_include_directories(refplayer_cli_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/cli ${RIALTO_API_INCLUDE_DIR})
    target_link_libraries(refplayer_cli_test PRIVATE Threads::Threads)
    add_test(NAME refplayer_cli_test COMMAND refplayer_cli_test)
endif()

install(TARGETS rialto_reference_player RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS RialtoRefPlayer LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

message(STATUS "REFERENCE_PLAYER_USE_MOCKS=${REFERENCE_PLAYER_USE_MOCKS}")
message(STATUS "REFERENCE_PLAYER_BUILD_JS_BINDING=${REFERENCE_PLAYER_BUILD_JS_BINDING}")
message(STATUS "REFERENCE_PLAYER_BUILD_TESTS=${REFERENCE_PLAYER_BUILD_TESTS}")
