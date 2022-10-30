#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "cpp17/log.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/log.hpp"
#elif defined(IOP_ESP32)
#include "arduino/log.hpp"
#elif defined(IOP_NOOP)
#ifdef ARDUINO
#include "arduino/log.hpp"
#else
#include "cpp17/log.hpp"
#endif
#else
#error "Target not supported"
#endif

#include "iop-hal/log.hpp"
#include "iop-hal/network.hpp"
#include "iop-hal/device.hpp"
#include "iop-hal/wifi.hpp"
#include "iop-hal/thread.hpp"

static bool hasInitialized = false;
static bool shouldFlush_ = true;

void iop::Log::shouldFlush(const bool flush) noexcept {
  shouldFlush_ = flush;
}

auto iop::Log::isTracing() const noexcept -> bool {
  return hasInitialized && this->level() <= iop::LogLevel::TRACE;
}

constexpr static iop::LogHook defaultHook(iop::LogHook::defaultViewPrinter,
                                      iop::LogHook::defaultStaticPrinter,
                                      iop::LogHook::defaultSetuper,
                                      iop::LogHook::defaultFlusher);
static iop::LogHook hook = defaultHook;

namespace iop {
void IOP_RAM Log::setup() noexcept { hook.setup(); }
void Log::flush() noexcept { if (shouldFlush_) hook.flush(); }
void IOP_RAM Log::print(const std::string_view view, const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.viewPrint(view, level, kind);
  else
    hook.traceViewPrint(view, level, kind);
}
void IOP_RAM Log::print(const StaticString progmem,
                                const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.staticPrint(progmem, level, kind);
  else
    hook.traceStaticPrint(progmem, level, kind);
}
auto Log::takeHook() noexcept -> LogHook {
  hasInitialized = false;
  auto old = hook;
  hook = defaultHook;
  return old;
}
void Log::setHook(LogHook newHook) noexcept {
  hasInitialized = false;
  hook = std::move(newHook);
}

void Log::printLogType(const LogType &logType,
                       const LogLevel &level) const noexcept {
  if (level == LogLevel::NO_LOG)
    return;

  switch (logType) {
  case LogType::CONTINUITY:
  case LogType::END:
    break;

  case LogType::START:
  case LogType::STARTEND:
    Log::print(IOP_STR("["), level, LogType::START);
    Log::print(this->levelToString(level).get(), level, LogType::CONTINUITY);
    Log::print(IOP_STR("] "), level, LogType::CONTINUITY);
    Log::print(this->target_.get(), level, LogType::CONTINUITY);
    Log::print(IOP_STR(": "), level, LogType::CONTINUITY);
  };
}

void Log::log(const LogLevel &level, const StaticString &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level() > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

void Log::log(const LogLevel &level, const std::string_view &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level() > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

auto Log::levelToString(const LogLevel level) const noexcept -> StaticString {
  switch (level) {
  case LogLevel::TRACE:
    return IOP_STR("TRACE");
  case LogLevel::DEBUG:
    return IOP_STR("DEBUG");
  case LogLevel::INFO:
    return IOP_STR("INFO");
  case LogLevel::WARN:
    return IOP_STR("WARN");
  case LogLevel::ERROR:
    return IOP_STR("ERROR");
  case LogLevel::CRIT:
    return IOP_STR("CRIT");
  case LogLevel::NO_LOG:
    return IOP_STR("NO_LOG");
  }
  return IOP_STR("UNKNOWN");
}

void IOP_RAM LogHook::defaultStaticPrinter(const StaticString str, const LogLevel level, const LogType type) noexcept {
  iop_hal::logPrint(str);
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultViewPrinter(const std::string_view str, const LogLevel level, const LogType type) noexcept {
  iop_hal::logPrint(str);
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultSetuper() noexcept {
  if (!hasInitialized) {
    hasInitialized = true;
    iop_hal::logSetup();
  }
}
void LogHook::defaultFlusher() noexcept {
  iop_hal::logFlush();
}
// NOLINTNEXTLINE *-use-equals-default
LogHook::LogHook(LogHook const &other) noexcept
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
LogHook::LogHook(LogHook &&other) noexcept
    // NOLINTNEXTLINE cert-oop11-cpp cert-oop54-cpp *-move-constructor-init
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
auto LogHook::operator=(LogHook const &other) noexcept -> LogHook & {
  if (this == &other)
    return *this;
  this->viewPrint = other.viewPrint;
  this->staticPrint = other.staticPrint;
  this->setup = other.setup;
  this->flush = other.flush;
  this->traceViewPrint = other.traceViewPrint;
  this->traceStaticPrint = other.traceStaticPrint;
  return *this;
}
auto LogHook::operator=(LogHook &&other) noexcept -> LogHook & {
  *this = other;
  return *this;
}

constexpr static char TRACER_NAME_RAW[] IOP_ROM = "TRACER";
static const iop::StaticString TRACER_NAME = reinterpret_cast<const __FlashStringHelper*>(TRACER_NAME_RAW);
static Log logger(TRACER_NAME);

Tracer::Tracer(CodePoint point) noexcept : point(point) {
  if (!logger.isTracing()) return;
  logger.trace(IOP_STR("Entering new scope at line "));
  logger.trace(this->point.line());
  logger.trace(IOP_STR(", in function "));
  logger.trace(this->point.func());
  logger.trace(IOP_STR(", at file "));
  logger.traceln(this->point.file());
  
  {
    // Could this cause memory fragmentation?
    auto memory = iop_hal::thisThread.availableMemory();
    
    logger.trace(IOP_STR("Free stack "));
    logger.traceln(memory.availableStack);

    for (const auto & item: memory.availableHeap) {
      logger.trace(IOP_STR("Free "));
      logger.trace(item.first);
      logger.trace(IOP_STR(" "));
      logger.traceln(item.second);
    }
    for (const auto & item: memory.biggestHeapBlock) {
      logger.trace(IOP_STR("Biggest "));
      logger.trace(item.first);
      logger.trace(IOP_STR(" Block "));
      logger.traceln(item.second);
    }
  }

  logger.trace(IOP_STR("Connection "));
  logger.traceln(iop::wifi.status() == iop_hal::StationStatus::GOT_IP);
}
Tracer::~Tracer() noexcept {
  if (!logger.isTracing()) return;
  logger.trace(IOP_STR("Leaving scope, at line "));
  logger.trace(this->point.line());
  logger.trace(IOP_STR(", in function "));
  logger.trace(this->point.func());
  logger.trace(IOP_STR(", at file "));
  logger.traceln(this->point.file());
}

void logMemory(Log &logger) noexcept {
  if (logger.level() > LogLevel::INFO) return;

  {
    // Could this cause memory fragmentation?
    auto memory = iop_hal::thisThread.availableMemory();
    
    logger.info(IOP_STR("Free stack "));
    logger.infoln(memory.availableStack);

    for (const auto & item: memory.availableHeap) {
      logger.info(IOP_STR("Free "));
      logger.info(item.first);
      logger.info(IOP_STR(" "));
      logger.infoln(item.second);
    }
    for (const auto & item: memory.biggestHeapBlock) {
      logger.info(IOP_STR("Biggest "));
      logger.info(item.first);
      logger.info(IOP_STR(" Block "));
      logger.infoln(item.second);
    }
  }

  logger.info(IOP_STR("Connection "));
  logger.infoln(iop::wifi.status() == iop_hal::StationStatus::GOT_IP);
}
} // namespace iop