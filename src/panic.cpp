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
  static iop::Log logger(IOP_STR("PANIC"));
  return logger;
}

auto panicHandler(std::string_view msg, CodePoint const &point) noexcept -> void {
  IOP_TRACE();
  hook.entry(msg, point);
  hook.viewPanic(msg, point);
  hook.halt(msg, point);
  iop_hal::thisThread.abort();
}

auto panicHandler(StaticString msg, CodePoint const &point) noexcept -> void {
  IOP_TRACE();
  const auto msg_ = msg.toString();
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  iop_hal::thisThread.abort();
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
auto setPanicHook(PanicHook newHook) noexcept -> void { hook = std::move(newHook); }

auto PanicHook::defaultViewPanic(std::string_view const &msg, CodePoint const &point) noexcept -> void {
  iop::panicLogger().crit(IOP_STR("Line "));
  iop::panicLogger().crit(point.line());
  iop::panicLogger().crit(IOP_STR(" of file "));
  iop::panicLogger().crit(point.file());
  iop::panicLogger().crit(IOP_STR(" inside "));
  iop::panicLogger().crit(point.func());
  iop::panicLogger().crit(IOP_STR(": "));
  iop::panicLogger().critln(msg);
}
void PanicHook::defaultStaticPanic(iop::StaticString const &msg, CodePoint const &point) noexcept {
  iop::panicLogger().crit(IOP_STR("Line "));
  iop::panicLogger().crit(point.line());
  iop::panicLogger().crit(IOP_STR(" of file "));
  iop::panicLogger().crit(point.file());
  iop::panicLogger().crit(IOP_STR(" inside "));
  iop::panicLogger().crit(point.func());
  iop::panicLogger().crit(IOP_STR(": "));
  iop::panicLogger().critln(msg);
}
void PanicHook::defaultEntry(std::string_view const &msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    iop::panicLogger().crit(IOP_STR("PANICK REENTRY: Line "));
    iop::panicLogger().crit(point.line());
    iop::panicLogger().crit(IOP_STR(" of file "));
    iop::panicLogger().crit(point.file());
    iop::panicLogger().crit(IOP_STR(" inside "));
    iop::panicLogger().crit(point.func());
    iop::panicLogger().crit(IOP_STR(": "));
    iop::panicLogger().critln(msg);
    iop::logMemory(iop::panicLogger());
    iop_hal::thisThread.halt();
  }
  isPanicking = true;

  constexpr const uint16_t oneSecond = 1000;
  iop_hal::thisThread.sleep(oneSecond);
}
void PanicHook::defaultHalt(std::string_view const &msg, CodePoint const &point) noexcept {
  (void)msg;
  (void)point;
  IOP_TRACE();
  iop_hal::thisThread.halt();
}
} // namespace iop
