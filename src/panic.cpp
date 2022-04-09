#include "iop-hal/panic.hpp"
#include "iop-hal/log.hpp"
#include "iop-hal/device.hpp"
#include "iop-hal/thread.hpp"

// TODO: implement panic backend that reboots, we always panic on OOM, but because of heap fragmentation you generally would want to reboot to maximize uptime

static bool isPanicking = false;

constexpr static iop::PanicHook defaultHook(iop::PanicHook::defaultViewPanic,
                                        iop::PanicHook::defaultStaticPanic,
                                        iop::PanicHook::defaultEntry,
                                        iop::PanicHook::defaultHalt);

static iop::PanicHook hook(defaultHook);

namespace iop {
auto panicLogger() noexcept -> Log & {
  static iop::Log logger(iop::LogLevel::CRIT, IOP_STR("PANIC"));
  return logger;
}

auto panicHandler(std::string_view msg, CodePoint const &point) noexcept -> void {
  IOP_TRACE();
  hook.entry(msg, point);
  hook.viewPanic(msg, point);
  hook.halt(msg, point);
  driver::thisThread.abort();
}

auto panicHandler(StaticString msg, CodePoint const &point) noexcept -> void {
  IOP_TRACE();
  const auto msg_ = msg.toString();
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  driver::thisThread.abort();
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
auto setPanicHook(PanicHook newHook) noexcept -> void { hook = std::move(newHook); }

auto PanicHook::defaultViewPanic(std::string_view const &msg, CodePoint const &point) noexcept -> void {
  iop::panicLogger().crit(IOP_STR("Line "), ::std::to_string(point.line()), IOP_STR(" of file "), point.file(),
              IOP_STR(" inside "), point.func(), IOP_STR(": "), msg);
}
void PanicHook::defaultStaticPanic(iop::StaticString const &msg, CodePoint const &point) noexcept {
  iop::panicLogger().crit(IOP_STR("Line "), ::std::to_string(point.line()), IOP_STR(" of file "), point.file(),
              IOP_STR(" inside "), point.func(), IOP_STR(": "), msg);
}
void PanicHook::defaultEntry(std::string_view const &msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    iop::panicLogger().crit(IOP_STR("PANICK REENTRY: Line "), std::to_string(point.line()),
                IOP_STR(" of file "), point.file(), IOP_STR(" inside "), point.func(),
                IOP_STR(": "), msg);
    iop::logMemory(iop::panicLogger());
    driver::thisThread.halt();
  }
  isPanicking = true;

  constexpr const uint16_t oneSecond = 1000;
  driver::thisThread.sleep(oneSecond);
}
void PanicHook::defaultHalt(std::string_view const &msg, CodePoint const &point) noexcept {
  (void)msg;
  (void)point;
  IOP_TRACE();
  driver::thisThread.halt();
}
} // namespace iop