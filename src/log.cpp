#ifdef IOP_LINUX_MOCK
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

#include "iop-hal/network.hpp"
#include "iop-hal/device.hpp"
#include "iop-hal/wifi.hpp"
#include "iop-hal/thread.hpp"

static bool initialized = false;
static bool isTracing_ = false; 
static bool shouldFlush_ = true;

void iop::Log::shouldFlush(const bool flush) noexcept {
  shouldFlush_ = flush;
}

auto iop::Log::isTracing() noexcept -> bool { 
  return isTracing_;
}

constexpr static iop::LogHook defaultHook(iop::LogHook::defaultViewPrinter,
                                      iop::LogHook::defaultStaticPrinter,
                                      iop::LogHook::defaultSetuper,
                                      iop::LogHook::defaultFlusher);
static iop::LogHook hook = defaultHook;

namespace iop {
void IOP_RAM Log::setup(LogLevel level) noexcept { hook.setup(level); }
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
  initialized = false;
  auto old = hook;
  hook = defaultHook;
  return old;
}
void Log::setHook(LogHook newHook) noexcept {
  initialized = false;
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
  if (this->level_ > level)
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
  if (this->level_ > level)
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

void IOP_RAM LogHook::defaultStaticPrinter(
    const StaticString str, const LogLevel level, const LogType type) noexcept {
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
LogHook::defaultSetuper(const LogLevel level) noexcept {
  isTracing_ |= level == LogLevel::TRACE;
  static bool hasInitialized = false;
  const auto shouldInitialize = !hasInitialized;
  hasInitialized = true;
  if (shouldInitialize)
    iop_hal::logSetup(level);
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

Tracer::Tracer(CodePoint point, Log logger) noexcept : point(std::move(point)), logger(logger) {
  if (this->logger.level() != LogLevel::TRACE) return;

  this->logger.trace(IOP_STR("Entering new scope at line "),
                     std::to_string(this->point.line()),
                     IOP_STR(", in function "),
                     this->point.func(),
                     IOP_STR(", at file "),
                     this->point.file());
  
  {
    // Could this cause memory fragmentation?
    auto memory = iop_hal::thisThread.availableMemory();
    
    this->logger.trace(IOP_STR("Free stack "), std::to_string(memory.availableStack));

    for (const auto & item: memory.availableHeap) {
      this->logger.trace(IOP_STR("Free "), item.first, IOP_STR(" "), std::to_string(item.second));
    }
    for (const auto & item: memory.biggestHeapBlock) {
      this->logger.trace(IOP_STR("Biggest "), item.first, IOP_STR(" Block "), std::to_string(item.second));
    }
  }

  this->logger.trace(IOP_STR("Connection "), std::to_string(iop::wifi.status() == iop_hal::StationStatus::GOT_IP));
}
Tracer::~Tracer() noexcept {
  if (this->logger.level() != LogLevel::TRACE) return;

  this->logger.trace(IOP_STR("Leaving scope, at line "),
                     std::to_string(this->point.line()),
                     IOP_STR(", in function "),
                     this->point.func(),
                     IOP_STR(", at file "),
                     this->point.file());
}

void logMemory(const Log &logger) noexcept {
  if (logger.level() > LogLevel::INFO) return;

  {
    // Could this cause memory fragmentation?
    auto memory = iop_hal::thisThread.availableMemory();
    
    logger.info(IOP_STR("Free stack "), std::to_string(memory.availableStack));

    for (const auto & item: memory.availableHeap) {
      logger.info(IOP_STR("Free "), item.first, IOP_STR(" "), std::to_string(item.second));
    }
    for (const auto & item: memory.biggestHeapBlock) {
      logger.info(IOP_STR("Biggest "), item.first, IOP_STR(" Block "), std::to_string(item.second));
    }
  }

  logger.info(IOP_STR("Connection "), std::to_string(iop::wifi.status() == iop_hal::StationStatus::GOT_IP));
}
} // namespace iop