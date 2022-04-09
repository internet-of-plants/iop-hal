#ifdef IOP_POSIX
#include "iop/posix/wifi.hpp"
#elif defined(IOP_ESP8266)
#include "iop/esp8266/wifi.hpp"
#elif defined(IOP_ESP32)
#include "iop/esp32/wifi.hpp"
#elif defined(IOP_NOOP)
#include "iop/noop/wifi.hpp"
#else
#error "Target not supported"
#endif