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

Font AssetManager::LoadCustomFont(const std::string& key, const std::string& path, int fontSize) {
    if (fonts.find(key) != fonts.end()) {
        return fonts[key];
    }
    
    Font font = LoadFontEx(path.c_str(), fontSize, 0, 250);
    if (font.texture.id == 0) {
        std::cerr << "Failed to load font: " << path << std::endl;
    }
    
    fonts[key] = font;
    return font;
}

Font AssetManager::GetCustomFont(const std::string& key) {
    auto it = fonts.find(key);
    if (it != fonts.end()) {
        return it->second;
    }
    return GetFontDefault();
}

void AssetManager::LoadGlobalFonts() {
    LoadCustomFont("PixeloidMono", "assets/fonts/PixeloidMono.ttf", 64);
    LoadCustomFont("PixeloidBold", "assets/fonts/PixeloidSans-Bold.ttf", 64);
    LoadCustomFont("PixeloidSans", "assets/fonts/PixeloidSans.ttf", 64);
}

void AssetManager::LoadCommonAssets() {
    LoadTexture2D("stats_checkpoint", "assets/UI/stats_checkpoint.png");
    LoadTexture2D("button_pause", "assets/UI/button_pause.png");
    LoadTexture2D("dialogue_panel", "assets/UI/dialogue.png");
    LoadTexture2D("select_arrow", "assets/UI/select_arrow.png");
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        UnloadTexture(pair.second);
    }
    textures.clear();
    
    for (auto& pair : fonts) {
        UnloadFont(pair.second);
    }
    fonts.clear();
}

