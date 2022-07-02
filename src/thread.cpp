#ifdef IOP_LINUX_MOCK
#include "posix/thread.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/esp/esp8266/thread.hpp"
#elif defined(IOP_ESP32)
#include "arduino/esp/esp32/thread.hpp"
#elif defined(IOP_NOOP)
#include "noop/thread.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/device.hpp"
#include "iop-hal/log.hpp"

namespace iop_hal {
auto Thread::halt() const noexcept -> void {
    IOP_TRACE();
    iop_hal::device.deepSleep(0);
    this->abort();
}

Thread thisThread;
}