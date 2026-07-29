#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h" // For CharacterSprites

class AssetManager {
private:
    std::unordered_map<std::string, Texture2D> textures;
    std::vector<std::function<void()>> loadTasks;
    int totalTasks = 0;

    AssetManager() = default;
    ~AssetManager() { UnloadAll(); }
    
    // Prevent copying
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

public:
    static AssetManager& GetInstance();

    // Core functions
    Texture2D LoadTexture2D(const std::string& key, const std::string& path, bool applyPointFilter = false);
    Texture2D GetTexture(const std::string& key);
    void UnloadAll();

    // Helper to load all character sprites
    void QueueCharacterAssets();
    bool UpdateLoading(float& outProgress);
    
    // Quick getters for specific sprite sheets
    CharacterSprites GetLanceSprites();
    CharacterSprites GetKeithSprites();
    CharacterSprites GetHunkSprites();
    
    EnemySprites GetRangeSprites();
    EnemySprites GetDiverSprites();
    EnemySprites GetChaserSprites();
};
