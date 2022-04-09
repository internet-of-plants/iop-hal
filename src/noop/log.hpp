#include "iop/log.hpp"
#include "iop/thread.hpp"

namespace driver {
void logSetup(const iop::LogLevel &level) noexcept {}
void logPrint(const iop::StaticString msg) noexcept {}
void logPrint(const std::string_view msg) noexcept {}
void logFlush() noexcept {}
}