#include "Core/Manager/MissionCheckpointManager.h"

#include "Core/Manager/GameManager.h"
#include "Core/Manager/ObjectManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/LevelManager.h"

#include "raylib.h"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
constexpr std::array<char, 8> SAVE_MAGIC = {
    'V', 'G', 'C', 'P', 'S', 'A', 'V', 'E'
};
constexpr std::uint64_t MAX_PAYLOAD_BYTES = 16ULL * 1024ULL * 1024ULL;
constexpr std::uint32_t MAX_COLLECTION_SIZE = 1'000'000U;

class BinaryWriter {
public:
    template <typename T>
    void Value(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(T));
    }

    void Bool(bool value) { Value<std::uint8_t>(value ? 1U : 0U); }
    void Vector2Value(Vector2 value) { Value(value.x); Value(value.y); }
    void RectangleValue(Rectangle value) {
        Value(value.x); Value(value.y); Value(value.width); Value(value.height);
    }
    void IntVector(const std::vector<int>& values) {
        Value(static_cast<std::uint32_t>(values.size()));
        for (int value : values) Value(value);
    }

    std::vector<std::uint8_t> data;
};

class BinaryReader {
public:
    explicit BinaryReader(const std::vector<std::uint8_t>& source)
        : data(source) {}

    template <typename T>
    T Value() {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset > data.size() || sizeof(T) > data.size() - offset) {
            throw std::runtime_error("Mission save ended unexpectedly");
        }
        T value;
        std::memcpy(&value, data.data() + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    bool Bool() {
        std::uint8_t value = Value<std::uint8_t>();
        if (value > 1U) throw std::runtime_error("Invalid saved boolean");
        return value != 0U;
    }
    Vector2 Vector2Value() { return { Value<float>(), Value<float>() }; }
    Rectangle RectangleValue() {
        return { Value<float>(), Value<float>(), Value<float>(), Value<float>() };
    }
    std::uint32_t Count(std::uint32_t maximum = MAX_COLLECTION_SIZE) {
        std::uint32_t count = Value<std::uint32_t>();
        if (count > maximum) throw std::runtime_error("Save collection too large");
        return count;
    }
    std::vector<int> IntVector() {
        std::uint32_t count = Count();
        std::vector<int> values;
        values.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index) {
            values.push_back(Value<int>());
        }
        return values;
    }
    bool Finished() const { return offset == data.size(); }

private:
    const std::vector<std::uint8_t>& data;
    std::size_t offset = 0;
};

std::filesystem::path SavePath() {
    return std::filesystem::path(GetApplicationDirectory()) /
        "saves" / "mission_checkpoint.bin";
}

std::uint32_t Checksum(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t hash = 2166136261U;
    for (std::uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 16777619U;
    }
    return hash;
}

void WritePaladin(BinaryWriter& writer, const SavedPaladinState& saved) {
    writer.Value(saved.id);
    writer.Vector2Value(saved.position);
    writer.Vector2Value(saved.aimTarget);
    writer.Value(saved.health);
    writer.Value(saved.maxHealth);
    writer.Value(saved.ghostHealth);
    writer.Value(saved.displayedHealth);
    writer.Value(saved.exEnergy);
    writer.Value(saved.displayedExEnergy);
    writer.Value(saved.attackCooldown);
    writer.Value(saved.dashCooldown);
    writer.Value(saved.ultimateCooldown);
    writer.Value(saved.activeSkillDuration);
    writer.Value(saved.activeSkillTimer);
    writer.Value(saved.paladinLevel);
    writer.Bool(saved.facingLeft);
    writer.Bool(saved.skillActive);
}

SavedPaladinState ReadPaladin(BinaryReader& reader) {
    SavedPaladinState saved;
    saved.id = reader.Value<int>();
    saved.position = reader.Vector2Value();
    saved.aimTarget = reader.Vector2Value();
    saved.health = reader.Value<int>();
    saved.maxHealth = reader.Value<int>();
    saved.ghostHealth = reader.Value<float>();
    saved.displayedHealth = reader.Value<float>();
    saved.exEnergy = reader.Value<float>();
    saved.displayedExEnergy = reader.Value<float>();
    saved.attackCooldown = reader.Value<float>();
    saved.dashCooldown = reader.Value<float>();
    saved.ultimateCooldown = reader.Value<float>();
    saved.activeSkillDuration = reader.Value<float>();
    saved.activeSkillTimer = reader.Value<float>();
    saved.paladinLevel = reader.Value<int>();
    saved.facingLeft = reader.Bool();
    saved.skillActive = reader.Bool();
    return saved;
}

