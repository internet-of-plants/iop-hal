#ifndef IOP_DRIVER_PANIC_HPP
#define IOP_DRIVER_PANIC_HPP

#include "iop-hal/log.hpp"

namespace iop {
/// Represents a panic interface that can be attached to the system
class PanicHook {
public:
  /// Represents a panic logger for runtime panic data
  using ViewPanic = void (*)(std::string_view const &, CodePoint const &);
  /// Represents a panic logger for compile time panic data
  using StaticPanic = void (*) (StaticString const &, CodePoint const &);
  /// Represents the complete process of a panic, calling halt on the end
  using Entry = void (*) (std::string_view const &, CodePoint const &);
  /// Represents the last step on a panic (that either halts, reboots or waits for somethin - like a binary update from the monitor server)
  using Halt = void (*) (std::string_view const &, CodePoint const &);
  /// Represents the cleanup function, cleaning all resources before halting (turn water pump off, etc)
  using Cleanup = void (*) ();

  ViewPanic viewPanic;
  StaticPanic staticPanic;
  Entry entry;
  Halt halt;
  Cleanup cleanup;

  /// Prints runtime panic data
  static void defaultViewPanic(std::string_view const &msg, CodePoint const &point) noexcept;
  /// Prints compile time panic data
  static void defaultStaticPanic(StaticString const &msg, CodePoint const &point) noexcept;
  /// Handles reentry, prints the panic message and calls halt
  static void defaultEntry(std::string_view const &msg, CodePoint const &point) noexcept;
  /// Halts the system, waiting for manual intervention (reboot and serial update)
  static void defaultHalt(std::string_view const &msg, CodePoint const &point) noexcept __attribute__((noreturn));
  /// Cleans-up resources before halting (turn water pump off, etc)
  static void defaultCleanup() noexcept {}

  constexpr PanicHook(ViewPanic view, StaticPanic progmem, Entry entry, Halt halt, Cleanup cleanup) noexcept
      : viewPanic(std::move(view)), staticPanic(std::move(progmem)),
        entry(std::move(entry)), halt(std::move(halt)), cleanup(std::move(cleanup)) {}
};

/// Sets new panic hook. Very useful to support panics that report to
/// the network, write to storage, and try to update, for example.
/// The default just prints to UART0 and halts
void setPanicHook(PanicHook hook) noexcept;

/// Removes current hook, replaces for default one (that just prints to UART0 and halts)
auto takePanicHook() noexcept -> PanicHook;

/// Initiates panic process for a runtime string (prefer calling `iop_panic` and `iop_assert` instead of this)
void panicHandler(std::string_view msg, CodePoint const &point) noexcept __attribute__((noreturn));
/// Initiates panic process for a compile time string (prefer calling `iop_panic` and `iop_assert` instead of this)
void panicHandler(StaticString msg, CodePoint const &point) noexcept __attribute__((noreturn));

class Log;
Log & panicLogger() noexcept;
} // namespace iop

/// Call panics hooks with specified message
/// Data: Message + Line + Function + File
///
/// Custom panic hooks can be set, to provide for network logging of the panic, storage logging, reporting of the stack trace, waiting for remote updates, etc
#define iop_panic(msg) ::iop::panicHandler((msg), IOP_CTX())

/// Calls `iop_panic` with provided message if condition is false
#define iop_assert(cond, msg)                                                  \
  if (!(cond))                                                                 \
    iop_panic(msg);

#endif