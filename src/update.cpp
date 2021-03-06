#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "cpp17/update.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/esp/update.hpp"
#elif defined(IOP_ESP32)
#include "arduino/esp/update.hpp"
#elif defined(IOP_NOOP)
#include "noop/update.hpp"
#else
#error "Target not supported"
#endif