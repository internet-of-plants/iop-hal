#ifndef IOP_DRIVER_LOG_HPP
#define IOP_DRIVER_LOG_HPP

#include "iop/string.hpp"
#include <optional>

/// Helps getting metadata on the executed code (at compile time)
#define IOP_FILE ::iop::StaticString(reinterpret_cast<const __FlashStringHelper*>(__FILE__))
#define IOP_LINE static_cast<uint32_t>(__LINE__)
#define IOP_FUNC ::iop::StaticString(reinterpret_cast<const __FlashStringHelper*>(__PRETTY_FUNCTION__))

/// Returns CodePoint object pointing to the caller
/// This is useful to track callers of functions that can panic
/// C++20 solves this need, but it's not supported in _most_ platforms
#define IOP_CTX() IOP_CODE_POINT()
#define IOP_CODE_POINT() ::iop::CodePoint(IOP_FILE, IOP_LINE, IOP_FUNC)

/// Logs scope changes to serial if logLevel is set to TRACE
/// FIXME: Sadly tracing is a global property for now, it should be fixed
#define IOP_TRACE2(logger) IOP_TRACE_INNER(__COUNTER__, logger)
#define IOP_TRACE() IOP_TRACE_INNER(__COUNTER__, iop::Log(iop::Log::isTracing() ? iop::LogLevel::TRACE : iop::LogLevel::DEBUG, IOP_STR("TRACER")))
// Technobabble to stringify __COUNTER__
#define IOP_TRACE_INNER(x, logger) IOP_TRACE_INNER2(x, logger)
#define IOP_TRACE_INNER2(x, logger) const ::iop::Tracer iop_tracer_##x(IOP_CODE_POINT(), logger);

namespace iop {

/// Specifies logging level hierarchy
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };

/// Internal structure to allow dynamically building logs
/// Helpful for properly displaying logging metadata, but also for network/storage logging
enum class LogType { START, CONTINUITY, STARTEND, END };

/// Represents an individual point in the codebase (used to track callers of panics)
class CodePoint {
  uint32_t line_;
  StaticString file_;
  StaticString func_;

public:
  /// Use the `IOP_CODE_POINT()` instead of instantiating it manually.
  CodePoint(StaticString file, uint32_t line, StaticString func) noexcept
      : line_(line),file_(file), func_(func) {}

  auto file() const noexcept -> StaticString { return this->file_; }
  auto line() const noexcept -> uint32_t { return this->line_; }
  auto func() const noexcept -> StaticString { return this->func_; }
};

/// Represents a logging interface that can be attached to the system
class LogHook {
public:
  /// Primitive to print runtime strings
  using ViewPrinter = void (*) (std::string_view, LogLevel, LogType);
  /// Primitive to print compile time strings
  using StaticPrinter = void (*) (StaticString, LogLevel, LogType);
  /// Primitive to initialize the logger
  using Setuper = void (*) (LogLevel);
  /// Primitive to flush the logged data
  using Flusher = void (*) ();

  /// Primitive to print runtime strings that garantees not to yield (can be run from interrupts)
  using TraceViewPrinter = ViewPrinter;
  /// Primitive to print compile time strings that garantees not to yield (can be run from interrupts)
  using TraceStaticPrinter = StaticPrinter;

  ViewPrinter viewPrint;
  StaticPrinter staticPrint;
  Setuper setup;
  Flusher flush;
  TraceViewPrinter traceViewPrint;
  TraceStaticPrinter traceStaticPrint;

  /// Prints runtime string.
  ///
  /// Interrupt safe
  static void defaultViewPrinter(std::string_view, LogLevel level, LogType type) noexcept;
  /// Prints compile time string.
  ///
  /// Interrupt safe
  static void defaultStaticPrinter(StaticString, LogLevel level,
                                   LogType type) noexcept;
  static void defaultSetuper(iop::LogLevel level) noexcept;
  static void defaultFlusher() noexcept;

  constexpr LogHook(LogHook::ViewPrinter viewPrinter,
                 LogHook::StaticPrinter staticPrinter, LogHook::Setuper setuper,
                 LogHook::Flusher flusher) noexcept
    : viewPrint(std::move(viewPrinter)), staticPrint(std::move(staticPrinter)),
      setup(std::move(setuper)), flush(std::move(flusher)),
      traceViewPrint(defaultViewPrinter),
      traceStaticPrint(defaultStaticPrinter) {}

  // Allows specifying a custom implementation of the log interface. Tracing functions must be interrupt safe.
  constexpr LogHook(LogHook::ViewPrinter viewPrinter,
                  LogHook::StaticPrinter staticPrinter, LogHook::Setuper setuper,
                  LogHook::Flusher flusher,
                  LogHook::TraceViewPrinter traceViewPrint,
                  LogHook::TraceStaticPrinter traceStaticPrint) noexcept
      : viewPrint(std::move(viewPrinter)), staticPrint(std::move(staticPrinter)),
        setup(std::move(setuper)), flush(std::move(flusher)),
        traceViewPrint(std::move(traceViewPrint)),
        traceStaticPrint(std::move(traceStaticPrint)) {}
  ~LogHook() noexcept = default;
  LogHook(LogHook const &other) noexcept;
  LogHook(LogHook &&other) noexcept;
  auto operator=(LogHook const &other) noexcept -> LogHook &;
  auto operator=(LogHook &&other) noexcept -> LogHook &;
};

