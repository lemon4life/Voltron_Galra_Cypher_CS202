#pragma once

#include "raylib.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <functional>
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h" // For CharacterSprites

class AssetManager {
private:
    struct LoadingTask {
        std::string label;
        std::function<void()> action;
    };

    std::unordered_map<std::string, Texture2D> textures;
    std::unordered_map<std::string, Texture2D> texturesByPath;
    std::unordered_map<std::string, Font> fonts;
    std::deque<LoadingTask> loadTasks;
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
    
    Font LoadCustomFont(const std::string& key, const std::string& path, int fontSize);
    Font GetCustomFont(const std::string& key);
    
    void LoadGlobalFonts();
    void UnloadAll();
    std::size_t GetTextureAliasCount() const { return textures.size(); }
    std::size_t GetUniqueTextureCount() const { return texturesByPath.size(); }
    std::size_t GetFontCount() const { return fonts.size(); }
    std::size_t GetEstimatedTextureBytes() const;

    // Helper to load all character sprites
    void BeginLoadingQueue();
    void QueueLoadingTask(
        const std::string& label,
        std::function<void()> action
    );
    void QueueCommonAssets();
    void QueueCharacterAssets();
    bool UpdateLoading(
        float& outProgress,
        std::string& outCurrentTask
    );
    
    // Quick getters for specific sprite sheets
    CharacterSprites GetLanceSprites();
    CharacterSprites GetKeithSprites();
    CharacterSprites GetHunkSprites();
    CharacterSprites GetPidgeSprites();
    
    EnemySprites GetRangeSprites();
    EnemySprites GetDiverSprites();
    EnemySprites GetChaserSprites();
    EnemySprites GetBossSprites();
};
