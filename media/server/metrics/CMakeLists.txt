find_package( PkgConfig REQUIRED )
pkg_check_modules( RialtoMetricsGStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0 )

add_library(
        RialtoMetrics
        STATIC

        source/RialtoMetricsImpl.cpp
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

        PRIVATE
        PkgConfig::RialtoMetricsGStreamer
        protobuf::libprotobuf
)

install(
        TARGETS RialtoMetrics
        DESTINATION lib
)