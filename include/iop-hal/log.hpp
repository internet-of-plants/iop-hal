#ifndef IOP_DRIVER_LOG_HPP
#define IOP_DRIVER_LOG_HPP

#include "iop-hal/string.hpp"
#include <optional>

#ifndef IOP_LOG_LEVEL
#define IOP_LOG_LEVEL iop::LogLevel::INFO
#endif

/// Helps getting metadata on the executed code (at compile time)
#define IOP_FILE ::iop::StaticString(reinterpret_cast<const __FlashStringHelper*>(__FILE__))
#define IOP_LINE static_cast<uint16_t>(__LINE__)
#define IOP_FUNC ::iop::StaticString(reinterpret_cast<const __FlashStringHelper*>(__PRETTY_FUNCTION__))

/// Returns CodePoint object pointing to the caller
/// This is useful to track callers of functions that can panic
/// C++20 solves this need, but it's not supported in _most_ platforms
#define IOP_CTX() IOP_CODE_POINT()
#define IOP_CODE_POINT() ::iop::CodePoint(IOP_FILE, IOP_LINE, IOP_FUNC)

/// Logs scope changes to serial if logLevel is set to TRACE
#define IOP_TRACE() IOP_TRACE_INNER(__COUNTER__)
// Technobabble to stringify __COUNTER__
#define IOP_TRACE_INNER(x) IOP_TRACE_INNER2(x)
#define IOP_TRACE_INNER2(x) const ::iop::Tracer iop_tracer_##x(IOP_CODE_POINT());

namespace iop {

/// Specifies logging level hierarchy
enum class LogLevel: uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };

/// Internal structure to allow dynamically building logs
/// Helpful for properly displaying logging metadata, but also for network/storage logging
enum class LogType: uint8_t { START, CONTINUITY, STARTEND, END };

/// Represents an individual point in the codebase (used to track callers of panics)
class CodePoint {
  uint16_t line_;
  StaticString file_;
  StaticString func_;

public:
  /// Use the `IOP_CODE_POINT()` instead of instantiating it manually.
  CodePoint(StaticString file, uint16_t line, StaticString func) noexcept
      : line_(line),file_(file), func_(func) {}

  auto file() const noexcept -> StaticString { return this->file_; }
  auto line() const noexcept -> uint16_t { return this->line_; }
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
  using Setuper = void (*) ();
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
  static void defaultSetuper() noexcept;
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

class Tracer;

extern LogType state;

/// Logger structure, contains its log level and log target
class Log {
  StaticString target_;

public:
  Log(const StaticString target) noexcept
      : target_(target) {}

  /// Replaces current hook for the argument.
  /// It's very useful to support other logging channels, like network or storage.
  ///
  /// The default just prints to `UART0`. Assume it has been initialized.
  /// So if you don't want to use `UART0` undefine `IOP_SERIAL` to turn the default hooks into noops.
  static void setHook(LogHook hook) noexcept;

  /// Removes current hook, replaces for default one (that just prints to UART0)
  static auto takeHook() noexcept -> LogHook;

  auto level() const noexcept -> LogLevel { return IOP_LOG_LEVEL; }
  auto target() const noexcept -> StaticString { return this->target_; }

  /// Toggles global flushing setting, defines if system flushes after every complete log
  /// (until a `iop::LogType::END` or a `iop::LogType::STARTEND`)
  static void shouldFlush(bool flush) noexcept;

  /// Returns true if any global logger has started tracing (this is not ideal, it should be local)
  // TODO: stop with the global tracing
  auto isTracing() const noexcept -> bool;

  auto updateState() noexcept -> void {
    switch (state) {
      case LogType::START:
        state = LogType::CONTINUITY;
        break;
      case LogType::CONTINUITY:
        break;
      case LogType::STARTEND:
        state = LogType::START;
        break;
      case LogType::END:
        state = LogType::START;
        break;
    }
  }
  auto finalizeState() noexcept -> void {
    switch (state) {
      case LogType::START:
        state = LogType::END;
        break;
      case LogType::CONTINUITY:
        state = LogType::END;
        break;
      case LogType::STARTEND:
        state = LogType::STARTEND;
        break;
      case LogType::END:
        state = LogType::STARTEND;
        break;
    }
  }

  // TODO: fix wdt reset when logging function is called before Log::setup is