/// Logger structure, contains its log level and log target
class Log {
  LogLevel level_;
  StaticString target_;

public:
  Log(const LogLevel &level, StaticString target) noexcept
      : level_{level}, target_(std::move(target)) {}

  /// Replaces current hook for the argument. 
  /// It's very useful to support other logging channels, like network or storage.
  ///
  /// The default just prints to `UART0`. Assume it has been initialized.
  /// So if you don't want to use `UART0` undefine `IOP_SERIAL` to turn the default hooks into noops.
  static void setHook(LogHook hook) noexcept;

  /// Removes current hook, replaces for default one (that just prints to UART0)
  static auto takeHook() noexcept -> LogHook;

  auto level() const noexcept -> LogLevel { return this->level_; }
  auto target() const noexcept -> StaticString { return this->target_; }
  
  /// Toggles global flushing setting, defines if system flushes after every complete log
  /// (until a `iop::LogType::END` or a `iop::LogType::STARTEND`)
  static void shouldFlush(bool flush) noexcept;

  /// Returns true if any global logger has started tracing (this is not ideal, it should be local)
  // TODO: stop with the global tracing
  static auto isTracing() noexcept -> bool;

  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::TRACE`
  /// Message is ignored if the logger has a lower level than `iop::LogType::TRACE`
  template <typename... Args> void trace(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::TRACE, true, args...);
  }
  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::DEBUG`
  /// Message is ignored if the logger has a lower level than `iop::LogType::DEBUG`
  template <typename... Args> void debug(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::DEBUG, true, args...);
  }
  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::INFO`
  /// Message is ignored if the logger has a lower level than `iop::LogType::INFO`
  template <typename... Args> void info(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::INFO, true, args...);
  }
  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::WARN`
  /// Message is ignored if the logger has a lower level than `iop::LogType::WARN`
  template <typename... Args> void warn(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::WARN, true, args...);
  }
  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::ERROR`
  /// Message is ignored if the logger has a lower level than `iop::LogType::ERROR`
  template <typename... Args> void error(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::ERROR, true, args...);
  }
  /// Logs a sequence of runtime and compile time strings as a line, as `iop::LogType::CRIT`
  /// Message is ignored if the logger has a lower level than `iop::LogType::CRIT`
  template <typename... Args> void crit(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::CRIT, true, args...);
  }

  /// Primitive that allows printing an individual compile time string according to the log level
  static void print(StaticString progmem, LogLevel level, LogType kind) noexcept;
  /// Primitive that allows printing an individual runtime string according to the log level
  static void print(std::string_view view, LogLevel level, LogType kind) noexcept;
  /// Primitive that flushes the log
  static void flush() noexcept;
  /// Primitive that initializes the log, gets a log level as parameter to enable global tracing as needed
  // TODO: stop with the global tracing
  static void setup(LogLevel level) noexcept;

private:
  // "Recursive" internal variadic function 
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, IOP_STR(""));
    } else {
      this->log(level, msg, LogType::CONTINUITY, IOP_STR(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, IOP_STR("\n"));
    } else {
      this->log(level, msg, LogType::END, IOP_STR("\n"));
    }
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const std::string_view msg, const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, IOP_STR(""));
    } else {
      this->log(level, msg, LogType::CONTINUITY, IOP_STR(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const std::string_view msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, IOP_STR("\n"));
    } else {
      this->log(level, msg, LogType::END, IOP_STR("\n"));
    }
  }

  void printLogType(const LogType &logType, const LogLevel &level) const noexcept;
  auto levelToString(LogLevel level) const noexcept -> StaticString;

  void log(const LogLevel &level, const StaticString &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;
  void log(const LogLevel &level, const std::string_view &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;
};


/// Tracer objects, logs scope changes. Useful for debugging.
///
/// Doesn't use the official logging system to avoid clutter.
// TODO: use official logging and stop with global tracing
class Tracer {
  CodePoint point;
  Log logger;

public:
  /// Use `IOP_TRACE()` instead of instantiating it manually.
  explicit Tracer(CodePoint point, iop::Log logger) noexcept;
  ~Tracer() noexcept;

  // Don't move or copy it, just use it as a scope logging guard
  Tracer(const Tracer &other) noexcept = delete;
  Tracer(Tracer &&other) noexcept = delete;
  auto operator=(const Tracer &other) noexcept -> Tracer & = delete;
  auto operator=(Tracer &&other) noexcept -> Tracer & = delete;
};

void logMemory(const iop::Log &logger) noexcept;
} // namespace iop

namespace driver {
// Internals, don't use them. Use the Log abstraction instead
void logSetup(const iop::LogLevel &level) noexcept;
void logPrint(const std::string_view msg) noexcept;
void logPrint(const iop::StaticString msg) noexcept;
void logFlush() noexcept;
} // namespace driver

#endif