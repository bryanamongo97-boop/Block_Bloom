#include "credits.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

static std::string getExeDirectory()
{
    char buffer[MAX_PATH] = {0};
    if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) == 0) {
        return {};
    }
    std::string path(buffer);
    const size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? std::string() : path.substr(0, pos);
}

static bool tryOpenFile(const std::string& path)
{
    return std::ifstream(path).good();
}

CreditsAssets loadCreditsAssets(unsigned int windowWidth, unsigned int windowHeight)
{
    CreditsAssets assets;
    const unsigned int frameCount = 19;
    const float defaultDelay = 0.1f;
    assets.frames.reserve(frameCount);
    assets.frameDurations.reserve(frameCount);

    const std::string exeDir = getExeDirectory();
    std::string sourceDir;
    if (!exeDir.empty()) {
        std::string stickmanPath = exeDir + "\\Stickman\\s1.png";
        std::string picturesPath = exeDir + "\\pictures\\s1.png";
        if (tryOpenFile(stickmanPath)) {
            sourceDir = exeDir + "\\Stickman";
        } else if (tryOpenFile(picturesPath)) {
            sourceDir = exeDir + "\\pictures";
        }
    }
    if (sourceDir.empty()) {
        if (tryOpenFile("Stickman/s1.png")) {
            sourceDir = "Stickman";
        } else if (tryOpenFile("pictures/s1.png")) {
            sourceDir = "pictures";
        } else {
            std::cerr << "Credits animation directory not found. Looked for Stickman/s1.png and pictures/s1.png\n";
            return assets;
        }
    }

    // First, load all images and prepare transparency, collect sizes and baselines
    std::vector<sf::Image> images;
    images.reserve(frameCount);
    unsigned int maxW = 0, maxH = 0;
    std::vector<unsigned int> baselines;
    baselines.reserve(frameCount);

    for (unsigned int i = 1; i <= frameCount; ++i) {
        sf::Image image;
        std::string filename = sourceDir + "/s" + std::to_string(i) + ".png";
        if (!image.loadFromFile(filename)) {
            std::cerr << "Failed to load animation frame: " << filename << "\n";
            continue;
        }

        // Make background/key color or bright white transparent
        const sf::Color keyColor = image.getPixel(sf::Vector2u(0, 0));
        for (unsigned int y = 0; y < image.getSize().y; ++y) {
            for (unsigned int x = 0; x < image.getSize().x; ++x) {
                sf::Vector2u coords(x, y);
                sf::Color pixel = image.getPixel(coords);
                if ((pixel.r == keyColor.r && pixel.g == keyColor.g && pixel.b == keyColor.b) ||
                    (pixel.r > 240 && pixel.g > 240 && pixel.b > 240)) {
                    pixel.a = 0;
                    image.setPixel(coords, pixel);
                }
            }
        }

        images.push_back(std::move(image));
        const auto sz = images.back().getSize();
        maxW = std::max(maxW, sz.x);
        maxH = std::max(maxH, sz.y);
        // compute baseline (lowest non-transparent pixel row)
        int baseline = -1;
        for (int y = static_cast<int>(sz.y) - 1; y >= 0; --y) {
            bool found = false;
            for (unsigned int x = 0; x < sz.x; ++x) {
                if (images.back().getPixel(sf::Vector2u(x, static_cast<unsigned int>(y))).a > 10) { // not transparent
                    baseline = y;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (baseline < 0) baseline = static_cast<int>(sz.y); // no opaque pixels
        baselines.push_back(static_cast<unsigned int>(baseline));
    }

    if (images.empty()) {
        std::cerr << "Failed to load any stickman animation frames from " << sourceDir << "/s1.png - s19.png\n";
        return assets;
    }

    std::cerr << "Loaded " << images.size() << " stickman frames from " << sourceDir << "\n";

    // Compute a uniform scale so frames don't change size
    const sf::Vector2u refSize = images.front().getSize();
    const float maxWidth = static_cast<float>(windowWidth) * 0.55f;
    const float maxHeight = static_cast<float>(windowHeight) * 0.45f;
    const float uniformScale = std::min(maxWidth / static_cast<float>(maxW), maxHeight / static_cast<float>(maxH));

    // Convert images to textures and store per-frame origin (baseline)
    for (size_t i = 0; i < images.size(); ++i) {
        sf::Texture texture;
        if (!texture.loadFromImage(images[i])) {
            std::cerr << "Failed to create texture from image frame " << i << "\n";
            continue;
        }
        assets.frames.push_back(std::move(texture));
        assets.frameDurations.push_back(defaultDelay);
        const auto sz = images[i].getSize();
        assets.frameOrigins.push_back(sf::Vector2f(static_cast<float>(sz.x) / 2.f, static_cast<float>(baselines[i])));
    }
    assets.uniformScale = uniformScale;

    assets.currentFrameIndex = 0;
    assets.frameTimer = 0.f;
    assets.stickmanSprite = std::make_unique<sf::Sprite>(assets.frames[0]);
    // apply uniform scale and origin based on baseline for the first frame
    assets.stickmanSprite->setScale(sf::Vector2f(assets.uniformScale, assets.uniformScale));
    if (!assets.frameOrigins.empty()) {
        assets.stickmanSprite->setOrigin(assets.frameOrigins[0]);
    }
    assets.stickmanSprite->setPosition(
        sf::Vector2f(static_cast<float>(windowWidth) / 2.f, static_cast<float>(windowHeight) - 80.f)
    );

    assets.loaded = true;
    return assets;
}