void AssetManager::QueueCharacterAssets() {
    loadTasks.clear();
    auto add = [this](const std::string& k, const std::string& p, bool f = false) { 
        loadTasks.push_back([this, k, p, f](){ LoadTexture2D(k, p, f); }); 
    };

    auto addBilinear = [this](const std::string& k, const std::string& p) { 
        loadTasks.push_back([this, k, p](){ 
            Texture2D tex = LoadTexture2D(k, p, false); 
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        }); 
    };

    // Character Cards
    addBilinear("Card_Lance", "assets/img/CharacterCard/Lance.png");
    addBilinear("Card_Keith", "assets/img/CharacterCard/Keith.png");
    addBilinear("Card_Hunk", "assets/img/CharacterCard/Hunk.png");
    addBilinear("Card_Pidge", "assets/img/CharacterCard/Pidge.png");

    // Lance
    add("Lance_Idle", "assets/sprites/Lance/Idle_Sheet.png", true);
    add("Lance_Run", "assets/sprites/Lance/Run_Sheet.png", true);
    add("Lance_DashFront", "assets/sprites/Lance/Dash_front.png", true);
    add("Lance_DashBack", "assets/sprites/Lance/Dash_back.png", true);
    add("Lance_Weapon", "assets/sprites/Lance/Weapon_Static.png", true);
    add("Lance_Muzzle", "assets/sprites/Lance/Muzzle_Flash.png", true);
    add("Lance_Bullet", "assets/sprites/Lance/Bullet.png", true);
    add("Lance_Impact", "assets/sprites/Lance/Bullet_Impact.png", true);
    add("Lance_Parry", "assets/sprites/Lance/Parry.png", true);

    // Keith
    add("Keith_Idle", "assets/sprites/Keith/Idle_Sheet.png", true);
    add("Keith_Run", "assets/sprites/Keith/Run_Sheet.png", true);
    add("Keith_Weapon", "assets/sprites/Keith/Weapon_Static.png", true);
    add("Keith_Attack1", "assets/sprites/Keith/Attack_1.png", true);
    add("Keith_Attack2", "assets/sprites/Keith/Attack_2.png", true);
    add("Keith_DashFront", "assets/sprites/Keith/Dash_front.png", true);
    add("Keith_DashBack", "assets/sprites/Keith/Dash_back.png", true);
    add("Keith_Parry", "assets/sprites/Keith/Parry.png", true);

    // Hunk
    add("Hunk_Idle", "assets/sprites/Hunk/Idle_Sheet.png", true);
    add("Hunk_Run", "assets/sprites/Hunk/Run_Sheet.png", true);
    add("Hunk_Weapon", "assets/sprites/Hunk/Weapon_Static.png", true);
    add("Hunk_Muzzle", "assets/sprites/Hunk/Muzzle.png", true);
    add("Hunk_Bullet", "assets/sprites/Hunk/Beam.png", true);
    add("Hunk_Impact", "assets/sprites/Hunk/Beam_Impact.png", true);
    add("Hunk_DashFront", "assets/sprites/Hunk/Dash_front.png", true);
    add("Hunk_DashBack", "assets/sprites/Hunk/Dash_back.png", true);
    add("Hunk_Parry", "assets/sprites/Hunk/Parry.png", true);
    
    // Down Sprites
    add("Lance_Down", "assets/sprites/Lance/Down.png");
    add("Keith_Down", "assets/sprites/Keith/Down.png");
    add("Hunk_Down", "assets/sprites/Hunk/Down.png");
    add("Enemy_Down", "assets/sprites/Enemy/Knight_down.png", true);

    // Pidge
    add("Paladin_Pidge_Idle", "assets/sprites/Pidge/Idle_Sheet.png", true);
    add("Paladin_Pidge_Run", "assets/sprites/Pidge/Run_Sheet.png", true);
    add("Paladin_Pidge_Weapon", "assets/sprites/Pidge/Weapon_Static.png", true);
    add("Paladin_Pidge_DashFront", "assets/sprites/Pidge/Dash_front.png", true);
    add("Paladin_Pidge_DashBack", "assets/sprites/Pidge/Dash_back.png", true);
    add("Paladin_Pidge_Parry", "assets/sprites/Pidge/Parry.png", true);
    add("Paladin_Pidge_Down", "assets/sprites/Pidge/Down.png", true);
    add("Rover", "assets/sprites/Pidge/Rover.png", true);
    add("Rover_bullet", "assets/sprites/Pidge/Rover_bullet.png", true);
    add("HP_effect", "assets/sprites/Effects/HP_effect.png", true);
    add("Ex_effect", "assets/sprites/Effects/Ex_effect.png", true);
    add("Quint_effect", "assets/sprites/Effects/Quint_effect.png", true);
    add("Freeze", "assets/sprites/Effects/freeze.png", true);
    add("Freeze_Base", "assets/sprites/Effects/freeze_base.png", true);
    add("Freeze_Big", "assets/sprites/Effects/freeze_big.png", true);

    // Objects
    add("Quint_Orb", "assets/Objects/quint.png", true);

    // UI, Objects and Effects
    add("Player_Circle", "assets/UI/Player_Circle.png");
    add("box", "assets/Objects/box.png", true);
    add("object_2", "assets/Objects/object_2.png", true);
    add("pot_ex", "assets/Objects/pot_ex.png", true);
    add("pot_hp", "assets/Objects/pot_hp.png", true);
    add("pot_quint", "assets/Objects/pot_quint.png", true);
    add("tall_object_1_8", "assets/Objects/tall_object_1_8.png", true);
    add("Transfer_gate", "assets/Objects/Transfer_gate.png", true);
    add("Galra_Floors", "assets/tileset/Galra_Floors.png", true);
    add("Galra_Walls", "assets/tileset/Galra_Walls.png", true);
    
    add("Run_Dust", "assets/UI/Run_Dust.png");
    add("Aim", "assets/UI/aim.png");

    add("Knight_Idle", "assets/sprites/Enemy/Knight.png", true);
    add("Knight_Run", "assets/sprites/Enemy/Knight_run-Sheet.png", true);
    add("Knight_Down", "assets/sprites/Enemy/Knight_down.png", true);
    add("Knight_Gun", "assets/sprites/Enemy/Knight_gun.png", true);
    add("Knight_Gun_Bullet", "assets/sprites/Enemy/Knight_gun_bullet.png", true);
    add("Knight_Lance", "assets/sprites/Enemy/Knight_lance.png", true);
    add("Lance_Stab", "assets/sprites/Effects/Lance_stab_small.png", true);
    add("Knight_Sword", "assets/sprites/Enemy/Knight_sword.png", true);
    add("Sword_Slash_Small", "assets/sprites/Effects/Sword_slash_small.png", true);
    add("Sword_Slash_Big", "assets/sprites/Effects/Sword_slash_big.png", true);

    // Boss
    add("Boss_Idle", "assets/sprites/Enemy/Boss/Boss-1-idle.png", true);
    add("Boss_Run", "assets/sprites/Enemy/Boss/boss-run-1.png", true);
    add("Boss_Spell", "assets/sprites/Enemy/Boss/boss-spell.png", true);
    add("Boss_Punch_Ready", "assets/sprites/Enemy/Boss/Boss-punch-ready.png", true);
    add("Boss_Punch_Body", "assets/sprites/Enemy/Boss/Boss-punch-body.png", true);
    add("Boss_Punch_Hand", "assets/sprites/Enemy/Boss/Hand.png", true);
    add("Boss_Fire_Punch", "assets/sprites/Enemy/Boss/Fire-Punch.png", true);
    add("Enemy_Spawn", "assets/sprites/Effects/Spawn.png", true);

    // Drone
    add("Drone", "assets/sprites/Enemy/Drone.png", true);
    add("Drone_bullet", "assets/sprites/Enemy/Drone_bullet.png", true);
    add("Drone_down", "assets/sprites/Enemy/Drone_down.png", true);
}

