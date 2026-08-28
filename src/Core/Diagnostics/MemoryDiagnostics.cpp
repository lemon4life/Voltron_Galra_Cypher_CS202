#include "Core/Diagnostics/MemoryDiagnostics.h"
#include "Core/Diagnostics/ProcessMemory.h"
#include "Core/Diagnostics/FramePerformanceStats.h"

#include "Core/Constants.h"
#include "Core/Level/RoomNode.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/DecalManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/World/MapObject.h"
#include "Entities/GameObject.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
constexpr float LOG_INTERVAL_SECONDS = 10.0f;
float periodicTimer = 0.0f;

double ToMegabytes(std::size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

const char* StateName(GameState state) {
    switch (state) {
        case GameState::MAIN_MENU: return "MAIN_MENU";
        case GameState::HUB: return "HUB";
        case GameState::GAMEPLAY: return "GAMEPLAY";
        case GameState::PAUSE: return "PAUSE";
        case GameState::SETTINGS: return "SETTINGS";
        case GameState::ROOM_EDITOR: return "ROOM_EDITOR";
        case GameState::GAME_OVER: return "GAME_OVER";
        case GameState::VICTORY: return "VICTORY";
    }
    return "UNKNOWN";
}

std::string LocalTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local = {};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}
}

void MemoryDiagnostics::ResetLog() {
    periodicTimer = 0.0f;
    if (!Constants::DEBUG_MEMORY_DIAGNOSTICS) return;
    std::ofstream output("log.txt", std::ios::trunc);
    output << "Voltron runtime memory diagnostics\n";
    output << "Started: " << LocalTimestamp() << "\n";
    output << "Memory values are process CPU values; inspect GPU/shared "
              "memory separately.\n\n";
}

void MemoryDiagnostics::Capture(
    const std::string& label,
    const GameManager& game
) {
    if (!Constants::DEBUG_MEMORY_DIAGNOSTICS) return;

    ProcessMemorySnapshot process = ReadProcessMemorySnapshot();
    const FramePerformanceSnapshot& frameStats =
        FramePerformanceStats::GetInstance().GetSnapshot();
    ObjectManagerMemoryStats objects =
        game.GetObjectManager().GetMemoryStats();
    PathFindingMemoryStats paths =
        game.GetPathFindingManager().GetMemoryStats();
    LevelMemoryStats level = game.GetLevelManager()->GetMemoryStats();
    const EffectManager& effects = game.GetEffectManager();
    const ParticleManager& particles = ParticleManager::GetInstance();
    const DecalManager& decals = DecalManager::GetInstance();
    const AssetManager& assets = AssetManager::GetInstance();
    const AudioManager& audio = AudioManager::GetInstance();
    const DialogueManager& dialogue = DialogueManager::GetInstance();

    std::ofstream output("log.txt", std::ios::app);
    if (!output) return;
    output << std::fixed << std::setprecision(2);
    output << '[' << LocalTimestamp() << "] " << label
           << " state=" << StateName(game.GetState()) << '\n';
    output << "  process_mb private=" << ToMegabytes(process.privateBytes)
           << " working_set=" << ToMegabytes(process.workingSet)
           << " peak_working_set=" << ToMegabytes(process.peakWorkingSet)
           << " commit=" << ToMegabytes(process.commitBytes) << '\n';
    output << "  performance fps_current=" << frameStats.currentFps
           << " fps_average=" << frameStats.averageFps
           << " fps_lowest=" << frameStats.lowestFps
           << " fps_highest=" << frameStats.highestFps
           << " fps_1_percent_low=" << frameStats.onePercentLowFps
           << " fps_0_1_percent_low=" << frameStats.pointOnePercentLowFps
           << " below_target_percent=" << frameStats.belowTargetPercent
           << " hitch_percent=" << frameStats.hitchPercent
           << " frame_ms_current=" << frameStats.currentFrameMilliseconds
           << " frame_ms_average=" << frameStats.averageFrameMilliseconds
           << " frame_ms_p95=" << frameStats.p95FrameMilliseconds
           << " frame_ms_p99=" << frameStats.p99FrameMilliseconds
           << " frame_ms_max=" << frameStats.maximumFrameMilliseconds
           << " frame_ms_deviation="
           << frameStats.frameTimeDeviationMilliseconds
           << " sample_window_seconds=" << frameStats.windowSeconds
           << " samples=" << frameStats.sampleCount << '\n';
    output << "  objects enemies=" << objects.enemies << '/'
           << objects.enemyCapacity
           << " projectiles=" << objects.projectiles << '/'
           << objects.projectileCapacity
           << " pickups=" << objects.pickups
           << " assists=" << objects.assists
           << " interactables=" << objects.interactables
           << " orbs=" << objects.orbs << '/' << objects.orbCapacity
           << " pending_add=" << objects.pendingAdditions
           << " pending_remove=" << objects.pendingRemovals
           << " live_game_objects=" << GameObject::GetLiveCount()
           << " live_map_objects=" << MapObject::GetLiveCount() << '\n';
    output << "  effects timed=" << effects.GetActiveEffectCount() << '/'
           << effects.GetActiveEffectCapacity()
           << " particles=" << particles.GetActiveCount() << '/'
           << particles.GetCapacity()
           << " corpses=" << decals.GetCount() << '/'
           << decals.GetCapacity() << '\n';
    output << "  paths enemies=" << paths.enemies << '/'
           << paths.enemyCapacity
           << " records=" << paths.pathRecords
           << " grids=" << paths.navigationGrids
           << " grid_cells=" << paths.navigationGridCells
           << " flow_fields=" << paths.flowFields
           << " flow_cells=" << paths.flowFieldCells
           << " shared_goals=" << paths.sharedGoals << '/'
           << paths.sharedGoalCapacity << '\n';
    output << "  level nodes=" << level.roomNodes
           << " live_nodes=" << level.liveRoomNodes
           << " map_objects=" << level.mapObjects
           << " layer_cells=" << level.layerCells
           << " spawn_nodes=" << level.staticSpawnNodes
           << " los_tiles=" << level.lineOfSightBlockerTiles
           << " los_traces=" << level.lineOfSightTraces
           << " los_rectangles=" << level.lineOfSightRectangles << '\n';
    output << "  assets aliases=" << assets.GetTextureAliasCount()
           << " unique_textures=" << assets.GetUniqueTextureCount()
           << " decoded_texture_mb="
           << ToMegabytes(assets.GetEstimatedTextureBytes())
           << " fonts=" << assets.GetFontCount()
           << " sounds=" << audio.GetSoundCount()
           << " music=" << audio.GetMusicCount() << '\n';
    output << "  dialogue source_nodes="
           << dialogue.GetDialogueNodeCount()
           << " transient_response="
           << (dialogue.HasTransientResponse() ? 1 : 0) << "\n\n";
}

void MemoryDiagnostics::UpdatePeriodic(
    float deltaTime,
    const GameManager& game
) {
    if (!Constants::DEBUG_MEMORY_DIAGNOSTICS) return;
    periodicTimer += deltaTime;
    if (periodicTimer < LOG_INTERVAL_SECONDS) return;
    periodicTimer = 0.0f;
    Capture("periodic", game);
}
