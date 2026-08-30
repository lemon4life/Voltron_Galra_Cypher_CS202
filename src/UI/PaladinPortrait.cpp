#include "UI/PaladinPortrait.h"

#include "Entities/Player/Paladin.h"
#include "Core/Manager/AssetManager.h"

#include <algorithm>

namespace {
constexpr float IDLE_FRAME_COUNT = 4.0f;

/// Crops to aspect.
Rectangle CropToAspect(Rectangle source, Rectangle destination) {
    float destinationAspect = destination.width / destination.height;
    float sourceAspect = source.width / source.height;

    if (destinationAspect > sourceAspect) {
        float cropHeight = source.width / destinationAspect;
        source.y += (source.height - cropHeight) * 0.5f;
        source.height = cropHeight;
    } else {
        float cropWidth = source.height * destinationAspect;
        source.x += (source.width - cropWidth) * 0.5f;
        source.width = cropWidth;
    }
    return source;
}
}

/// Renders paladin portrait.
void DrawPaladinPortrait(const Paladin* paladin, Rectangle destination) {
    if (!paladin) {
        return;
    }
    
    Texture2D cardTex = AssetManager::GetInstance().GetTexture(paladin->GetIntroData().portraitTextureID);
    if (cardTex.id == 0) return;
    
    Rectangle source = paladin->GetHudPortraitSlice();
    source = CropToAspect(source, destination);
    
    source.x = std::round(source.x);
    source.y = std::round(source.y);
    source.width = std::round(source.width);
    source.height = std::round(source.height);

    destination.x = std::round(destination.x);
    destination.y = std::round(destination.y);
    destination.width = std::round(destination.width);
    destination.height = std::round(destination.height);
    
    Color tint = paladin->GetHealth() <= 0 ? DARKGRAY : WHITE;
    DrawTexturePro(cardTex, source, destination, {0.0f, 0.0f}, 0.0f, tint);
}

/// Renders paladin portrait.
void DrawPaladinPortrait(
    Texture2D idleTexture,
    Rectangle destination,
    bool downed
) {
    if (idleTexture.id == 0 || destination.width <= 0.0f ||
        destination.height <= 0.0f) {
        return;
    }

    float frameWidth = (float)idleTexture.width / IDLE_FRAME_COUNT;
    float frameHeight = (float)idleTexture.height;
    Rectangle source = {
        0.0f,
        2.0f,
        frameWidth,
        frameHeight * 0.5f + 2.0f
    };
    source = CropToAspect(source, destination);

    source.x = std::round(source.x);
    source.y = std::round(source.y);
    source.width = std::round(source.width);
    source.height = std::round(source.height);

    destination.x = std::round(destination.x);
    destination.y = std::round(destination.y);
    destination.width = std::round(destination.width);
    destination.height = std::round(destination.height);

    DrawTexturePro(
        idleTexture,
        source,
        destination,
        {0.0f, 0.0f},
        0.0f,
        downed ? DARKGRAY : WHITE
    );
}

/// Renders paladin full body.
void DrawPaladinFullBody(Texture2D idleTexture, Rectangle destination) {
    if (idleTexture.id == 0 || destination.width <= 0.0f ||
        destination.height <= 0.0f) {
        return;
    }

    float frameWidth = (float)idleTexture.width / IDLE_FRAME_COUNT;
    float frameHeight = (float)idleTexture.height;
    float scale = std::min(
        destination.width / frameWidth,
        destination.height / frameHeight
    );
    Rectangle fitted = {
        destination.x + (destination.width - frameWidth * scale) * 0.5f,
        destination.y + (destination.height - frameHeight * scale) * 0.5f,
        frameWidth * scale,
        frameHeight * scale
    };

    fitted.x = std::round(fitted.x);
    fitted.y = std::round(fitted.y);
    fitted.width = std::round(fitted.width);
    fitted.height = std::round(fitted.height);

    Rectangle source = {0.0f, 0.0f, frameWidth, frameHeight};
    source.x = std::round(source.x);
    source.y = std::round(source.y);
    source.width = std::round(source.width);
    source.height = std::round(source.height);

    DrawTexturePro(
        idleTexture,
        source,
        fitted,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}
