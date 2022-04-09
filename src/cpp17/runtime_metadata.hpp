namespace driver {
/// Horrible hack to help accessing argv[0], as it contains the full path to the current binary
/// Not exclusive to POSIX
auto execution_path() noexcept -> std::string_view __attribute__((weak));

/// Horrible hack to help calculating used stack, it gets the stack pointer from the beggining of main
/// And compares with current stack pointer.
auto stack_used() noexcept -> uintmax_t __attribute__((weak));
}