bool AssetManager::UpdateLoading(float& outProgress) {
    if (totalTasks == 0 && !loadTasks.empty()) {
        totalTasks = loadTasks.size();
    }
    
    // Process up to 2 tasks per frame
    for(int i=0; i<2; i++) {
        if (!loadTasks.empty()) {
            loadTasks.front()();
            loadTasks.erase(loadTasks.begin());
        }
    }
    
    if (totalTasks > 0) {
        outProgress = 1.0f - ((float)loadTasks.size() / (float)totalTasks);
    } else {
        outProgress = 1.0f;
    }
    
    return loadTasks.empty();
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
    sprites.down = GetTexture("Lance_Down");
    return sprites;
}

CharacterSprites AssetManager::GetKeithSprites() {
    CharacterSprites sprites;
    sprites.idle = GetTexture("Keith_Idle");
    sprites.run = GetTexture("Keith_Run");
    sprites.weapon = GetTexture("Keith_Weapon");
    sprites.attack1 = GetTexture("Keith_Attack1");
    sprites.attack2 = GetTexture("Keith_Attack2");
    sprites.dashFront = GetTexture("Keith_DashFront");
    sprites.dashBack = GetTexture("Keith_DashBack");
    sprites.parry = GetTexture("Keith_Parry");
    sprites.down = GetTexture("Keith_Down");
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
    sprites.dashFront = GetTexture("Hunk_DashFront");
    sprites.dashBack = GetTexture("Hunk_DashBack");
    sprites.parry = GetTexture("Hunk_Parry");
    sprites.down = GetTexture("Hunk_Down");
    return sprites;
}

CharacterSprites AssetManager::GetPidgeSprites() {
    CharacterSprites sprites;
    sprites.idle = GetTexture("Paladin_Pidge_Idle");
    sprites.run = GetTexture("Paladin_Pidge_Run");
    sprites.weapon = GetTexture("Paladin_Pidge_Weapon");
    sprites.dashFront = GetTexture("Paladin_Pidge_DashFront");
    sprites.dashBack = GetTexture("Paladin_Pidge_DashBack");
    sprites.parry = GetTexture("Paladin_Pidge_Parry");
    sprites.down = GetTexture("Paladin_Pidge_Down");
    return sprites;
}

EnemySprites AssetManager::GetRangeSprites() {
    return EnemySprites{
        GetTexture("Knight_Idle"),
        GetTexture("Knight_Run"),
        GetTexture("Knight_Down"),
        GetTexture("Knight_Gun"),
        GetTexture("Knight_Gun_Bullet"), // effect not used here
        GetTexture("Knight_Gun_Bullet")
    };
}
EnemySprites AssetManager::GetDiverSprites() {
    return EnemySprites{
        GetTexture("Knight_Idle"),
        GetTexture("Knight_Run"),
        GetTexture("Knight_Down"),
        GetTexture("Knight_Lance"),
        GetTexture("Lance_Stab"),
        {0}
    };
}
EnemySprites AssetManager::GetChaserSprites() {
    return EnemySprites{
        GetTexture("Knight_Idle"),
        GetTexture("Knight_Run"),
        GetTexture("Knight_Down"),
        GetTexture("Knight_Sword"),
        GetTexture("Sword_Slash_Small"),
        {0}
    };
}

EnemySprites AssetManager::GetBossSprites() {
    return EnemySprites{
        GetTexture("Boss_Idle"),
        GetTexture("Boss_Run"),
        {0},
        {0},
        {0},
        {0}
    };
}
