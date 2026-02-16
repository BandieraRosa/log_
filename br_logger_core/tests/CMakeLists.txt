find_package(GTest REQUIRED)

set(TEST_SOURCES
    test_log_level.cpp
    test_ring_buffer.cpp
    test_log_context.cpp
    test_pattern_formatter.cpp
    test_json_formatter.cpp
    test_console_sink.cpp
    test_callback_sink.cpp
    test_ring_memory_sink.cpp
    test_rotating_file_sink.cpp
    test_daily_file_sink.cpp
    test_logger_integration.cpp
    test_fixed_vector.cpp
    test_timestamp.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE br_logger_core GTest::GTest GTest::Main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
