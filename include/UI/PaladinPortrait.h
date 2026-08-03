#pragma once

#include "raylib.h"

class Paladin;

void DrawPaladinPortrait(const Paladin* paladin, Rectangle destination);
void DrawPaladinPortrait(
    Texture2D idleTexture,
    Rectangle destination,
    bool downed
);
void DrawPaladinFullBody(Texture2D idleTexture, Rectangle destination);
