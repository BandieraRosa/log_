#pragma once
#include <cstdint>

namespace br_logger {

struct SourceLocation {
    const char* file_path;
    const char* file_name;
    const char* function_name;
    const char* pretty_function;
    uint32_t    line;
    uint32_t    column;          // C++20 可用，否则为 0

    static constexpr const char* extract_filename(const char* path) {
        const char* name = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '/' || *p == '\\') name = p + 1;
        }
        return name;
    }
};

#if __cplusplus >= 202002L && __has_include(<source_location>)
    #include <source_location>
    #define BR_LOG_HAS_SOURCE_LOCATION 1
    #define BR_LOG_CURRENT_LOCATION() \
        ::br_logger::SourceLocation { \
            std::source_location::current().file_name(), \
            ::br_logger::SourceLocation::extract_filename(std::source_location::current().file_name()), \
            std::source_location::current().function_name(), \
            std::source_location::current().function_name(), \
            std::source_location::current().line(), \
            std::source_location::current().column() \
        }
#else
    #define BR_LOG_HAS_SOURCE_LOCATION 0
    #define BR_LOG_CURRENT_LOCATION() \
        ::br_logger::SourceLocation { \
            __FILE__, \
            ::br_logger::SourceLocation::extract_filename(__FILE__), \
            __func__, \
            __PRETTY_FUNCTION__, \
            static_cast<uint32_t>(__LINE__), \
            0u \
        }
#endif

} // namespace br_logger
