#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "raylib.h"
#include <cstddef>
#include <string>

class TeamManager;
class Paladin;

class EnhanceMenuUI {
private:
    bool open = false;
    PaladinId inspectedPaladin = PaladinId::Lance;
    std::string feedbackText;
    float feedbackTimer = 0.0f;

    void SetFeedback(const std::string& text);

public:
    EnhanceMenuUI();
    ~EnhanceMenuUI() = default;

    void Open(PaladinId paladinId);
    void Close();
    bool IsOpen() const { return open; }

    void Update(
        float deltaTime,
        Vector2 mousePosition,
        TeamManager& teamManager
    );
    void Draw(Vector2 mousePosition, const TeamManager& teamManager) const;
};
