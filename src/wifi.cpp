#ifdef IOP_POSIX
#include "posix/wifi.hpp"
#elif defined(IOP_ESP8266)
#include "esp8266/wifi.hpp"
#elif defined(IOP_ESP32)
#include "esp32/wifi.hpp"
#elif defined(IOP_NOOP)
#include "noop/wifi.hpp"
#else
#error "Target not supported"
#endif