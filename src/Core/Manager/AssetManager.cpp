#include "Core/Manager/AssetManager.h"
#include <iostream>

AssetManager& AssetManager::GetInstance() {
    static AssetManager instance;
    return instance;
}

Texture2D AssetManager::LoadTexture2D(const std::string& key, const std::string& path, bool applyPointFilter) {
    if (textures.find(key) != textures.end()) {
        return textures[key]; // Already loaded
    }

    Texture2D tex = LoadTexture(path.c_str());
    if (tex.id == 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
    } else if (applyPointFilter) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    }

    textures[key] = tex;
    return tex;
}

Texture2D AssetManager::GetTexture(const std::string& key) {
    auto it = textures.find(key);
    if (it != textures.end()) {
        return it->second;
    }
    // Return empty texture if not found (id = 0)
    return Texture2D{0};
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        UnloadTexture(pair.second);
    }
    textures.clear();
}

void AssetManager::LoadCharacterAssets() {
    // Lance
    LoadTexture2D("Lance_Idle", "assets/sprites/Lance/Idle_Sheet.png");
    LoadTexture2D("Lance_Run", "assets/sprites/Lance/Run_Sheet.png");
    LoadTexture2D("Lance_DashFront", "assets/sprites/Lance/Dash_front.png");
    LoadTexture2D("Lance_DashBack", "assets/sprites/Lance/Dash_back.png");
    LoadTexture2D("Lance_Weapon", "assets/sprites/Lance/Weapon_Static.png", true); // Point filter for rotation
    LoadTexture2D("Lance_Muzzle", "assets/sprites/Lance/Muzzle_Flash.png", true);
    LoadTexture2D("Lance_Bullet", "assets/sprites/Lance/Bullet.png", true);
    LoadTexture2D("Lance_Impact", "assets/sprites/Lance/Bullet_Impact.png", true);
    LoadTexture2D("Lance_Parry", "assets/sprites/Lance/Parry.png", true);

    // Keith
    LoadTexture2D("Keith_Idle", "assets/sprites/Keith/Idle_Sheet.png");
    LoadTexture2D("Keith_Run", "assets/sprites/Keith/Run_Sheet.png");
    LoadTexture2D("Keith_Weapon", "assets/sprites/Keith/Weapon_Static.png", true);
    LoadTexture2D("Keith_Attack1", "assets/sprites/Keith/Attack_1.png", true);
    LoadTexture2D("Keith_Attack2", "assets/sprites/Keith/Attack_2.png", true);

    // Hunk
    LoadTexture2D("Hunk_Idle", "assets/sprites/Hunk/Idle_Sheet.png");
    LoadTexture2D("Hunk_Run", "assets/sprites/Hunk/Run_Sheet.png");
    LoadTexture2D("Hunk_Weapon", "assets/sprites/Hunk/Weapon_Static.png", true);
    LoadTexture2D("Hunk_Muzzle", "assets/sprites/Hunk/Muzzle.png", true);
    LoadTexture2D("Hunk_Bullet", "assets/sprites/Hunk/Beam.png", true);
    LoadTexture2D("Hunk_Impact", "assets/sprites/Hunk/Beam_Impact.png", true);
}

CharacterSprites AssetManager::GetLanceSprites() {
    CharacterSprites sprites;
    sprites.idle = GetTexture("Lance_Idle");
    sprites.run = GetTexture("Lance_Run");
    sprites.weapon = GetTexture("Lance_Weapon");
    sprites.muzzleFlash = GetTexture("Lance_Muzzle");
    sprites.bullet = GetTexture("Lance_Bullet");
    sprites.impact = GetTexture("Lance_Impact");
    sprites.dashFront = GetTexture("Lance_DashFront");
    sprites.dashBack = GetTexture("Lance_DashBack");
    sprites.parry = GetTexture("Lance_Parry");
    return sprites;
}

CharacterSprites AssetManager::GetKeithSprites() {
    CharacterSprites sprites;
    sprites.idle = GetTexture("Keith_Idle");
    sprites.run = GetTexture("Keith_Run");
    sprites.weapon = GetTexture("Keith_Weapon");
    sprites.attack1 = GetTexture("Keith_Attack1");
    sprites.attack2 = GetTexture("Keith_Attack2");
    return sprites;
}

CharacterSprites AssetManager::GetHunkSprites() {
    CharacterSprites sprites;
    sprites.idle = GetTexture("Hunk_Idle");
    sprites.run = GetTexture("Hunk_Run");
    sprites.weapon = GetTexture("Hunk_Weapon");
    sprites.muzzleFlash = GetTexture("Hunk_Muzzle");
    sprites.bullet = GetTexture("Hunk_Bullet");
    sprites.impact = GetTexture("Hunk_Impact");
    return sprites;
}
