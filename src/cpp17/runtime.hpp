#include "iop-hal/runtime.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/panic.hpp"

#include <sys/resource.h>

static char * filename;
static uintptr_t stackstart = 0;

namespace driver {
auto execution_path() noexcept -> std::string_view {
  iop_assert(filename != nullptr, IOP_STR("Filename wasn't initialized, maybe you are trying to get the execution path before main runs?"));
  return filename;
}
#ifdef IOP_POSIX
auto stack_used() noexcept -> uintmax_t {
  // Not the most precise way of getting stack usage
  uintptr_t stackend;
  stackend = (uintptr_t) (void*) &stackend;

  struct rlimit limit;
  getrlimit(RLIMIT_STACK, &limit);

  iop_assert(stackstart != 0, IOP_STR("The stack start has never been collected, maybe are you trying to get the stack size before main runs?"));
  return limit.rlim_cur - (stackstart - stackend);
}
#endif
}

int main(int argc, char** argv) {
  stackstart = (uintptr_t) (void*) &argc;
  iop_assert(argc > 0, IOP_STR("argc is 0"));
  filename = argv[0];

  driver::setup();
  while (true) {
    driver::loop();
    driver::thisThread.sleep(50);
  }
  return 0;
}