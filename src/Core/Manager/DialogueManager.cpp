#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
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
                // Typewriter effect sound removed per user request
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
            AudioManager::GetInstance().PlayRandomClick();
        }
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
            selectedOption++;
            if (selectedOption >= (int)node.options.size()) selectedOption = 0;
            AudioManager::GetInstance().PlayRandomClick();
        }

        if (IsKeyPressed(KEY_ENTER)) {
            AudioManager::GetInstance().PlayRandomClick();
            if (node.options.empty()) {
                // End dialogue if no options
                int next = -1;
                if (!node.nextNodeIndices.empty()) next = node.nextNodeIndices[0];

                if (next < 0) { // e.g., -1 or -2
                    isDialogueActive = false;
                    missionRequested = true;
                    requestedMissionId = next;
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
                    if (entity->GetObjectType() == GameObjectType::Player) {
                        Paladin* p = static_cast<Paladin*>(entity);
                        if (p->GetTeamManager()->GetActiveIndex() != 0) {
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

void DialogueManager::Draw(int screenWidth, int screenHeight) {
    if (!isDialogueActive || currentTree.empty()) return;

    const DialogueNode& node = currentTree[currentNode];
    constexpr float MARGIN = 10.0f;
    constexpr float PORTRAIT_HEIGHT = 400.0f;
    constexpr float PORTRAIT_BOTTOM_OFFSET = 100.0f;
    constexpr float BOX_HEIGHT = 125.0f;
    constexpr float TEXT_SPACING = 0.5f;
    const float logicalWidth = static_cast<float>(screenWidth);
    const float logicalHeight = static_cast<float>(screenHeight);

    // Render portrait
    if (portraits.find(node.speakerName) != portraits.end()) {
        Texture2D port = portraits[node.speakerName];

        float targetHeight = PORTRAIT_HEIGHT;
        float scale = targetHeight / (float)port.height;
        float scaledWidth = (float)port.width * scale;

        bool isLeft = (node.speakerName == "Lance" || node.speakerName == "Keith");

        float portX = isLeft
            ? MARGIN
            : logicalWidth - scaledWidth - MARGIN;
        float portY =
            logicalHeight - PORTRAIT_BOTTOM_OFFSET - targetHeight;

        Rectangle source = { 0, 0, (float)port.width, (float)port.height };

        Rectangle dest = { portX, portY, scaledWidth, targetHeight };

        DrawTexturePro(port, source, dest, {0,0}, 0.0f, WHITE);
    }

    // Background box at the bottom
    Rectangle box = {
        MARGIN,
        logicalHeight - BOX_HEIGHT - MARGIN,
        logicalWidth - MARGIN * 2.0f,
        BOX_HEIGHT
    };
    DrawRectangleRec(box, { 30, 30, 30, 240 });
    DrawRectangleLinesEx(box, 3.0f, DARKGRAY);

    // Name Box
    Rectangle nameBox = {
        box.x + 10.0f,
        box.y - 20.0f,
        100.0f,
        30.0f
    };
    DrawRectangleRec(nameBox, { 50, 50, 50, 255 });
    DrawRectangleLinesEx(nameBox, 2.0f, LIGHTGRAY);
    DrawTextEx(
        dialogFont,
        node.speakerName.c_str(),
        {
            nameBox.x + 10.0f,
            nameBox.y + 5.0f
        },
        24.0f,
        TEXT_SPACING,
        YELLOW
    );

    // Text (Typewriter effect)
    std::string visibleText = node.text.substr(0, visibleCharCount);
    DrawTextEx(
        dialogFont,
        visibleText.c_str(),
        {
            box.x + 20.0f,
            box.y + 30.0f
        },
        22.0f,
        TEXT_SPACING,
        WHITE
    );

    // Options (Only draw if typing is done)
    if (visibleCharCount >= (int)node.text.length()) {
        float optionY = box.y + 70.0f;
        for (int i = 0; i < (int)node.options.size(); ++i) {
            Color color = (i == selectedOption) ? YELLOW : LIGHTGRAY;
            std::string prefix = (i == selectedOption) ? "> " : "  ";
            DrawTextEx(
                dialogFont,
                (prefix + node.options[i]).c_str(),
                {
                    box.x + 30.0f,
                    optionY + (float)i * 25.0f
                },
                20.0f,
                TEXT_SPACING,
                color
            );
        }
    }
}
