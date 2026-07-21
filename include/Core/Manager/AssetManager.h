#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>
#include "Entities/Player/Paladin.h" // For CharacterSprites

class AssetManager {
private:
    std::unordered_map<std::string, Texture2D> textures;

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
    void LoadCharacterAssets();
    
    // Quick getters for specific sprite sheets
    CharacterSprites GetLanceSprites();
    CharacterSprites GetKeithSprites();
    CharacterSprites GetHunkSprites();
};
