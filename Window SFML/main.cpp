#include <SFML/Graphics.hpp>
#include <iostream>
#include "playgame.hpp" // Connects your brand new Tetris file
#include "credits.hpp"

using namespace sf;
using namespace std;

enum class Gamestate{
    Mainmenu,
    Playing,
    Settings,
    Credits,
};

int main()
{
    const unsigned int windowWidth = 600;
    const unsigned int windowHeight = 700;
    RenderWindow window(VideoMode(Vector2u{windowWidth, windowHeight}), "BLOCK BLOOM", Style::Titlebar | Style::Close);
    window.setKeyRepeatEnabled(false);

    Gamestate currentState = Gamestate::Mainmenu;
    bool settingsFromPause = false;

    const float buttonWidth = 520.f;
    const float buttonHeight = 60.f;
    const float centerX = windowWidth / 2.f;

    RectangleShape startButton(Vector2f(buttonWidth, buttonHeight));
    startButton.setPosition(Vector2f{centerX - buttonWidth / 2.f, 260.f});
    startButton.setFillColor(Color::Blue);
    startButton.setOutlineThickness(3.f);
    startButton.setOutlineColor(Color::White);

    RectangleShape settingsButton(Vector2f(buttonWidth, buttonHeight));
    settingsButton.setPosition(Vector2f{centerX - buttonWidth / 2.f, 340.f});
    settingsButton.setFillColor(Color::Red);
    settingsButton.setOutlineThickness(3.f);
    settingsButton.setOutlineColor(Color::White);

    RectangleShape creditsButton(Vector2f(buttonWidth, buttonHeight));
    creditsButton.setPosition(Vector2f{centerX - buttonWidth / 2.f, 420.f});
    creditsButton.setFillColor(Color::Green);
    creditsButton.setOutlineThickness(3.f);
    creditsButton.setOutlineColor(Color::White);

    RectangleShape backButton(Vector2f{50.f, 50.f});
    backButton.setPosition(Vector2f{20.f, 20.f});
    backButton.setFillColor(Color(40, 40, 40));
    backButton.setOutlineThickness(3.f);
    backButton.setOutlineColor(Color::White);

    Font font;
    if (!font.openFromFile("arial.ttf")) {
        return -1;
    }

    optional<Cursor> arrowCursor = Cursor::createFromSystem(Cursor::Type::Arrow);
    optional<Cursor> handCursor = Cursor::createFromSystem(Cursor::Type::Hand);
    bool cursorLoaded = arrowCursor.has_value() && handCursor.has_value();

    float gameOverScale = 1.f;
    float gameOverOffsetX = 0.f;
    float gameOverOffsetY = 0.f;

    Texture sceneTextures[3];
    Sprite* sceneSprite = nullptr;
    Shader swayShader;
    bool hasSceneShader = false;
    int currentSceneIndex = 0;
    int nextSceneIndex = 1;
    bool isSceneTransitioning = false;
    float sceneTransitionTime = 0.f;
    float sceneCycleTimer = 0.f;
    const float sceneCycleDuration = 4.f;
    const float sceneTransitionDuration = 1.5f;

    auto getGameOverButtonBounds = [&](FloatRect& retryBounds, FloatRect& menuBounds)
    {
        const float retryOriginalX = 603.f;
        const float retryOriginalY = 650.f;
        const float menuOriginalX = 603.f;
        const float menuOriginalY = 755.f;
        const float buttonOriginalWidth = 330.f;
        const float buttonOriginalHeight = 65.f;

        retryBounds = FloatRect(
            Vector2f(retryOriginalX * gameOverScale + gameOverOffsetX,
                     retryOriginalY * gameOverScale + gameOverOffsetY),
            Vector2f(buttonOriginalWidth * gameOverScale,
                     buttonOriginalHeight * gameOverScale)
        );

        menuBounds = FloatRect(
            Vector2f(menuOriginalX * gameOverScale + gameOverOffsetX,
                     menuOriginalY * gameOverScale + gameOverOffsetY),
            Vector2f(buttonOriginalWidth * gameOverScale,
                     buttonOriginalHeight * gameOverScale)
        );
    };

    Texture gameOverTexture;
    Sprite* gameOverSprite = nullptr;
    bool hasGameOverBackground = false;
    const bool showGameOverButtonZones = false; // hide game-over debug hitboxes
    CreditsAssets creditsAssets = loadCreditsAssets(windowWidth, windowHeight);
    if (gameOverTexture.loadFromFile("pictures/gameover.jpg")) {
        gameOverSprite = new Sprite(gameOverTexture);
        const float scaleX = static_cast<float>(windowWidth) / static_cast<float>(gameOverTexture.getSize().x);
        const float scaleY = static_cast<float>(windowHeight) / static_cast<float>(gameOverTexture.getSize().y);
        const float scale = max(scaleX, scaleY);
        gameOverScale = scale;
        gameOverOffsetX = (windowWidth - gameOverTexture.getSize().x * scale) / 2.f;
        gameOverOffsetY = (windowHeight - gameOverTexture.getSize().y * scale) / 2.f;
        gameOverSprite->setScale(Vector2f(scale, scale));
        gameOverSprite->setOrigin(Vector2f(gameOverTexture.getSize().x / 2.f, gameOverTexture.getSize().y / 2.f));
        gameOverSprite->setPosition(Vector2f(windowWidth / 2.f, windowHeight / 2.f));
        hasGameOverBackground = true;
    } else {
        cerr << "Failed to load game over image: pictures/gameover.jpg\n";
    }

    // Load the scene textures and shader for smooth crossfade transitions
    const std::string sceneFiles[3] = {"pictures/a1.jpg", "pictures/a2.jpg", "pictures/a3.jpg"};
    for (int i = 0; i < 3; ++i) {
        if (!sceneTextures[i].loadFromFile(sceneFiles[i])) {
            cerr << "Failed to load scene texture: " << sceneFiles[i] << "\n";
        }
    }
    sceneSprite = new Sprite(sceneTextures[currentSceneIndex]);
    sceneSprite->setScale(Vector2f(
        static_cast<float>(windowWidth) / sceneTextures[currentSceneIndex].getSize().x,
        static_cast<float>(windowHeight) / sceneTextures[currentSceneIndex].getSize().y
    ));

    if (swayShader.loadFromFile("sway.frag", Shader::Type::Fragment)) {
        hasSceneShader = true;
    } else {
        cerr << "Failed to load shader: sway.frag\n";
    }

    // Instantiating our newly separated PlayGame gameplay module
    PlayGame tetrisGame(font);
    Clock gameClock;
    Clock shaderClock;

    auto centerTextInShape = [&](Text& text, const RectangleShape& shape)
    {
        FloatRect bounds = text.getLocalBounds();
        text.setOrigin(Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
        text.setPosition(shape.getPosition() + shape.getSize() / 2.f);
    };

    Text titleText(font, "BLOCK BLOOM", 40);
    titleText.setFillColor(Color::Black);
    FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(Vector2f(titleBounds.position.x + titleBounds.size.x / 2.f, titleBounds.position.y + titleBounds.size.y / 2.f));
    titleText.setPosition(Vector2f{centerX, 90.f});

    Text subtitleText(font, "- TETRIS -", 28);
    subtitleText.setFillColor(Color::Black);
    FloatRect subtitleBounds = subtitleText.getLocalBounds();
    subtitleText.setOrigin(Vector2f(subtitleBounds.position.x + subtitleBounds.size.x / 2.f, subtitleBounds.position.y + subtitleBounds.size.y / 2.f));
    subtitleText.setPosition(Vector2f{centerX, 135.f});

    Text playText(font, "PLAY GAME", 24);
    playText.setFillColor(Color::White);
    centerTextInShape(playText, startButton);

    Text settingsText(font, "SETTINGS", 24);
    settingsText.setFillColor(Color::White);
    centerTextInShape(settingsText, settingsButton);

    Text creditsText(font, "CREDITS", 24);
    creditsText.setFillColor(Color::White);
    centerTextInShape(creditsText, creditsButton);

    Text backText(font, "<", 28);
    backText.setFillColor(Color::White);
    centerTextInShape(backText, backButton);

    RectangleShape pauseBar1(Vector2f{4.f, 16.f});
    pauseBar1.setFillColor(Color::White);
    pauseBar1.setPosition(Vector2f(
        backButton.getPosition().x + backButton.getSize().x / 2.f - 5.f, // Centered alignment adjustment
        backButton.getPosition().y + backButton.getSize().y / 2.f - 8.f
    ));

    RectangleShape pauseBar2(Vector2f{4.f, 16.f});
    pauseBar2.setFillColor(Color::White);
    pauseBar2.setPosition(Vector2f(
        backButton.getPosition().x + backButton.getSize().x / 2.f + 2.f, // Centered alignment adjustment
        backButton.getPosition().y + backButton.getSize().y / 2.f - 8.f
    ));

    while (window.isOpen())
    {
        float elapsedTime = gameClock.restart().asSeconds();

        // Feed step timers down to game loop logic dynamically when running
        bool gameOverSignal = false;
        if (currentState == Gamestate::Playing) {
            tetrisGame.update(elapsedTime, gameOverSignal);
            if (gameOverSignal && !tetrisGame.getGameOver()) {
                tetrisGame.enterGameOver();
            }
        }

        while (const optional event = window.pollEvent())
        { 
            if(event->is<Event::Closed>())
            {
                window.close();
            }

            // Route key controls strictly to your active game session module
            if (currentState == Gamestate::Playing) {
                bool handledHere = false;

                // Always notify key-release events so PlayGame can clear suppression flags
                if (const auto* keyReleased = event->getIf<Event::KeyReleased>()) {
                    printf("[DBG_MAIN] KeyReleased code=%d paused=%d\n", static_cast<int>(keyReleased->code), tetrisGame.getPaused() ? 1 : 0);
                    tetrisGame.notifyKeyReleased(keyReleased->code);
                    handledHere = true;
                }

                if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) {
                    auto code = keyPressed->code;
                    printf("[DBG_MAIN] KeyPressed code=%d paused=%d\n", static_cast<int>(code), tetrisGame.getPaused() ? 1 : 0);
                    // Toggle pause keys handled immediately
                    if (code == Keyboard::Key::P || code == Keyboard::Key::Escape) {
                        tetrisGame.togglePause();
                        handledHere = true;
                    }

                    // If paused, route pause-navigation keys to the pause handler in PlayGame
                    else if (tetrisGame.getPaused()) {
                        if (code == Keyboard::Key::Up || code == Keyboard::Key::Down || code == Keyboard::Key::Space) {
                            tetrisGame.handlePauseKey(code);
                            handledHere = true;
                        } else {
                            // Consume other key presses while paused so they don't reach gameplay
                            handledHere = true;
                        }
                    }
                }

                if (!handledHere) tetrisGame.handleInput(*event, gameOverSignal);

                int kAction = tetrisGame.consumePendingPauseAction();
                if (kAction != 0) {
                    if (kAction == 1) {
                        tetrisGame.togglePause();
                    } else if (kAction == 2) {
                        settingsFromPause = true;
                        currentState = Gamestate::Settings;
                    } else if (kAction == 3) {
                        tetrisGame.togglePause();
                        tetrisGame.reset();
                        currentState = Gamestate::Mainmenu;
                    }
                }

                if (gameOverSignal && !tetrisGame.getGameOver()) {
                    tetrisGame.enterGameOver();
                }

                int goAction = tetrisGame.consumePendingGameOverAction();
                if (goAction != 0) {
                    if (goAction == 1) {
                        tetrisGame.reset();
                    } else if (goAction == 2) {
                        tetrisGame.reset();
                        currentState = Gamestate::Mainmenu;
                    }
                }
            }



            if (const auto* mouseMoved = event->getIf<Event::MouseMoved>())
            {
                Vector2f mousePosF(static_cast<float>(mouseMoved->position.x), static_cast<float>(mouseMoved->position.y));
                bool hoverHand = false;

                if (currentState == Gamestate::Playing) {
                    if (tetrisGame.getGameOver()) {
                        FloatRect retryBounds, menuBounds;
                        getGameOverButtonBounds(retryBounds, menuBounds);
                        hoverHand = retryBounds.contains(mousePosF) || menuBounds.contains(mousePosF);
                    } else {
                        hoverHand = pauseBar1.getGlobalBounds().contains(mousePosF)
                                 || pauseBar2.getGlobalBounds().contains(mousePosF);
                    }
                }
                else if (currentState == Gamestate::Mainmenu) {
                    hoverHand = startButton.getGlobalBounds().contains(mousePosF)
                             || settingsButton.getGlobalBounds().contains(mousePosF)
                             || creditsButton.getGlobalBounds().contains(mousePosF);
                }
                else if (currentState == Gamestate::Settings || currentState == Gamestate::Credits) {
                    hoverHand = backButton.getGlobalBounds().contains(mousePosF);
                }

                if (cursorLoaded) {
                    window.setMouseCursor(hoverHand ? *handCursor : *arrowCursor);
                }
            }

            if(const auto* mousePressed = event->getIf<Event::MouseButtonPressed>())
            {
                if(mousePressed->button == Mouse::Button::Left)
                {
                    Vector2i mousePos(mousePressed->position.x, mousePressed->position.y);
                    Vector2f mousePosF(mousePos);

                    if (currentState == Gamestate::Playing && tetrisGame.getGameOver()) {
                        FloatRect retryBounds, menuBounds;
                        getGameOverButtonBounds(retryBounds, menuBounds);

                        if (retryBounds.contains(mousePosF)) {
                            tetrisGame.reset();
                            currentState = Gamestate::Playing;
                            continue;
                        }
                        if (menuBounds.contains(mousePosF)) {
                            tetrisGame.reset();
                            currentState = Gamestate::Mainmenu;
                            continue;
                        }
                    }

                    if(currentState == Gamestate::Mainmenu)
                    {
                        if(startButton.getGlobalBounds().contains(mousePosF))
                        {
                            tetrisGame.reset(); // Always clear old blocks when hitting play button
                            currentState = Gamestate::Playing;
                        } 
                        else if (settingsButton.getGlobalBounds().contains(mousePosF))
                        {
                            currentState = Gamestate::Settings;
                            settingsFromPause = false;
                        } 
                        else if (creditsButton.getGlobalBounds().contains(mousePosF))
                        {
                            currentState = Gamestate::Credits;
                        }
                    }

                    // Pause menu click handling when paused
                    if (currentState == Gamestate::Playing && tetrisGame.getPaused()) {
                        int action = tetrisGame.pauseMenuActionAt(mousePosF);
                        if (action == 1) {
                            tetrisGame.togglePause(); // Resume
                        } else if (action == 2) {
                            // Enter settings from pause; remember origin so we can return to game
                            settingsFromPause = true;
                            currentState = Gamestate::Settings;
                        } else if (action == 3) {
                            tetrisGame.togglePause();
                            tetrisGame.reset();
                            currentState = Gamestate::Mainmenu;
                        }
                    }

                    // Pause and back button handling (when not clicking pause menu)
                    if (currentState == Gamestate::Playing &&
                        (pauseBar1.getGlobalBounds().contains(mousePosF) || pauseBar2.getGlobalBounds().contains(mousePosF)))
                    {
                        tetrisGame.togglePause();
                    }
                    else if(backButton.getGlobalBounds().contains(mousePosF))
                    {
                        if (currentState == Gamestate::Playing) {
                            tetrisGame.togglePause(); // Toggles freeze state without wiping matrix data
                        } else if (currentState == Gamestate::Settings) {
                            if (settingsFromPause) {
                                // Return to the paused game WITHOUT resuming
                                currentState = Gamestate::Playing;
                                settingsFromPause = false;
                            } else {
                                currentState = Gamestate::Mainmenu;
                            }
                        } else {
                            currentState = Gamestate::Mainmenu;
                        }
                    }
                }
            }
        }

        // --- DRAW ZONE ---
        if(currentState == Gamestate::Mainmenu)
        {
            if (hasSceneShader && sceneSprite) {
                if (isSceneTransitioning) {
                    sceneTransitionTime += elapsedTime;
                    float progress = sceneTransitionTime / sceneTransitionDuration;
                    if (progress >= 1.f) {
                        progress = 1.f;
                        currentSceneIndex = nextSceneIndex;
                        nextSceneIndex = (currentSceneIndex + 1) % 3;
                        isSceneTransitioning = false;
                        sceneTransitionTime = 0.f;
                        sceneSprite->setTexture(sceneTextures[currentSceneIndex]);
                        sceneSprite->setScale(Vector2f(
                            static_cast<float>(windowWidth) / sceneTextures[currentSceneIndex].getSize().x,
                            static_cast<float>(windowHeight) / sceneTextures[currentSceneIndex].getSize().y
                        ));
                    }
                    swayShader.setUniform("textureCurrent", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("textureNext", sceneTextures[nextSceneIndex]);
                    swayShader.setUniform("time", shaderClock.getElapsedTime().asSeconds());
                    swayShader.setUniform("progress", progress);
                    window.clear(Color::Black);
                    window.draw(*sceneSprite, &swayShader);
                } else {
                    sceneCycleTimer += elapsedTime;
                    if (sceneCycleTimer >= sceneCycleDuration) {
                        sceneCycleTimer = 0.f;
                        nextSceneIndex = (currentSceneIndex + 1) % 3;
                        isSceneTransitioning = true;
                        sceneTransitionTime = 0.f;
                    }
                    swayShader.setUniform("textureCurrent", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("textureNext", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("time", shaderClock.getElapsedTime().asSeconds());
                    swayShader.setUniform("progress", 0.f);
                    window.clear(Color::Black);
                    window.draw(*sceneSprite, &swayShader);
                }
            } else {
                window.clear(Color::White);
            }
            window.draw(startButton);
            window.draw(settingsButton);
            window.draw(creditsButton);
            window.draw(titleText);
            window.draw(subtitleText);
            window.draw(playText);
            window.draw(settingsText);
            window.draw(creditsText);
        }
        else if (currentState == Gamestate::Playing)
        {
            if (tetrisGame.getGameOver() && hasGameOverBackground && gameOverSprite) {
                window.clear(Color::Black);
                window.draw(*gameOverSprite);
                if (showGameOverButtonZones) {
                    FloatRect retryBounds, menuBounds;
                    getGameOverButtonBounds(retryBounds, menuBounds);

                    RectangleShape retryHighlight(Vector2f(retryBounds.size.x, retryBounds.size.y));
                    retryHighlight.setPosition(Vector2f(retryBounds.position.x, retryBounds.position.y));
                    retryHighlight.setFillColor(Color(255, 255, 255, 50));
                    retryHighlight.setOutlineColor(Color::Green);
                    retryHighlight.setOutlineThickness(2.f);
                    window.draw(retryHighlight);

                    RectangleShape menuHighlight(Vector2f(menuBounds.size.x, menuBounds.size.y));
                    menuHighlight.setPosition(Vector2f(menuBounds.position.x, menuBounds.position.y));
                    menuHighlight.setFillColor(Color(255, 255, 255, 50));
                    menuHighlight.setOutlineColor(Color::Yellow);
                    menuHighlight.setOutlineThickness(2.f);
                    window.draw(menuHighlight);
                }
            } else {
                window.clear(Color(30, 30, 45)); // Slate background looks fantastic for gameplay panels!
                tetrisGame.draw(window);
                window.draw(pauseBar1);
                window.draw(pauseBar2);
            }
        } 
        else if (currentState == Gamestate::Settings)
        {
            window.clear(Color::Cyan);
            window.draw(backButton);
            window.draw(backText);
        }
        else if (currentState == Gamestate::Credits)
        {
            if (hasSceneShader && sceneSprite) {
                if (isSceneTransitioning) {
                    sceneTransitionTime += elapsedTime;
                    float progress = sceneTransitionTime / sceneTransitionDuration;
                    if (progress >= 1.f) {
                        progress = 1.f;
                        currentSceneIndex = nextSceneIndex;
                        nextSceneIndex = (currentSceneIndex + 1) % 3;
                        isSceneTransitioning = false;
                        sceneTransitionTime = 0.f;
                        sceneSprite->setTexture(sceneTextures[currentSceneIndex]);
                        sceneSprite->setScale(Vector2f(
                            static_cast<float>(windowWidth) / sceneTextures[currentSceneIndex].getSize().x,
                            static_cast<float>(windowHeight) / sceneTextures[currentSceneIndex].getSize().y
                        ));
                    }
                    swayShader.setUniform("textureCurrent", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("textureNext", sceneTextures[nextSceneIndex]);
                    swayShader.setUniform("time", shaderClock.getElapsedTime().asSeconds());
                    swayShader.setUniform("progress", progress);
                    window.clear(Color::Black);
                    window.draw(*sceneSprite, &swayShader);
                } else {
                    sceneCycleTimer += elapsedTime;
                    if (sceneCycleTimer >= sceneCycleDuration) {
                        sceneCycleTimer = 0.f;
                        nextSceneIndex = (currentSceneIndex + 1) % 3;
                        isSceneTransitioning = true;
                        sceneTransitionTime = 0.f;
                    }
                    swayShader.setUniform("textureCurrent", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("textureNext", sceneTextures[currentSceneIndex]);
                    swayShader.setUniform("time", shaderClock.getElapsedTime().asSeconds());
                    swayShader.setUniform("progress", 0.f);
                    window.clear(Color::Black);
                    window.draw(*sceneSprite, &swayShader);
                }
            } else {
                window.clear(Color(255, 165, 0));
            }

            if (creditsAssets.loaded && creditsAssets.stickmanSprite) {
                if (creditsAssets.frames.size() > 1) {
                    creditsAssets.frameTimer += elapsedTime;
                    float delay = creditsAssets.frameDurations[creditsAssets.currentFrameIndex];
                    if (creditsAssets.frameTimer >= delay) {
                        creditsAssets.frameTimer -= delay;
                        creditsAssets.currentFrameIndex = (creditsAssets.currentFrameIndex + 1) % creditsAssets.frames.size();
                        // update texture and set origin/scale/position using precomputed values to avoid jumps
                        creditsAssets.stickmanSprite->setTexture(creditsAssets.frames[creditsAssets.currentFrameIndex]);
                        if (creditsAssets.frameOrigins.size() > creditsAssets.currentFrameIndex) {
                            creditsAssets.stickmanSprite->setOrigin(creditsAssets.frameOrigins[creditsAssets.currentFrameIndex]);
                        }
                        creditsAssets.stickmanSprite->setScale(sf::Vector2f(creditsAssets.uniformScale, creditsAssets.uniformScale));
                        creditsAssets.stickmanSprite->setPosition(sf::Vector2f(static_cast<float>(windowWidth) / 2.f, static_cast<float>(windowHeight) - 80.f));
                    }
                }
                // draw stickman and UI in screen-space so background view transitions don't move him
                sf::View oldView = window.getView();
                window.setView(window.getDefaultView());
                window.draw(*creditsAssets.stickmanSprite);
                window.draw(backButton);
                window.draw(backText);
                window.setView(oldView);
            }
        }
        window.display();
    }
    return 0;
}
