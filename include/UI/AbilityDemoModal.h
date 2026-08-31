#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "UI/DemoSandbox.h"
#include "raylib.h"
#include <string>

struct ModalLayout {
    Rectangle container;
    Rectangle stagePanel;
    Rectangle cardsPanel;
    Rectangle backButton;
    float cardHeight;
    float cardGap;
    float scale;
    float titleFontSize;
    float cardTitleFontSize;
    float cardTagFontSize;
    float cardDescFontSize;
    float backBtnFontSize;

    Rectangle GetCardBounds(int index) const {
        return {
            cardsPanel.x,
            cardsPanel.y + index * (cardHeight + cardGap),
            cardsPanel.width,
            cardHeight
        };
    }
};

// Design Pattern - View Component / Modal:
// AbilityDemoModal coordinates the character ability preview showcase UI,
// dynamically calculating responsive layout dimensions and delegating
// live combat simulation polymorphically to DemoSandbox.
class AbilityDemoModal {
private:
    bool open;
    PaladinId currentPaladin;
    DemoSandbox sandbox;

    void DrawRightActionCards(const ModalLayout& layout, Vector2 mousePosition) const;

public:
    AbilityDemoModal();
    ~AbilityDemoModal();

    /// Calculates responsive modal dimensions anchored to screen resolution.
    static ModalLayout CalculateLayout(int screenWidth, int screenHeight);

    /// Opens the ability demo modal for the inspected Paladin.
    void Open(PaladinId paladinId);
    /// Closes the modal and resets simulation state.
    void Close();
    /// Reports whether the demo modal is currently open.
    bool IsOpen() const { return open; }

    /// Advances the demo animation and handles card switching.
    void Update(float deltaTime, Vector2 mousePosition);
    /// Renders the modal backdrop, arena stage, and action cards.
    void Draw(Vector2 mousePosition) const;
};
