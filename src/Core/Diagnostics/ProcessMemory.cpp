#include "Core/Diagnostics/ProcessMemory.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

/// Reads process memory snapshot.
ProcessMemorySnapshot ReadProcessMemorySnapshot() {
    ProcessMemorySnapshot snapshot;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters = {};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        snapshot.workingSet = counters.WorkingSetSize;
        snapshot.peakWorkingSet = counters.PeakWorkingSetSize;
        snapshot.privateBytes = counters.PrivateUsage;
        snapshot.commitBytes = counters.PagefileUsage;
    }
#endif
    return snapshot;
}
