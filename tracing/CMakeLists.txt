

add_library( RialtoTracing

        STATIC
        perfetto/sdk/perfetto.cc
        perfetto/source/RialtoPerfettoTracing.cpp
        perfetto/source/PerfettoTracingSingleton.cpp

        )

target_include_directories( RialtoTracing

        PUBLIC
        perfetto/sdk/
        ${CMAKE_CURRENT_SOURCE_DIR}/perfetto/include
        ${CMAKE_CURRENT_BINARY_DIR}

        PRIVATE
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>

        )

target_compile_definitions( RialtoTracing

        PUBLIC
        PERFETTO_SDK_VERSION="29.0"

        )

configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/perfetto/include/RialtoPerfettoTracing.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/RialtoPerfettoTracing.h

        )
