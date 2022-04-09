#include "iop-hal/storage.hpp"
#include "iop-hal/panic.hpp"

namespace iop_hal {
auto Storage::setup(const uintmax_t size) noexcept -> bool {
    iop_assert(size > 0, IOP_STR("Storage size is zero"));
    
    if (!this->buffer) {
        this->buffer = new (std::nothrow) uint8_t[size];
        if (!this->buffer) return false;
        this->size = size;
        std::memset(buffer, 0, size);
    }
    return true;
}
auto Storage::commit() noexcept -> bool { return true; }
auto Storage::asRef() const noexcept -> uint8_t const * { if (!buffer) iop_panic(IOP_STR("Buffer is nullptr")); return buffer; }
auto Storage::asMut() noexcept -> uint8_t * { if (!buffer) iop_panic(IOP_STR("Buffer is nullptr")); return buffer; }
}