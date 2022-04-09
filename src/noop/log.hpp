#include "iop-hal/log.hpp"
#include "iop-hal/thread.hpp"

namespace driver {
void logSetup(const iop::LogLevel &level) noexcept { (void) level; }
void logPrint(const iop::StaticString msg) noexcept { (void) msg; }
void logPrint(const std::string_view msg) noexcept { (void) msg; }
void logFlush() noexcept {}
}