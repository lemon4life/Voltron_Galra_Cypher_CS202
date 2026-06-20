#include "Core/DialogueManager.h"

DialogueManager::DialogueManager() : isDialogueActive(false), currentNode(0), selectedOption(0), missionRequested(false) {}

DialogueManager& DialogueManager::GetInstance() {
    static DialogueManager instance;
    return instance;
}

void DialogueManager::StartDialogue(const std::vector<DialogueNode>& tree) {
    currentTree = tree;
    currentNode = 0;
    selectedOption = 0;
    isDialogueActive = true;
    missionRequested = false;
}

void DialogueManager::Update(float deltaTime) {
    if (!isDialogueActive || currentTree.empty()) return;

    const DialogueNode& node = currentTree[currentNode];

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        selectedOption--;
        if (selectedOption < 0) selectedOption = node.options.size() - 1;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        selectedOption++;
        if (selectedOption >= (int)node.options.size()) selectedOption = 0;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        int next = node.nextNodeIndices[selectedOption];
        if (next == -1) {
            // End dialogue and transition
            isDialogueActive = false;
            missionRequested = true;
        } else {
            currentNode = next;
            selectedOption = 0;
        }
    }
}

void DialogueManager::Draw() {
    if (!isDialogueActive || currentTree.empty()) return;

    const DialogueNode& node = currentTree[currentNode];

    // Background box at the bottom
    Rectangle box = { 10.0f, 512.0f - 160.0f, 512.0f - 20.0f, 150.0f };
    DrawRectangleRec(box, { 20, 20, 20, 220 });
    DrawRectangleLinesEx(box, 2.0f, LIGHTGRAY);

    // Speaker Name
    DrawText(node.speakerName.c_str(), box.x + 10, box.y + 10, 20, YELLOW);

    // Text
    DrawText(node.text.c_str(), box.x + 10, box.y + 40, 18, WHITE);

    // Options
    int optionY = box.y + 80;
    for (int i = 0; i < (int)node.options.size(); ++i) {
        Color color = (i == selectedOption) ? YELLOW : LIGHTGRAY;
        std::string prefix = (i == selectedOption) ? "> " : "  ";
        DrawText((prefix + node.options[i]).c_str(), box.x + 20, optionY + (i * 20), 16, color);
    }
}
