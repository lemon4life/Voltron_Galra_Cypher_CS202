#include "UI/PaladinPortrait.h"

#include "Entities/Player/Paladin.h"

#include <algorithm>

namespace {
constexpr float IDLE_FRAME_COUNT = 4.0f;

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

void DrawPaladinPortrait(const Paladin* paladin, Rectangle destination) {
    if (!paladin) {
        return;
    }
    DrawPaladinPortrait(
        paladin->GetIdleTexture(),
        destination,
        paladin->GetHealth() <= 0
    );
}

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

    DrawTexturePro(
        idleTexture,
        source,
        destination,
        {0.0f, 0.0f},
        0.0f,
        downed ? DARKGRAY : WHITE
    );
}

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

    DrawTexturePro(
        idleTexture,
        {0.0f, 0.0f, frameWidth, frameHeight},
        fitted,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}
