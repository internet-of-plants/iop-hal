#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "posix/device.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/esp/esp8266/device.hpp"
#elif defined(IOP_ESP32)
#include "arduino/esp/esp32/device.hpp"
#elif defined(IOP_NOOP)
#include "noop/device.hpp"
#else
#error "Target not supported"
#endif

namespace iop_hal {
    Device device;
}