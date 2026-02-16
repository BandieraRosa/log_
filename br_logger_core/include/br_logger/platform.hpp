#pragma once

// ===== 平台检测 =====
#if defined(__linux__)
    #define BR_LOG_PLATFORM_LINUX 1
#elif defined(_WIN32)
    #define BR_LOG_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define BR_LOG_PLATFORM_MACOS 1
#endif

// ===== 编译模式 =====
#ifndef BR_LOG_EMBEDDED
    #define BR_LOG_EMBEDDED 0
#endif

#ifndef BR_LOG_HAS_THREAD
    #if BR_LOG_EMBEDDED
        #define BR_LOG_HAS_THREAD 0
    #else
        #define BR_LOG_HAS_THREAD 1
    #endif
#endif

// ===== Ring Buffer 默认大小 =====
#ifndef BR_LOG_RING_SIZE
    #if BR_LOG_EMBEDDED
        #define BR_LOG_RING_SIZE 256
    #else
        #define BR_LOG_RING_SIZE 8192
    #endif
#endif

// ===== 日志消息最大长度 =====
#ifndef BR_LOG_MAX_MSG_LEN
    #if BR_LOG_EMBEDDED
        #define BR_LOG_MAX_MSG_LEN 128
    #else
        #define BR_LOG_MAX_MSG_LEN 384
    #endif
#endif

// ===== Tag 系统常量 =====
#ifndef BR_LOG_MAX_TAGS
    #define BR_LOG_MAX_TAGS 8
#endif
#ifndef BR_LOG_MAX_TAG_KEY_LEN
    #define BR_LOG_MAX_TAG_KEY_LEN 32
#endif
#ifndef BR_LOG_MAX_TAG_VAL_LEN
    #define BR_LOG_MAX_TAG_VAL_LEN 64
#endif

// ===== 编译信息注入（CMake 设置） =====
#ifndef BR_LOG_GIT_HASH
    #define BR_LOG_GIT_HASH "unknown"
#endif
#ifndef BR_LOG_BUILD_TYPE
    #define BR_LOG_BUILD_TYPE "unknown"
#endif

// ===== cacheline 大小 =====
#ifndef BR_LOG_CACHELINE_SIZE
    #define BR_LOG_CACHELINE_SIZE 64
#endif
