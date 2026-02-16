# br_logger

高性能、跨平台 C++ 日志库，面向 ROS2 机器人上位机、日常 C++ 开发、嵌入式开发三大场景。

## 特性

- **无锁异步**：MPSC 环形队列 + 后端消费线程，日志写入不阻塞业务逻辑
- **编译期过滤**：通过 `BR_LOG_ACTIVE_LEVEL` 在编译期移除低级别日志，零开销
- **多 Sink 架构**：Console / RotatingFile / DailyFile / Callback / RingMemory，可自由组合
- **结构化上下文**：全局标签、线程局部标签、ScopedTag RAII，自动注入到每条日志
- **双格式化器**：PatternFormatter（19 个占位符）、JsonFormatter（JSON 结构化输出）
- **跨平台**：Linux / macOS / Windows，C++17 标准
- **嵌入式裁剪**：`BR_LOG_EMBEDDED_MODE` 可裁剪线程、缩减内存，适配 MCU

## 快速开始

### 编译

```bash
cmake -B build -DBR_LOG_BUILD_TESTS=ON -DBR_LOG_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build
```

### 最小示例

```cpp
#include <br_logger/logger.hpp>
#include <br_logger/sinks/console_sink.hpp>
#include <br_logger/formatters/pattern_formatter.hpp>

int main() {
    auto& logger = br_logger::Logger::Instance();

    auto console = std::make_unique<br_logger::ConsoleSink>();
    console->SetFormatter(std::make_unique<br_logger::PatternFormatter>());
    logger.AddSink(std::move(console));
    logger.Start();

    LOG_INFO("hello %s", "world");
    LOG_WARN("value = %d", 42);

    logger.Stop();
}
```

### CMake 集成

**子目录方式**（推荐）：

```cmake
add_subdirectory(path/to/br_logger)
target_link_libraries(your_target PRIVATE br_logger_core)
```

**install 后 find_package**：

```cmake
find_package(br_logger_core REQUIRED)
target_link_libraries(your_target PRIVATE br_logger::br_logger_core)
```

## API 参考

### Logger

```cpp
auto& logger = br_logger::Logger::Instance();

logger.AddSink(std::move(sink));   // 添加 Sink（Start() 前调用）
logger.SetLevel(LogLevel::Debug);  // 运行时级别阈值（atomic）
logger.Start();                    // 启动后端消费线程
logger.Stop();                     // 停止后端、刷新所有 Sink
logger.Drain(64);                  // 手动消费（嵌入式/测试用）
logger.DropCount();                // 查询丢弃计数
```

### 日志宏

| 宏 | 用法 |
|----|------|
| `LOG_TRACE(fmt, ...)` | 最低级别，可被编译期移除 |
| `LOG_DEBUG(fmt, ...)` | 调试信息 |
| `LOG_INFO(fmt, ...)` | 常规信息 |
| `LOG_WARN(fmt, ...)` | 警告 |
| `LOG_ERROR(fmt, ...)` | 错误 |
| `LOG_FATAL(fmt, ...)` | 致命错误 |
| `LOG_INFO_IF(cond, fmt, ...)` | 条件日志 |
| `LOG_WARN_IF(cond, fmt, ...)` | 条件日志 |
| `LOG_ERROR_IF(cond, fmt, ...)` | 条件日志 |
| `LOG_EVERY_N(lvl, n, fmt, ...)` | 每 N 次记录一条 |
| `LOG_ONCE(lvl, fmt, ...)` | 只记录一次 |

格式字符串使用 printf 风格（`%d`, `%s`, `%f` 等），启用 fmtlib 时使用 `{}` 占位符。

### Sink

| Sink | 构造参数 | 说明 |
|------|----------|------|
| `ConsoleSink` | `force_color` (optional) | 自动检测 TTY，stderr 分流 WARN+ |
| `RotatingFileSink` | `path`, `max_size`, `max_files` | 按大小轮转，POSIX I/O |
| `DailyFileSink` | `dir`, `name`, `max_days`, `use_utc` | 按日期轮转 |
| `CallbackSink` | `std::function<void(const LogEntry&)>` | 用户自定义回调 |
| `RingMemorySink` | `capacity` | 环形内存存储，支持 DumpToFile |

每个 Sink 支持：
- `SetFormatter(std::unique_ptr<IFormatter>)` — 设置独立格式化器
- `SetLevel(LogLevel)` — 设置 Sink 级别过滤（独立于全局）

### Formatter

**PatternFormatter** — 19 个占位符：

| 占位符 | 含义 |
|--------|------|
| `%D` | 日期 (YYYY-MM-DD) |
| `%T` | 时间 (HH:MM:SS) |
| `%e` | 微秒 |
| `%L` | 级别全名 |
| `%l` | 级别缩写 |
| `%f` | 文件名 |
| `%F` | 文件完整路径 |
| `%n` | 函数名 |
| `%N` | 完整函数签名 |
| `%#` | 行号 |
| `%t` | 线程 ID |
| `%P` | 进程 ID |
| `%r` | 线程名 |
| `%s` | 序列号 |
| `%g` | 标签 |
| `%m` | 消息正文 |
| `%C` | ANSI 颜色起始 |
| `%R` | ANSI 颜色重置 |

默认 pattern: `[%D %T%e] [%C%L%R] [tid:%t] [%f:%#::%n] %g %m`

**JsonFormatter** — 输出 JSON 结构，包含所有字段 + 标签。

### LogContext（上下文管理）