std::vector<std::uint8_t> Serialize(const MissionSaveData& saved) {
    BinaryWriter writer;
    writer.Value(saved.floor);
    writer.Bool(saved.talkedToShiro);
    writer.Bool(saved.autoAim);

    const SavedLevelState& level = saved.level;
    writer.Value(level.width);
    writer.Value(level.height);
    writer.Value(level.graphWidth);
    writer.Value(level.graphHeight);
    writer.Value(level.spawnRoomX);
    writer.Value(level.spawnRoomY);
    writer.Vector2Value(level.roomOffset);
    writer.IntVector(level.floorTiles);
    writer.IntVector(level.objectTiles);
    writer.IntVector(level.propTiles);
    writer.Value(static_cast<std::uint32_t>(level.rooms.size()));
    for (const SavedRoomState& room : level.rooms) {
        writer.Value(room.gridX);
        writer.Value(room.gridY);
        writer.Value(room.type);
        writer.Value(room.roomSize);
        writer.Bool(room.discovered);
        writer.Bool(room.cleared);
        writer.Value(room.state);
        writer.RectangleValue(room.triggerBounds);
    }
    writer.Value(static_cast<std::uint32_t>(level.mapObjects.size()));
    for (const SavedMapObjectState& object : level.mapObjects) {
        writer.Value(object.type);
        writer.Vector2Value(object.position);
        writer.Value(object.row);
        writer.Value(object.column);
        writer.Bool(object.door);
        writer.Value(object.doorState);
        writer.Bool(object.projectileBarrier);
    }

    const SavedTeamState& team = saved.team;
    writer.Value(static_cast<std::uint32_t>(team.roster.size()));
    for (const SavedPaladinState& paladin : team.roster) {
        WritePaladin(writer, paladin);
    }
    writer.IntVector(team.selectedSlots);
    writer.Value(team.activeIndex);
    writer.Value(team.sharedArmor);
    writer.Value(team.maxSharedArmor);
    writer.Value(team.quintessence);
    writer.Value(team.coins);

    writer.Value(static_cast<std::uint32_t>(saved.utilityObjects.size()));
    for (const SavedDynamicObject& object : saved.utilityObjects) {
        writer.Value(object.type);
        writer.Vector2Value(object.position);
    }
    return std::move(writer.data);
}

MissionSaveData Deserialize(const std::vector<std::uint8_t>& payload) {
    BinaryReader reader(payload);
    MissionSaveData saved;
    saved.floor = reader.Value<int>();
    saved.talkedToShiro = reader.Bool();
    saved.autoAim = reader.Bool();

    SavedLevelState& level = saved.level;
    level.width = reader.Value<int>();
    level.height = reader.Value<int>();
    level.graphWidth = reader.Value<int>();
    level.graphHeight = reader.Value<int>();
    level.spawnRoomX = reader.Value<int>();
    level.spawnRoomY = reader.Value<int>();
    level.roomOffset = reader.Vector2Value();
    level.floorTiles = reader.IntVector();
    level.objectTiles = reader.IntVector();
    level.propTiles = reader.IntVector();
    std::uint32_t roomCount = reader.Count(128U);
    level.rooms.reserve(roomCount);
    for (std::uint32_t index = 0; index < roomCount; ++index) {
        SavedRoomState room;
        room.gridX = reader.Value<int>();
        room.gridY = reader.Value<int>();
        room.type = reader.Value<int>();
        room.roomSize = reader.Value<int>();
        room.discovered = reader.Bool();
        room.cleared = reader.Bool();
        room.state = reader.Value<int>();
        room.triggerBounds = reader.RectangleValue();
        level.rooms.push_back(room);
    }
    std::uint32_t mapObjectCount = reader.Count(100'000U);
    level.mapObjects.reserve(mapObjectCount);
    for (std::uint32_t index = 0; index < mapObjectCount; ++index) {
        SavedMapObjectState object;
        object.type = reader.Value<int>();
        object.position = reader.Vector2Value();
        object.row = reader.Value<int>();
        object.column = reader.Value<int>();
        object.door = reader.Bool();
        object.doorState = reader.Value<int>();
        object.projectileBarrier = reader.Bool();
        level.mapObjects.push_back(object);
    }

    SavedTeamState& team = saved.team;
    std::uint32_t rosterCount = reader.Count(8U);
    team.roster.reserve(rosterCount);
    for (std::uint32_t index = 0; index < rosterCount; ++index) {
        team.roster.push_back(ReadPaladin(reader));
    }
    team.selectedSlots = reader.IntVector();
    if (team.selectedSlots.size() > 4U) {
        throw std::runtime_error("Invalid saved team size");
    }
    team.activeIndex = reader.Value<int>();
    team.sharedArmor = reader.Value<int>();
    team.maxSharedArmor = reader.Value<int>();
    team.quintessence = reader.Value<float>();
    team.coins = reader.Value<int>();

    std::uint32_t utilityCount = reader.Count(128U);
    saved.utilityObjects.reserve(utilityCount);
    for (std::uint32_t index = 0; index < utilityCount; ++index) {
        saved.utilityObjects.push_back({
            reader.Value<int>(),
            reader.Vector2Value()
        });
    }
    if (!reader.Finished()) throw std::runtime_error("Unexpected save data");
    return saved;
}
}

