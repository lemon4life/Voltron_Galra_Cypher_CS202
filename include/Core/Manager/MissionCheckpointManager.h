#pragma once

#include "Core/MissionSaveData.h"

#include <cstdint>
#include <optional>

class GameManager;

// Maintains one stable mission checkpoint. Expensive map capture and disk I/O
// occur only on floor/room progress events, never during battle updates.
class MissionCheckpointManager {
public:
    static MissionCheckpointManager& GetInstance();

    /// Validates the on-disk version/checksum without changing the active world.
    bool HasValidSave() const;
    /// Loads the saved checkpoint into the supplied game manager.
    bool Load(GameManager& gameManager);
    /// Captures and writes a stable floor/room checkpoint when progress changed.
    void SaveIfProgressed(GameManager& gameManager);
    /// Updates only the small in-memory pre-battle team/utility portion.
    void CapturePreBattle(GameManager& gameManager);
    /// Writes the best cached stable checkpoint during normal shutdown.
    void FlushOnShutdown(GameManager& gameManager);
    /// Removes persistent and in-memory mission progress.
    void DeleteSave();

private:
    MissionCheckpointManager() = default;

    bool Write(const MissionSaveData& saved) const;
    bool Read(MissionSaveData& saved) const;

    std::optional<MissionSaveData> cachedCheckpoint;
    std::uint64_t capturedProgressRevision = 0;
};