```cpp
auto& ctx = br_logger::LogContext::Instance();

ctx.SetGlobalTag("env", "prod");
ctx.RemoveGlobalTag("env");
ctx.SetProcessName("my_app");
ctx.SetAppVersion("2.0.0");

br_logger::LogContext::SetThreadName("worker-1");

{
    br_logger::LogContext::ScopedTag tag("request_id", "abc-123");
    LOG_INFO("processing");
}
LOG_SCOPED_TAG("module", "network");
```

## 配置选项

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BR_LOG_BUILD_TESTS` | OFF | 编译单元测试 |
| `BR_LOG_BUILD_EXAMPLES` | OFF | 编译示例 |
| `BR_LOG_BUILD_BENCH` | OFF | 编译性能测试 |
| `BR_LOG_USE_FMTLIB` | OFF | 使用 fmtlib 替代 snprintf |
| `BR_LOG_BUILD_ROS2` | OFF | 编译 ROS2 扩展层（需 ROS2 humble 环境） |
| `BR_LOG_EMBEDDED_MODE` | OFF | 嵌入式裁剪模式 |

### 编译期宏

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `BR_LOG_ACTIVE_LEVEL` | 0 (Trace) | 编译期最低级别，低于此级别的日志代码被完全移除 |
| `BR_LOG_RING_SIZE` | 8192 | Ring buffer 容量（条目数） |
| `BR_LOG_MAX_MSG_LEN` | 512 | 单条消息最大字节数 |
| `BR_LOG_MAX_TAGS` | 16 | 每条日志最大标签数 |

编译期过滤示例（只保留 WARN 及以上）：

```bash
cmake -B build -DBR_LOG_ACTIVE_LEVEL=3
```

## ROS2 集成

br_logger 提供 `br_logger_ros2` 扩展包，与 ROS2 日志系统无缝桥接。

### 编译

```bash
# 需先 source ROS2 环境
source /opt/ros/humble/setup.bash
cmake -B build -DBR_LOG_BUILD_ROS2=ON
cmake --build build
```

### 一键初始化

```cpp
#include <br_logger_ros2/ros2_bridge.hpp>

// 在节点中初始化
br_logger::ros2::BridgeConfig config;
config.enable_ros2_sink = true;   // 桥接到 rcl 日志（/rosout）
config.enable_console = true;     // 同时输出到终端
config.enable_file = true;        // 同时写入文件
config.file_path = "/tmp/robot_logs/";
br_logger::ros2::init(node_shared_ptr, config);

// 关闭
br_logger::ros2::shutdown();
```

### 回调宏

在 ROS2 回调中自动附加上下文标签：

```cpp
#include <br_logger_ros2/ros2_macros.hpp>

void OnChatter(const std_msgs::msg::String::SharedPtr msg) {
    LOG_SUB_CALLBACK("/chatter");     // 自动注入 ros.topic, ros.cb_type
    LOG_INFO("received: %s", msg->data.c_str());
}

void OnTimer() {
    LOG_TIMER_CALLBACK("heartbeat");  // 自动注入 ros.timer, ros.cb_type
    LOG_INFO("tick");
}
```

可用宏：`LOG_SUB_CALLBACK`, `LOG_SRV_CALLBACK`, `LOG_TIMER_CALLBACK`, `LOG_ACTION_CALLBACK`

### 自动注入的上下文标签

初始化时 `ROS2ContextProvider` 自动注入以下全局标签：

| 标签 key | 来源 |
|----------|------|
| `ros.node` | 节点名 |
| `ros.namespace` | 节点命名空间 |
| `ros.domain_id` | DDS Domain ID |
| `ros.package` | CMake 宏 `BR_LOG_ROS2_PACKAGE_NAME` |
| `ros.executable` | /proc/self/exe |
| `ros.rmw` | RMW 实现名称 |

## 嵌入式裁剪

启用 `BR_LOG_EMBEDDED_MODE=ON` 时：

- 不编译后端线程代码（`#if BR_LOG_HAS_THREAD`）
- 不链接 pthread
- Ring buffer 缩小到 256 条
- 消息缓冲区缩小到 128 字节
- 通过 `Logger::Instance().Drain()` 在主循环中手动消费

```bash
cmake -B build -DBR_LOG_EMBEDDED_MODE=ON -DBR_LOG_ACTIVE_LEVEL=2
```

## 线程安全

| 操作 | 保证 |
|------|------|
| `LOG_*` 宏 | 线程安全（无锁 MPSC） |
| `Logger::SetLevel()` | 线程安全（atomic） |
| `Logger::AddSink()` | **非线程安全**，仅在 `Start()` 前调用 |
| `LogContext::SetGlobalTag()` | 线程安全（mutex） |
| `LogContext::SetThreadName()` | 仅在当前线程调用 |
| `ScopedTag` | 仅在当前线程有效（TLS） |

## 项目结构

```
br_logger_core/
├── include/br_logger/          # 核心头文件
├── src/                        # 实现文件
└── tests/                      # 186 个单元测试

br_logger_ros2/
├── include/br_logger_ros2/
│   ├── ros2_bridge.hpp         # 一键初始化 init()/shutdown()
│   ├── ros2_context.hpp        # ROS2ContextProvider 上下文注入
│   ├── ros2_sink.hpp           # ROS2Sink 桥接 rcutils_log
│   └── ros2_macros.hpp         # 回调便捷宏
├── src/                        # 实现文件
├── CMakeLists.txt              # ament_cmake 构建
└── package.xml                 # ROS2 包描述

examples/
├── basic_cpp/                  # 基础 C++ 示例
└── ros2_node/                  # ROS2 节点示例
```

## License

MIT
