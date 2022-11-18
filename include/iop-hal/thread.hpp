#ifndef IOP_DRIVER_THREAD
#define IOP_DRIVER_THREAD

#include <stdint.h>
#include <unordered_map>
#include <string_view>

namespace iop {
namespace time {
using milliseconds = uintmax_t;
using seconds = uint32_t;
}
}

namespace iop_hal {
/// Describes the device's memory state in an instant.
struct Memory {
  uintmax_t availableStack;
  // It's a map because some environments have multiple, specialized, RAMs.
  // This allocation sucks tho. The keys should have static lifetime
  std::unordered_map<std::string_view, uintmax_t> availableHeap;
  std::unordered_map<std::string_view, uintmax_t> biggestHeapBlock;

  Memory(uintmax_t stack, std::unordered_map<std::string_view, uintmax_t> heap, std::unordered_map<std::string_view, uintmax_t> biggestBlock) noexcept:
    availableStack(stack), availableHeap(heap), biggestHeapBlock(biggestBlock) {}
};

class Thread {
public:
  /// Returns numbers of milliseconds since boot.
  auto timeRunning() const noexcept -> iop::time::milliseconds;

  /// Stops device for number of specified milliseconds
  void sleep(iop::time::milliseconds ms) const noexcept;

  /// Allows background task to proceed. Needs to be done every few hundred milliseconds
  /// Since we are based on cooperative concurrency the running task needs to allow others to run
  /// Generally used to allow WiFi to be operated, for example, but supports any kind of multithread system
  void yield() const noexcept;

  /// Stops firmware execution with failure. Might result in a device restart, or not.
  /// Don't expect to come back from it.
  void abort() const noexcept __attribute__((noreturn));

  /// Pauses execution and never comes back.
  void halt() const noexcept __attribute__((noreturn));

  /// Measures current memory usage
  ///
  /// This is very important for monitoring stack overflows and heap fragmentation, as the device runs for a long time
  auto availableMemory() const noexcept -> Memory;

};

extern Thread thisThread;
}

#endif