#pragma once

#include <cstddef>

struct ProcessMemorySnapshot {
    std::size_t workingSet = 0;
    std::size_t peakWorkingSet = 0;
    std::size_t privateBytes = 0;
    std::size_t commitBytes = 0;
};

/// Reads process memory snapshot.
ProcessMemorySnapshot ReadProcessMemorySnapshot();
