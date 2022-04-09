#ifdef IOP_POSIX
#include "posix/thread.hpp"
#elif defined(IOP_ESP8266)
#include "esp8266/thread.hpp"
#elif defined(IOP_ESP32)
#include "esp32/thread.hpp"
#elif defined(IOP_NOOP)
#include "noop/thread.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/device.hpp"

namespace driver {
auto Thread::halt() const noexcept -> void {
    driver::device.deepSleep(0);
    this->abort();
}

Thread thisThread;
}