#ifdef IOP_POSIX
#include "iop/posix/thread.hpp"
#elif defined(IOP_ESP8266)
#include "iop/esp8266/thread.hpp"
#elif defined(IOP_ESP32)
#include "iop/esp32/thread.hpp"
#elif defined(IOP_NOOP)
#include "iop/noop/thread.hpp"
#else
#error "Target not supported"
#endif

#include "iop/device.hpp"

namespace driver {
    auto Thread::halt() const noexcept -> void {
        driver::device.deepSleep(0);
        this->abort();
    }

    Thread thisThread;
}