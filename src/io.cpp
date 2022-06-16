#ifdef IOP_POSIX_MOCK
#include "noop/io.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/io.hpp"
#elif defined(IOP_ESP32)
#include "arduino/io.hpp"
#elif defined(IOP_NOOP)
#include "noop/io.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/io.hpp"

namespace iop_hal {
iop_hal::io::GPIO gpio;
}
