cmake_minimum_required(VERSION 3.16)
project(br_logger_core VERSION 1.0.0 LANGUAGES CXX)

# Options
option(BR_LOG_USE_FMTLIB "Use fmtlib for formatting" OFF)
option(BR_LOG_EMBEDDED_MODE "Build for embedded targets" OFF)
option(BR_LOG_BUILD_TESTS "Build unit tests" OFF)
option(BR_LOG_BUILD_BENCH "Build benchmarks" OFF)

# Core library
add_library(br_logger_core
    src/logger.cpp
    src/backend.cpp
    src/log_context.cpp
    src/timestamp.cpp
    src/formatters/pattern_formatter.cpp
    src/formatters/json_formatter.cpp
    src/sinks/console_sink.cpp
    src/sinks/rotating_file_sink.cpp
    src/sinks/daily_file_sink.cpp
    src/sinks/callback_sink.cpp
    src/sinks/ring_memory_sink.cpp
)

target_include_directories(br_logger_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(br_logger_core PUBLIC cxx_std_17)

# pthread on non-embedded
if(NOT BR_LOG_EMBEDDED_MODE)
    find_package(Threads REQUIRED)
    target_link_libraries(br_logger_core PUBLIC Threads::Threads)
endif()

# fmtlib (optional)
if(BR_LOG_USE_FMTLIB)
    find_package(fmt REQUIRED)
    target_link_libraries(br_logger_core PUBLIC fmt::fmt)
    target_compile_definitions(br_logger_core PUBLIC BR_LOG_USE_FMTLIB=1)
endif()

# Embedded mode
if(BR_LOG_EMBEDDED_MODE)
    target_compile_definitions(br_logger_core PUBLIC BR_LOG_EMBEDDED=1)
endif()

# Git hash injection
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()
target_compile_definitions(br_logger_core PUBLIC
    BR_LOG_GIT_HASH="${GIT_HASH}"
    BR_LOG_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)

# Install rules
include(GNUInstallDirs)

install(TARGETS br_logger_core
    EXPORT br_logger_core-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/br_logger
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT br_logger_core-targets
    FILE br_logger_core-targets.cmake
    NAMESPACE br_logger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/br_logger_core
)

# Config file for find_package()
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/br_logger_core-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/br_logger_core-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/br_logger_core
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/br_logger_core-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/br_logger_core-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/br_logger_core-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/br_logger_core
)

# Tests
if(BR_LOG_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Benchmarks
if(BR_LOG_BUILD_BENCH)
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    add_executable(bench_throughput tests/bench_throughput.cpp)
    target_link_libraries(bench_throughput PRIVATE br_logger_core benchmark::benchmark)
    target_compile_definitions(bench_throughput PRIVATE BR_LOG_ACTIVE_LEVEL=0)
endif()
