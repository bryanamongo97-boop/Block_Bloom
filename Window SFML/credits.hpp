#pragma once

#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>

struct CreditsAssets {
    std::vector<sf::Texture> frames;
    std::vector<float> frameDurations;
    std::vector<sf::Vector2f> frameOrigins; // origin (in pixels) for each frame to align baseline
    float uniformScale = 1.f; // common scale applied to all frames to avoid jumping
    sf::Vector2f commonOrigin = sf::Vector2f(0.f, 0.f); // a fixed origin to use when locking position
    std::unique_ptr<sf::Sprite> stickmanSprite;
    std::size_t currentFrameIndex = 0;
    float frameTimer = 0.f;
    bool loaded = false;
};

CreditsAssets loadCreditsAssets(unsigned int windowWidth, unsigned int windowHeight);
