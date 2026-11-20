find_package(PkgConfig REQUIRED)
pkg_check_modules(RialtoMetricsTestGStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)

add_gtests(
        RialtoServerMetricsUnitTests
        RialtoMetricsTest.cpp
)

target_link_libraries(
        RialtoServerMetricsUnitTests
        RialtoMetrics
        PkgConfig::RialtoMetricsTestGStreamer
)