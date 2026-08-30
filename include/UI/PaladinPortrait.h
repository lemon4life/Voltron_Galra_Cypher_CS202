#pragma once

#include "raylib.h"

class Paladin;

/// Renders paladin portrait.
void DrawPaladinPortrait(const Paladin* paladin, Rectangle destination);
/// Renders paladin portrait.
void DrawPaladinPortrait(
    Texture2D idleTexture,
    Rectangle destination,
    bool downed
);
/// Renders paladin full body.
void DrawPaladinFullBody(Texture2D idleTexture, Rectangle destination);
