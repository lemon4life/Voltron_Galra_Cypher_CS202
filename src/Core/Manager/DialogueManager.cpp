#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Player/Player.h"
#include <fstream>
#include <iostream>

DialogueManager::DialogueManager() : isDialogueActive(false), currentNode(0), selectedOption(0), missionRequested(false), typewriterTimer(0.0f), visibleCharCount(0) {}

DialogueManager::~DialogueManager() {
    UnloadFont(dialogFont);
    for (auto& pair : portraits) {
        UnloadTexture(pair.second);
    }
}

DialogueManager& DialogueManager::GetInstance() {
    static DialogueManager instance;
    return instance;
}

void DialogueManager::InitializeAssets() {
    dialogFont = LoadFontEx("assets/fonts/monogram.ttf", 32, 0, 250);
    portraits["Lance"] = LoadTexture("assets/img/Lance.PNG");
    portraits["Keith"] = LoadTexture("assets/img/Keith.PNG");
    portraits["Shiro"] = LoadTexture("assets/img/Shiro.PNG");
    portraits["Allura"] = LoadTexture("assets/img/Allura.PNG");
    portraits["Pidge"] = LoadTexture("assets/img/Pidge.PNG");
    
    for (auto& pair : portraits) {
        SetTextureFilter(pair.second, TEXTURE_FILTER_BILINEAR); // High HD filtering
    }
}

void DialogueManager::LoadDialogueTree(const std::string& filepath) {
    currentTree.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open dialogue file: " << filepath << std::endl;
        return;
    }

    std::string line;
    DialogueNode tempNode;
    bool hasNode = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '\r') continue;

        if (line.rfind("NODE:", 0) == 0) {
            if (hasNode) {
                currentTree.push_back(tempNode);
            }
            tempNode = DialogueNode();
            hasNode = true;
        } else if (line.rfind("SPEAKER:", 0) == 0) {
            tempNode.speakerName = line.substr(8);
            // Trim leading/trailing whitespace
            while (!tempNode.speakerName.empty() && tempNode.speakerName[0] == ' ') tempNode.speakerName = tempNode.speakerName.substr(1);
            while (!tempNode.speakerName.empty() && (tempNode.speakerName.back() == ' ' || tempNode.speakerName.back() == '\r')) tempNode.speakerName.pop_back();
        } else if (line.rfind("TEXT:", 0) == 0) {
            tempNode.text = line.substr(5);
            while (!tempNode.text.empty() && tempNode.text[0] == ' ') tempNode.text = tempNode.text.substr(1);
            while (!tempNode.text.empty() && (tempNode.text.back() == ' ' || tempNode.text.back() == '\r')) tempNode.text.pop_back();
        } else if (line.rfind("OPTION:", 0) == 0) {
            std::string optData = line.substr(7);
            while (!optData.empty() && optData[0] == ' ') optData = optData.substr(1);
            
            size_t delim = optData.find('|');
            if (delim != std::string::npos) {
                std::string optText = optData.substr(0, delim);
                while (!optText.empty() && optText.back() == ' ') optText.pop_back();
                
                std::string optIdxStr = optData.substr(delim + 1);
                while (!optIdxStr.empty() && optIdxStr[0] == ' ') optIdxStr = optIdxStr.substr(1);
                while (!optIdxStr.empty() && (optIdxStr.back() == ' ' || optIdxStr.back() == '\r')) optIdxStr.pop_back();
                
                tempNode.options.push_back(optText);
                tempNode.nextNodeIndices.push_back(std::stoi(optIdxStr));
            }
        }
    }
    if (hasNode) {
        currentTree.push_back(tempNode);
    }
    file.close();
}

void DialogueManager::StartDialogue() {
    currentNode = 0;
    selectedOption = 0;
    isDialogueActive = true;
    missionRequested = false;
    visibleCharCount = 0;
    typewriterTimer = 0.0f;
}

