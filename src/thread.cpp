#ifdef IOP_POSIX
#include "driver/posix/thread.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/thread.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/thread.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/thread.hpp"
#else
#error "Target not supported"
#endif

#include "driver/device.hpp"

namespace driver {
    auto Thread::halt() const noexcept -> void {
        driver::device.deepSleep(0);
        this->abort();
    }

    Thread thisThread;
}