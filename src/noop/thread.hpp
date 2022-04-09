#include "iop-hal/thread.hpp"

namespace driver {
auto Thread::sleep(iop::time::milliseconds ms) const noexcept -> void { (void) ms; }
auto Thread::yield() const noexcept -> void {}
auto Thread::abort() const noexcept -> void { while (true) {} }
auto Thread::timeRunning() const noexcept -> iop::time::milliseconds { static iop::time::milliseconds val = 0; return val++; }
auto Thread::availableMemory() const noexcept -> Memory {
    std::map<std::string_view, uintmax_t> heap;
    heap.insert({ std::string_view("DRAM"), 20000 });

    std::map<std::string_view, uintmax_t> biggestBlock;
    biggestBlock.insert({ std::string_view("DRAM"), 20000 });

    return Memory {
        .availableStack = 2000,
        .availableHeap = heap,
        .biggestHeapBlock = biggestBlock,
    };
}
}