void DialogueManager::Update(float deltaTime) {
    if (!isDialogueActive || currentTree.empty()) return;

    const DialogueNode& node = currentTree[currentNode];
    bool isTyping = (visibleCharCount < (int)node.text.length());

    if (isTyping) {
        typewriterTimer += deltaTime;
        if (typewriterTimer > 0.03f) { // Typist speed
            typewriterTimer = 0.0f;
            visibleCharCount++;
            char c = node.text[visibleCharCount - 1];
            if (c != ' ') {
                float pitch = GetRandomValue(80, 120) / 100.0f;
                AudioManager::GetInstance().PlaySoundEffectPitch("blip", pitch);
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            // skip typing
            visibleCharCount = node.text.length();
        }
    } else {
        // typing done, allow option selection
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
            selectedOption--;
            if (selectedOption < 0) selectedOption = node.options.size() - 1;
        }
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
            selectedOption++;
            if (selectedOption >= (int)node.options.size()) selectedOption = 0;
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (node.options.empty()) {
                // End dialogue if no options
                int next = -1;
                if (!node.nextNodeIndices.empty()) next = node.nextNodeIndices[0];
                
                if (next == -1) {
                    isDialogueActive = false;
                    missionRequested = true;
                } else {
                    currentNode = next;
                    selectedOption = 0;
                    visibleCharCount = 0;
                    typewriterTimer = 0.0f;
                }
            } else {
                int next = node.nextNodeIndices[selectedOption];
                
                // Determine player name dynamically
                std::string playerName = "Lance";
                for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
                    if (Player* p = dynamic_cast<Player*>(entity)) {
                        if (!p->IsPlayingAsLance()) {
                            playerName = "Keith";
                        }
                        break;
                    }
                }
                
                // Inject the player's response as a new temporary node
                DialogueNode tempNode;
                tempNode.speakerName = playerName;
                tempNode.text = node.options[selectedOption];
                tempNode.nextNodeIndices.push_back(next);
                
                int tempIdx = currentTree.size();
                currentTree.push_back(tempNode);
                
                currentNode = tempIdx;
                selectedOption = 0;
                visibleCharCount = 0;
                typewriterTimer = 0.0f;
            }
        }
    }
}

void DialogueManager::Draw() {
    if (!isDialogueActive || currentTree.empty()) return;

    const DialogueNode& node = currentTree[currentNode];

    // Screen dimensions (Main Window resolution is 1366x1024)
    float screenWidth = 1366.0f;
    float screenHeight = 1024.0f;

    // Render portrait
    if (portraits.find(node.speakerName) != portraits.end()) {
        Texture2D port = portraits[node.speakerName];
        
        // Scale by height. 1024px down to fit screen.
        float targetHeight = 800.0f;
        float scale = targetHeight / (float)port.height;
        float scaledWidth = (float)port.width * scale;
        
        bool isLeft = (node.speakerName == "Lance" || node.speakerName == "Keith");
        
        float portX = isLeft ? 20.0f : screenWidth - scaledWidth - 20.0f;
        float portY = screenHeight - 250.0f - targetHeight + 50.0f; // slightly overlapping dialogue box
        
        Rectangle source = { 0, 0, (float)port.width, (float)port.height };
        
        Rectangle dest = { portX, portY, scaledWidth, targetHeight };
        
        DrawTexturePro(port, source, dest, {0,0}, 0.0f, WHITE);
    }

    // Background box at the bottom
    float boxHeight = 250.0f;
    Rectangle box = { 20.0f, screenHeight - boxHeight - 20.0f, screenWidth - 40.0f, boxHeight };
    DrawRectangleRec(box, { 30, 30, 30, 240 });
    DrawRectangleLinesEx(box, 6.0f, DARKGRAY);

    // Name Box
    Rectangle nameBox = { box.x + 20.0f, box.y - 40.0f, 200.0f, 60.0f };
    DrawRectangleRec(nameBox, { 50, 50, 50, 255 });
    DrawRectangleLinesEx(nameBox, 4.0f, LIGHTGRAY);
    DrawTextEx(dialogFont, node.speakerName.c_str(), { nameBox.x + 20, nameBox.y + 10 }, 48, 1, YELLOW);

    // Text (Typewriter effect)
    std::string visibleText = node.text.substr(0, visibleCharCount);
    DrawTextEx(dialogFont, visibleText.c_str(), { box.x + 40, box.y + 60 }, 44, 1, WHITE);

    // Options (Only draw if typing is done)
    if (visibleCharCount >= (int)node.text.length()) {
        int optionY = box.y + 140;
        for (int i = 0; i < (int)node.options.size(); ++i) {
            Color color = (i == selectedOption) ? YELLOW : LIGHTGRAY;
            std::string prefix = (i == selectedOption) ? "> " : "  ";
            DrawTextEx(dialogFont, (prefix + node.options[i]).c_str(), { box.x + 60, (float)optionY + (i * 50) }, 40, 1, color);
        }
    }
}
