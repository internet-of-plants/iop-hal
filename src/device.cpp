#ifdef IOP_POSIX
#include "driver/posix/device.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/device.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/device.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/device.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
    Device device;
}