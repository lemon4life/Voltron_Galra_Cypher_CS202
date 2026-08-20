#pragma once

#include "Core/Manager/WaveManager.h"

// EncounterManager is the session-facing name for the existing wave policy.
// The alias keeps state/UI call sites source-compatible during the migration.
using EncounterManager = WaveManager;
