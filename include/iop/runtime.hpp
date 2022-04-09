#ifndef IOP_DRIVER_RUNTIME_HPP
#define IOP_DRIVER_RUNTIME_HPP

#include "iop/string.hpp"

namespace driver {
    /// Should be defined by the application
    ///
    /// Initializes system, run only once
    auto setup() noexcept -> void;

    /// Should be defined by the application
    ///
    /// Event loop function, continuously scheduled for execution
    auto loop() noexcept -> void;
}

#endif