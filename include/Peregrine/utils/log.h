#pragma once

// Logger wrapper:
//  - If edgehermes_ENABLE_LOGGING is defined and spdlog is available, use spdlog.
//  - Otherwise provide a no-op Logger and a minimal spdlog::level::level_enum so callers compile.

#include <memory>
#include <string>

// Prefer spdlog when logging is enabled and available
#if defined(edgehermes_ENABLE_LOGGING) && edgehermes_ENABLE_LOGGING && __has_include(<spdlog/spdlog.h>)

#if __has_include(<fmt/ostream.h>)
#include <fmt/ostream.h>
#elif __has_include(<spdlog/fmt/ostr.h>)
#include <spdlog/fmt/ostr.h>
#endif
#include <spdlog/spdlog.h>

#include <filesystem>

namespace edgehermes {

class Logger {
 public:
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }

  void init(const std::string& name = "EdgeHermes", const std::string& logFile = "EdgeHermes.log", spdlog::level::level_enum level = spdlog::level::info);

  void setLevel(spdlog::level::level_enum level) {
    if (logger_) logger_->set_level(level);
  }

  template <typename... Args>
  void trace(const char* fmt, const Args&... args) {
    if (logger_) logger_->trace(fmt, args...);
  }

  template <typename... Args>
  void debug(const char* fmt, const Args&... args) {
    if (logger_) logger_->debug(fmt, args...);
  }

  template <typename... Args>
  void info(const char* fmt, const Args&... args) {
    if (logger_) logger_->info(fmt, args...);
  }

  template <typename... Args>
  void warn(const char* fmt, const Args&... args) {
    if (logger_) logger_->warn(fmt, args...);
  }

  template <typename... Args>
  void error(const char* fmt, const Args&... args) {
    if (logger_) logger_->error(fmt, args...);
  }

  template <typename... Args>
  void critical(const char* fmt, const Args&... args) {
    if (logger_) logger_->critical(fmt, args...);
  }

  void flush() {
    if (logger_) logger_->flush();
  }

 private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace edgehermes

#else

// spdlog not available or logging disabled -> provide no-op Logger

// Minimal spdlog::level namespace so callers using spdlog::level::info still compile
namespace spdlog {
namespace level {
enum level_enum { trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5 };
}  // namespace level
}  // namespace spdlog

namespace edgehermes {

class Logger {
 public:
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }

  void init(const std::string& /*name*/ = "EdgeHermes", const std::string& /*logFile*/ = "EdgeHermes.log", spdlog::level::level_enum /*level*/ = spdlog::level::info);

  void setLevel(spdlog::level::level_enum /*level*/) {}

  template <typename... Args>
  void trace(const char* /*fmt*/, const Args&... /*args*/) {}

  template <typename... Args>
  void debug(const char* /*fmt*/, const Args&... /*args*/) {}

  template <typename... Args>
  void info(const char* /*fmt*/, const Args&... /*args*/) {}

  template <typename... Args>
  void warn(const char* /*fmt*/, const Args&... /*args*/) {}

  template <typename... Args>
  void error(const char* /*fmt*/, const Args&... /*args*/) {}

  template <typename... Args>
  void critical(const char* /*fmt*/, const Args&... /*args*/) {}

  void flush() {}

 private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
};

}  // namespace edgehermes

#endif

// Convenience macros (same API in both branches)
#define LOG_TRACE(...) edgehermes::Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) edgehermes::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) edgehermes::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) edgehermes::Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) edgehermes::Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) edgehermes::Logger::getInstance().critical(__VA_ARGS__)

// Initialize logger macro
#define LOG_INIT(name, logFile, level) edgehermes::Logger::getInstance().init(name, logFile, level)

// Set log level macro
#define LOG_SET_LEVEL(level) edgehermes::Logger::getInstance().setLevel(level)

// Flush logs macro
#define LOG_FLUSH() edgehermes::Logger::getInstance().flush()




