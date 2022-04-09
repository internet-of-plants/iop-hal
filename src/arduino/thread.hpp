#include "iop/thread.hpp"

#include <Arduino.h>

namespace driver {
auto Thread::sleep(iop::time::milliseconds ms) const noexcept -> void {
  ::delay(ms);
}
auto Thread::yield() const noexcept -> void {
  ::yield();
}
auto Thread::timeRunning() const noexcept -> iop::time::milliseconds {
  return ::millis();
}
}