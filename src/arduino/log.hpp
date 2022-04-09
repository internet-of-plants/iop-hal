#include "iop-hal/log.hpp"
#include "iop-hal/thread.hpp"

#include <Arduino.h>

//HardwareSerial Serial(UART0);

namespace iop_hal {
void logSetup(const iop::LogLevel &level) noexcept {
    constexpr const uint32_t BAUD_RATE = 115200;
    Serial.begin(BAUD_RATE);
    if (level <= iop::LogLevel::DEBUG)
        Serial.setDebugOutput(true);

    constexpr const uint32_t twoSec = 2 * 1000;
    const auto end = iop_hal::thisThread.timeRunning() + twoSec;
    while (!Serial && iop_hal::thisThread.timeRunning() < end)
        iop_hal::thisThread.yield();
}
void logPrint(const iop::StaticString msg) noexcept {
    Serial.print(msg.get());
}
void logPrint(const std::string_view msg) noexcept {
    Serial.write(reinterpret_cast<const char*>(msg.begin()), msg.length());
}
void logFlush() noexcept {
    Serial.flush();
}
}