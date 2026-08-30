#pragma once

#include "raylib.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <functional>
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h" // For CharacterSprites

// Design Patterns - Singleton, Flyweight/Resource Cache, and Command Queue:
// GetInstance supplies the sole resource owner. texturesByPath shares one GPU
// texture among key aliases (flyweights). LoadingTask packages labelled
// std::function commands that the loading screen executes incrementally.
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

    /// Creates a AssetManager instance from the supplied configuration.
    AssetManager() = default;
    /// Releases resources owned by this AssetManager instance.
    ~AssetManager() { UnloadAll(); }
    
    // Prevent copying
    /// Creates a AssetManager instance from the supplied configuration.
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

public:
    /// Returns the process-wide singleton instance of this manager.
    static AssetManager& GetInstance();

    // Core functions
    /// Loads texture2 d.
    Texture2D LoadTexture2D(const std::string& key, const std::string& path, bool applyPointFilter = false);
    /// Returns the current texture.
    Texture2D GetTexture(const std::string& key);
    
    /// Loads custom font.
    Font LoadCustomFont(const std::string& key, const std::string& path, int fontSize);
    /// Returns the current custom font.
    Font GetCustomFont(const std::string& key);
    
    /// Loads global fonts.
    void LoadGlobalFonts();
    /// Unloads all.
    void UnloadAll();
    /// Returns the current texture alias count.
    std::size_t GetTextureAliasCount() const { return textures.size(); }
    /// Returns the current unique texture count.
    std::size_t GetUniqueTextureCount() const { return texturesByPath.size(); }
    /// Returns the current font count.
    std::size_t GetFontCount() const { return fonts.size(); }
    /// Returns the current estimated texture bytes.
    std::size_t GetEstimatedTextureBytes() const;

    // Helper to load all character sprites
    /// Begins loading queue.
    void BeginLoadingQueue();
    /// Queues loading task.
    void QueueLoadingTask(
        const std::string& label,
        std::function<void()> action
    );
    /// Queues common assets.
    void QueueCommonAssets();
    /// Queues character assets.
    void QueueCharacterAssets();
    /// Updates loading.
    bool UpdateLoading(
        float& outProgress,
        std::string& outCurrentTask
    );
    
    // Quick getters for specific sprite sheets
    /// Returns the current lance sprites.
    CharacterSprites GetLanceSprites();
    /// Returns the current keith sprites.
    CharacterSprites GetKeithSprites();
    /// Returns the current hunk sprites.
    CharacterSprites GetHunkSprites();
    /// Returns the current pidge sprites.
    CharacterSprites GetPidgeSprites();
    
    /// Returns the current range sprites.
    EnemySprites GetRangeSprites();
    /// Returns the current diver sprites.
    EnemySprites GetDiverSprites();
    /// Returns the current chaser sprites.
    EnemySprites GetChaserSprites();
    /// Returns the current boss sprites.
    EnemySprites GetBossSprites();
};
