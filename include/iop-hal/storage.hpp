#ifndef IOP_DRIVER_STORAGE_HPP
#define IOP_DRIVER_STORAGE_HPP

#include "iop-hal/panic.hpp"
#include <stdint.h>
#include <optional>

namespace iop_hal {

/// Low level abstraction to write data to storage, might be flash, HDDs, SSDs, etc.
/// Sadly it doesn't provide much type-safety, nor runtime checks, it's a mere wrapper over the internal storage
/// TODO: improve safety here
class Storage {
  uintmax_t size = 0;
  uint8_t *buffer = nullptr;

  /// Returns pointers to data, this is very unsafe, but a very useful primitive
  auto asRef() const noexcept -> uint8_t const *;
  auto asMut() noexcept -> uint8_t *;

public:
  /// Initializes storage, requiring at most `size`
  auto setup(uintmax_t size) noexcept -> bool;

  /// Reads byte from specified address, returns std::nullopt if address is out of bounds
  auto get(uintmax_t address) const noexcept -> std::optional<uint8_t>;

  /// Schedules writes to specified address, No-Op if address is out of bounds.
  /// It won't not actually commit the data, `Storage::commit` must be called to flush the data to storage.
  auto set(uintmax_t address, uint8_t val) noexcept -> bool;

  /// Commits data to storage, allows buffering data before storage, as physical writes can be expensive
  auto commit() noexcept -> bool;

  // TODO: this API still is fragile, we should only support std::arrays, but by type, not size

  /// Reads byte array from specified address, returns std::nullopt if address is out of bounds
  template<size_t SIZE>
  auto read(uintmax_t address) const noexcept -> std::optional<std::array<char, SIZE>> {
    IOP_TRACE();
    if (this->size < SIZE || address >= this->size - SIZE) return std::nullopt;
    std::array<char, SIZE> array;
    memcpy(array.data(), this->asRef() + address, SIZE);
    return array;
  }

  /// Writes arrays to storage. Commit must be called to ensure it goes through.
  template<size_t SIZE>
  auto write(const uintmax_t address, const std::array<char, SIZE> &array) -> bool {
    IOP_TRACE();
    if (this->size < SIZE || address >= this->size - SIZE) return false;
    memcpy(this->asMut() + address, array.data(), SIZE);
    return true;
  }
};
extern Storage storage;
}

#endif