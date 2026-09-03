set( CMAKE_CXX_STANDARD 17 )

add_test_executable(
        RialtoServerMediaCapabilitiesTests

        mediaCapabilities/MediaCapabilitiesTests.cpp
        mediaCapabilities/MediaCapabilitiesTestsFixture.cpp
)

target_include_directories(
        RialtoServerMediaCapabilitiesTests

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mediaCapabilities
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
        RialtoServerMediaCapabilitiesTests

        PRIVATE
        RialtoServerMain
        RialtoServerGstPlayer
        RialtoCommon
        RialtoServerCommon
        RialtoLogging
        gmock
)

add_dependencies(
        RialtoServerMediaCapabilitiesTests
        RialtoServerMain
)
