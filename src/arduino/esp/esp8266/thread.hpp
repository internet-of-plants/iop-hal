#include "arduino/thread.hpp"
#include "iop-hal/log.hpp"

#include <umm_malloc/umm_heap_select.h>

namespace iop_hal {
auto Thread::abort() const noexcept -> void {
  IOP_TRACE();
  __panic_func(__FILE__, __LINE__, __PRETTY_FUNCTION__);
}

auto Thread::availableMemory() const noexcept -> Memory {
  static std::map<std::string_view, uintmax_t> heap;
  static std::map<std::string_view, uintmax_t> biggestBlock;

  if (heap.size() == 0) {
    {
      HeapSelectDram _guard;
      heap.insert({ std::string_view("DRAM"), ESP.getFreeHeap() });
      biggestBlock.insert({ std::string_view("DRAM"), ESP.getMaxFreeBlockSize() });
    }
    {
      HeapSelectIram _guard;
      heap.insert({ std::string_view("IRAM"), ESP.getFreeHeap() });
    }
  }

  ESP.resetFreeContStack();
  return Memory(ESP.getFreeContStack(), heap, biggestBlock);
}
}