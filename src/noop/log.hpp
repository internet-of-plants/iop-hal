#include "iop-hal/log.hpp"
#include "iop-hal/thread.hpp"

namespace iop_hal {
void logSetup() noexcept {}
void logPrint(const iop::StaticString msg) noexcept { (void) msg; }
void logPrint(const std::string_view msg) noexcept { (void) msg; }
void logFlush() noexcept {}
}