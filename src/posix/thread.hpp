#include "cpp17/thread.hpp"
#include "cpp17/runtime_metadata.hpp"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

namespace iop_hal {
auto Thread::availableMemory() const noexcept -> Memory {
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);

  std::unordered_map<std::string_view, uintmax_t> heap;
  heap.insert({ std::string_view("DRAM"), pages * page_size });

  std::unordered_map<std::string_view, uintmax_t> biggestBlock;
  biggestBlock.insert({ std::string_view("DRAM"), pages * page_size }); // Ballpark

  return Memory(iop_hal::stack_used(), heap, biggestBlock);
}
}