MissionCheckpointManager& MissionCheckpointManager::GetInstance() {
    static MissionCheckpointManager instance;
    return instance;
}

bool MissionCheckpointManager::Write(const MissionSaveData& saved) const {
    try {
        std::vector<std::uint8_t> payload = Serialize(saved);
        if (payload.empty() || payload.size() > MAX_PAYLOAD_BYTES) return false;
        std::filesystem::path path = SavePath();
        std::filesystem::create_directories(path.parent_path());
        std::filesystem::path temporary = path;
        temporary += ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        const std::uint32_t version = MissionSaveData::VERSION;
        const std::uint64_t payloadSize = payload.size();
        const std::uint32_t checksum = Checksum(payload);
        output.write(SAVE_MAGIC.data(), SAVE_MAGIC.size());
        output.write(reinterpret_cast<const char*>(&version), sizeof(version));
        output.write(
            reinterpret_cast<const char*>(&payloadSize),
            sizeof(payloadSize)
        );
        output.write(
            reinterpret_cast<const char*>(&checksum),
            sizeof(checksum)
        );
        output.write(
            reinterpret_cast<const char*>(payload.data()),
            static_cast<std::streamsize>(payload.size())
        );
        output.flush();
        if (!output) return false;
        output.close();
        std::error_code error;
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporary, path, error);
        return !error;
    } catch (...) {
        return false;
    }
}

bool MissionCheckpointManager::Read(MissionSaveData& saved) const {
    try {
        std::ifstream input(SavePath(), std::ios::binary);
        if (!input) return false;
        std::array<char, SAVE_MAGIC.size()> magic = {};
        std::uint32_t version = 0;
        std::uint64_t payloadSize = 0;
        std::uint32_t checksum = 0;
        input.read(magic.data(), magic.size());
        input.read(reinterpret_cast<char*>(&version), sizeof(version));
        input.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
        input.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
        if (!input || magic != SAVE_MAGIC ||
            version != MissionSaveData::VERSION || payloadSize == 0 ||
            payloadSize > MAX_PAYLOAD_BYTES) {
            return false;
        }
        std::vector<std::uint8_t> payload(
            static_cast<std::size_t>(payloadSize)
        );
        input.read(
            reinterpret_cast<char*>(payload.data()),
            static_cast<std::streamsize>(payload.size())
        );
        if (!input || Checksum(payload) != checksum) return false;
        saved = Deserialize(payload);
        return true;
    } catch (...) {
        return false;
    }
}

bool MissionCheckpointManager::HasValidSave() const {
    MissionSaveData ignored;
    return Read(ignored);
}

bool MissionCheckpointManager::Load(GameManager& gameManager) {
    MissionSaveData saved;
    if (!Read(saved) || !gameManager.RestoreCheckpointState(saved)) {
        return false;
    }
    cachedCheckpoint = saved;
    capturedProgressRevision = gameManager.GetLevelManager()
        ->GetCheckpointRevision();
    return true;
}

void MissionCheckpointManager::SaveIfProgressed(GameManager& gameManager) {
    if (!gameManager.HasCheckpointableMission()) return;
    std::uint64_t revision = gameManager.GetLevelManager()
        ->GetCheckpointRevision();
    if (cachedCheckpoint && revision == capturedProgressRevision) return;
    MissionSaveData candidate = gameManager.CaptureCheckpointState();
    if (candidate.level.width <= 0 || !Write(candidate)) return;
    cachedCheckpoint = std::move(candidate);
    capturedProgressRevision = revision;
}

void MissionCheckpointManager::CapturePreBattle(GameManager& gameManager) {
    if (!gameManager.HasCheckpointableMission()) return;
    if (!cachedCheckpoint) {
        cachedCheckpoint = gameManager.CaptureCheckpointState();
        capturedProgressRevision = gameManager.GetLevelManager()
            ->GetCheckpointRevision();
    } else {
        cachedCheckpoint->team = gameManager.GetTeamManager()
            ->CaptureCheckpointState();
        cachedCheckpoint->utilityObjects = gameManager.GetObjectManager()
            .CaptureCheckpointObjects();
        cachedCheckpoint->floor = gameManager.GetCurrentFloor();
    }
}

void MissionCheckpointManager::FlushOnShutdown(GameManager& gameManager) {
    if (!gameManager.HasCheckpointableMission()) return;
    if (!gameManager.GetLevelManager()->GetCurrentlyLockedRoom()) {
        cachedCheckpoint = gameManager.CaptureCheckpointState();
    } else {
        CapturePreBattle(gameManager);
    }
    if (cachedCheckpoint) Write(*cachedCheckpoint);
}

void MissionCheckpointManager::DeleteSave() {
    cachedCheckpoint.reset();
    capturedProgressRevision = 0;
    std::error_code ignored;
    std::filesystem::remove(SavePath(), ignored);
    std::filesystem::path temporary = SavePath();
    temporary += ".tmp";
    std::filesystem::remove(temporary, ignored);
}
