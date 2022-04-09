#ifdef IOP_POSIX
#include "iop/noop/io.hpp"
#elif defined(IOP_ESP8266)
#include "iop/arduino/io.hpp"
#elif defined(IOP_ESP32)
#include "iop/arduino/io.hpp"
#elif defined(IOP_NOOP)
#include "iop/noop/io.hpp"
#else
#error "Target not supported"
#endif

#include "iop/io.hpp"

namespace driver {
driver::io::GPIO gpio;
}