  auto trace(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR(""));
  }
  auto trace(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR(""));
  }
  auto trace(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR(""));
  }
  auto trace(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, iop::to_view(msg), state, IOP_STR(""));
  }
  auto trace(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto trace(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR(""));
  }
  auto traceln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR("\n"));
  }
  auto traceln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR("\n"));
  }
  auto traceln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, msg, state, IOP_STR("\n"));
  }
  auto traceln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto traceln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto traceln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::TRACE, std::to_string(msg), state, IOP_STR("\n"));
  }

  auto debug(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR(""));
  }
  auto debug(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR(""));
  }
  auto debug(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR(""));
  }
  auto debug(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, iop::to_view(msg), state, IOP_STR(""));
  }
  auto debug(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debug(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR(""));
  }
  auto debugln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR("\n"));
  }
  auto debugln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR("\n"));
  }
  auto debugln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, msg, state, IOP_STR("\n"));
  }
  auto debugln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto debugln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto debugln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::DEBUG, std::to_string(msg), state, IOP_STR("\n"));
  }

  auto info(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, msg, state, IOP_STR(""));
  }
  auto info(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, msg, state, IOP_STR(""));
  }
  auto info(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, msg, state, IOP_STR(""));
  }
  auto info(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, iop::to_view(msg), state, IOP_STR(""));
  }
  auto info(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto info(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR(""));
  }
  auto infoln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, msg, state, IOP_STR("\n"));
  }
  auto infoln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, msg, state, IOP_STR("\n"));
  }
  auto infoln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, msg, state, IOP_STR("\n"));
  }
  auto infoln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto infoln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto infoln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::INFO, std::to_string(msg), state, IOP_STR("\n"));
  }

  auto warn(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, msg, state, IOP_STR(""));
  }
  auto warn(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, msg, state, IOP_STR(""));
  }
  auto warn(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, msg, state, IOP_STR(""));
  }
  auto warn(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, iop::to_view(msg), state, IOP_STR(""));
  }
  auto warn(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warn(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR(""));
  }
  auto warnln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, msg, state, IOP_STR("\n"));
  }
  auto warnln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, msg, state, IOP_STR("\n"));
  }
  auto warnln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, msg, state, IOP_STR("\n"));
  }
  auto warnln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto warnln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto warnln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::WARN, std::to_string(msg), state, IOP_STR("\n"));
  }

  auto error(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR(""));
  }
  auto error(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR(""));
  }
  auto error(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR(""));
  }
  auto error(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, iop::to_view(msg), state, IOP_STR(""));
  }
  auto error(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto error(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR(""));
  }
  auto errorln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR("\n"));
  }
  auto errorln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR("\n"));
  }
  auto errorln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, msg, state, IOP_STR("\n"));
  }
  auto errorln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto errorln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto errorln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::ERROR, std::to_string(msg), state, IOP_STR("\n"));
  }

  auto crit(const StaticString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR(""));
  }
  auto crit(const std::string_view msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR(""));
  }
  auto crit(const std::string msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR(""));
  }
  auto crit(const CowString msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, iop::to_view(msg), state, IOP_STR(""));
  }
  auto crit(const uint8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const int8_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const uint16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const int16_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const uint32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const int32_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const uint64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto crit(const int64_t msg) noexcept -> void {
    this->updateState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR(""));
  }
  auto critln(const StaticString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR("\n"));
  }
  auto critln(const std::string_view msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR("\n"));
  }
  auto critln(const std::string msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, msg, state, IOP_STR("\n"));
  }
  auto critln(const CowString msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, iop::to_view(msg), state, IOP_STR("\n"));
  }
  auto critln(const uint8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const int8_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const uint16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const int16_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const uint32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const int32_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const uint64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }
  auto critln(const int64_t msg) noexcept -> void {
    this->finalizeState();
    this->log(LogLevel::CRIT, std::to_string(msg), state, IOP_STR("\n"));
  }

  /// Primitive that allows printing an individual compile time string according to the log level
  static void print(StaticString progmem, LogLevel level, LogType kind) noexcept;
  /// Primitive that allows printing an individual runtime string according to the log level
  static void print(std::string_view view, LogLevel level, LogType kind) noexcept;
  /// Primitive that flushes the log
  static void flush() noexcept;
  /// Primitive that initializes the log, gets a log level as parameter to enable global tracing as needed
  // TODO: stop with the global tracing
  static void setup() noexcept;

private:
  void printLogType(const LogType &logType, const LogLevel &level) const noexcept;
  auto levelToString(LogLevel level) const noexcept -> StaticString;

  void log(const LogLevel &level, const StaticString &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;
  void log(const LogLevel &level, const std::string_view &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;

  friend Tracer;
};


/// Tracer objects, logs scope changes. Useful for debugging.
///
/// Doesn't use the official logging system to avoid clutter.
// TODO: use official logging and stop with global tracing
class Tracer {
  CodePoint point;

public:
  /// Use `IOP_TRACE()` instead of instantiating it manually.
  explicit Tracer(CodePoint point) noexcept;
  ~Tracer() noexcept;

  // Don't move or copy it, just use it as a scope logging guard
  Tracer(const Tracer &other) noexcept = delete;
  Tracer(Tracer &&other) noexcept = delete;
  auto operator=(const Tracer &other) noexcept -> Tracer & = delete;
  auto operator=(Tracer &&other) noexcept -> Tracer & = delete;
};

void logMemory(iop::Log &logger) noexcept;
} // namespace iop

namespace iop_hal {
// Internals, don't use them. Use the Log abstraction instead
void logSetup() noexcept;
void logPrint(const std::string_view msg) noexcept;
void logPrint(const iop::StaticString msg) noexcept;
void logFlush() noexcept;
} // namespace iop_hal

#endif
