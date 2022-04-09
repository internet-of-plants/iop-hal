#ifdef IOP_POSIX
#include "iop/posix/device.hpp"
#elif defined(IOP_ESP8266)
#include "iop/esp8266/device.hpp"
#elif defined(IOP_ESP32)
#include "iop/esp32/device.hpp"
#elif defined(IOP_NOOP)
#include "iop/noop/device.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
    Device device;
}