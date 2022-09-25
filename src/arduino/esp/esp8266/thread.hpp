#include "arduino/thread.hpp"
#include "iop-hal/log.hpp"

#include <umm_malloc/umm_malloc.h>
#include <umm_malloc/umm_malloc_cfg.h>
#include <umm_malloc/umm_heap_select.h>

namespace iop_hal {
auto Thread::abort() const noexcept -> void {
  IOP_TRACE();
  __panic_func(__FILE__, __LINE__, __PRETTY_FUNCTION__);
}

auto Thread::availableMemory() const noexcept -> Memory {
  static std::unordered_map<std::string_view, uintmax_t> heap;
  static std::unordered_map<std::string_view, uintmax_t> biggestBlock;

  heap.clear();
  biggestBlock.clear();

  umm_info(NULL, false);

  {
    HeapSelectDram _guard;
    const auto free = umm_free_heap_size_core(umm_get_current_heap());
    heap.insert({ "DRAM", free });
    biggestBlock.insert({ "DRAM", ESP.getMaxFreeBlockSize() });
  }
#ifdef UMM_HEAP_IRAM
  {
    HeapSelectIram _guard;
    const auto free = umm_free_heap_size_core(umm_get_current_heap());
    heap.insert({ "IRAM", free });
    biggestBlock.insert({ "DRAM", ESP.getMaxFreeBlockSize() });
  }
#endif

  ESP.resetFreeContStack();
  return Memory(ESP.getFreeContStack(), heap, biggestBlock);
}
}