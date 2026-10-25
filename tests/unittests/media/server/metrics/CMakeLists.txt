add_gtests(
        RialtoServerMetricsUnitTests
        RialtoMetricsTest.cpp
)

target_link_libraries(
        RialtoServerMetricsUnitTests
        RialtoMetrics
)