#ifdef IOP_POSIX
#include "driver/noop/io.hpp"
#elif defined(IOP_ESP8266)
#include "driver/arduino/io.hpp"
#elif defined(IOP_ESP32)
#include "driver/arduino/io.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/io.hpp"
#else
#error "Target not supported"
#endif

#include "driver/io.hpp"

namespace driver {
driver::io::GPIO gpio;
}
