#include "arduino/thread.hpp"
//#include <freertos/task.h>

namespace iop_hal {
auto Thread::abort() const noexcept -> void {
  esp_system_abort("Called Thread::abort");
}

auto Thread::availableMemory() const noexcept -> Memory {
  static std::map<std::string_view, uintmax_t> heap;
  static std::map<std::string_view, uintmax_t> biggestBlock;

  // TODO: get IRAM
  if (heap.size() == 0) {
    {
      heap.insert({ std::string_view("DRAM"), ESP.getFreeHeap() });
      biggestBlock.insert({ std::string_view("DRAM"), ESP.getMaxAllocHeap() });
    }
  }

  // TODO: show stack usage: uxTaskGetStackHighWaterMark(taskHandler)
  return Memory(0, heap, biggestBlock);
}
}