#ifdef IOP_POSIX
#ifndef UNIT_TEST
#include "driver/cpp17/runtime.hpp"
#endif
#elif defined(IOP_ESP8266)
#include "driver/arduino/runtime.hpp"
#elif defined(IOP_ESP32)
#include "driver/arduino/runtime.hpp"
#elif defined(IOP_NOOP)
#ifdef ARDUINO
#include "driver/arduino/runtime.hpp"
#else
#ifndef UNIT_TEST
#include "driver/cpp17/runtime.hpp"
#endif
#endif
#else
#error "Target not supported"
#endif