#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "raylib.h"

#include <cstddef>
#include <string>

class TeamManager;

class PaladinSelectionMenu {
private:
    bool open;
    PaladinId inspectedPaladin;
    std::string feedbackText;
    float feedbackTimer;

    void SetFeedback(const std::string& text);

public:
    PaladinSelectionMenu();

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
