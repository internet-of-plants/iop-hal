#include "iop-hal/panic.hpp"

#include <iostream>
#include <mutex>

static std::mutex stdoutMutex;

namespace iop_hal {
void logSetup() noexcept {}
void logPrint(const std::string_view msg) noexcept {
    std::lock_guard<std::mutex> guard(stdoutMutex);
    std::cout << msg;
}
void logPrint(const iop::StaticString msg) noexcept {
    std::lock_guard<std::mutex> guard(stdoutMutex);
    std::cout << msg.asCharPtr();
}
void logFlush() noexcept {
    std::lock_guard<std::mutex> guard(stdoutMutex);
    std::cout << std::flush;
}
}