
#include "playgame.hpp"
#include <iostream>
#include <cstdio>
#include <ctime>
#include <string>
#include <algorithm>
#include <set>

using namespace sf;
using namespace std;

static const TetrisPoint tetrominoes[7][4] = {
    { {-1, 0}, {0, 0}, {1, 0}, {2, 0} },  // 1: I Block
    { {0, 0},  {1, 0}, {0, 1}, {1, 1} },  // 2: O Block
    { {-1, 0}, {0, 0}, {1, 0}, {0, 1} },  // 3: T Block
    { {0, 0},  {1, 0}, {-1, 1},{0, 1} },  // 4: S Block
    { {-1, 0}, {0, 0}, {0, 1}, {1, 1} },  // 5: Z Block
    { {-1, 0}, {0, 0}, {1, 0}, {1, 1} },  // 6: J Block
    { {-1, 0}, {0, 0}, {1, 0}, {-1, 1} }  // 7: L Block
};

static const Color blockColors[] = {
    Color::Black, Color::Cyan, Color::Yellow, Color::Magenta,
    Color::Green, Color::Red, Color::Blue, Color(255, 165, 0)
};

PlayGame::PlayGame(const Font& font)
    : fontRef(font)
    , scoreText(fontRef, "SCORE\n0", 20)
    , comboText(fontRef, "COMBO\nx1", 20)
    , holdTitle(fontRef, "HOLD", 18)
    , nextTitle(fontRef, "NEXT", 18)
    , resumeText(fontRef, "RESUME", 28)
    , settingsText(fontRef, "SETTINGS", 22)
    , quitText(fontRef, "QUIT", 22)
    , gameOverText(fontRef, "GAME OVER", 48)
    , gameOverRestartText(fontRef, "PRESS R OR SPACE TO RESTART", 18)
    , gameOverMenuText(fontRef, "PRESS ESC TO RETURN TO MENU", 18)
{
    srand(static_cast<unsigned int>(time(0)));
    fallDelay = 0.4f;
    lockTimer = 0.f;
    instantDropCooldown = 0.f;
    isPaused = false;
    committingPlacement = false;

    boardFrame.setSize(Vector2f(COLS * TILE_SIZE, ROWS * TILE_SIZE));
    boardFrame.setPosition(BOARD_OFFSET);
    boardFrame.setFillColor(Color(20, 20, 20));
    boardFrame.setOutlineThickness(3.f);
    boardFrame.setOutlineColor(Color::White);

    blockSprite.setSize(Vector2f(TILE_SIZE - 2.f, TILE_SIZE - 2.f));
    previewBlock.setSize(Vector2f(18.f, 18.f));

    scoreText.setFillColor(Color::White);
    scoreText.setPosition(Vector2f(475.f, 60.f));

    comboText.setFillColor(Color::Yellow);
    comboText.setPosition(Vector2f(475.f, 280.f));

    holdFrame.setSize(Vector2f(100.f, 100.f));
    holdFrame.setPosition(Vector2f(25.f, 150.f));
    holdFrame.setFillColor(Color(20, 20, 20));
    holdFrame.setOutlineThickness(3.f);
    holdFrame.setOutlineColor(Color::White);

    holdTitle.setFillColor(Color::White);
    holdTitle.setPosition(Vector2f(25.f, 120.f));

    nextFrame.setSize(Vector2f(100.f, 100.f));
    nextFrame.setPosition(Vector2f(475.f, 150.f));
    nextFrame.setFillColor(Color(20, 20, 20));
    nextFrame.setOutlineThickness(3.f);
    nextFrame.setOutlineColor(Color::White);

    nextTitle.setFillColor(Color::White);
    nextTitle.setPosition(Vector2f(475.f, 120.f));

    resumeText.setFillColor(Color::Yellow);
    settingsText.setFillColor(Color::White);
    quitText.setFillColor(Color::White);

    Vector2f center = BOARD_OFFSET + Vector2f((COLS * TILE_SIZE) / 2.f, (ROWS * TILE_SIZE) / 2.f);

    FloatRect rBounds = resumeText.getLocalBounds();
    resumeText.setOrigin(Vector2f(rBounds.position.x + rBounds.size.x / 2.f, rBounds.position.y + rBounds.size.y / 2.f));
    resumeText.setPosition(center + Vector2f(0.f, -36.f));

    FloatRect sBounds = settingsText.getLocalBounds();
    settingsText.setOrigin(Vector2f(sBounds.position.x + sBounds.size.x / 2.f, sBounds.position.y + sBounds.size.y / 2.f));
    settingsText.setPosition(center + Vector2f(0.f, 0.f));

    FloatRect qBounds = quitText.getLocalBounds();
    quitText.setOrigin(Vector2f(qBounds.position.x + qBounds.size.x / 2.f, qBounds.position.y + qBounds.size.y / 2.f));
    quitText.setPosition(center + Vector2f(0.f, 36.f));

    gameOverText.setFillColor(Color::Red);
    FloatRect goBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(Vector2f(goBounds.position.x + goBounds.size.x / 2.f, goBounds.position.y + goBounds.size.y / 2.f));
    gameOverText.setPosition(center + Vector2f(0.f, -40.f));

    gameOverRestartText.setFillColor(Color::White);
    gameOverRestartText.setString("PRESS R OR SPACE TO RESTART");
    FloatRect restartBounds = gameOverRestartText.getLocalBounds();
    gameOverRestartText.setOrigin(Vector2f(restartBounds.position.x + restartBounds.size.x / 2.f, restartBounds.position.y + restartBounds.size.y / 2.f));
    gameOverRestartText.setPosition(center + Vector2f(0.f, 40.f));

    gameOverMenuText.setFillColor(Color::White);
    gameOverMenuText.setString("PRESS ESC TO RETURN TO MENU");
    FloatRect menuBounds = gameOverMenuText.getLocalBounds();
    gameOverMenuText.setOrigin(Vector2f(menuBounds.position.x + menuBounds.size.x / 2.f, menuBounds.position.y + menuBounds.size.y / 2.f));
    gameOverMenuText.setPosition(center + Vector2f(0.f, 60.f));

    gameOverImageLoaded = false;
    if (!gameOverTexture.loadFromFile("pictures/gameover.jpg")) {
        std::cerr << "Failed to load game over image: pictures/gameover.jpg\n";
    } else {
        gameOverSprite = std::make_unique<sf::Sprite>(gameOverTexture);
        gameOverImageLoaded = true;
    }

    reset();
}

void PlayGame::reset() {
    score = 0;
    combo = 0;
    timer = 0.f;
    lockTimer = 0.f;
    instantDropCooldown = 0.f;
    holdType = 0; 
    canHold = true;
    isPaused = false;
    isGameOver = false;
    pauseSelection = 1;
    pendingPauseAction = 0;
    pendingGameOverAction = 0;
    scoreText.setString("SCORE\n0");
    comboText.setString("COMBO\nx1");
    
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) grid[r][c] = 0;
    }
    
    nextType = rand() % 7 + 1;
    spawnPiece();

    // initialize suppression flags
    suppressHeldInputs = false;
    suppressLeft = suppressRight = suppressShift = suppressSpace = false;
    // initialize cooldowns
    moveCooldown = rotateCooldown = holdCooldown = hardDropCooldown = 0.f;
    // clear pressed keys tracking
    pressedKeys.clear();
    committingPlacement = false;
}

void PlayGame::togglePause() {
    if (isGameOver) return;
    isPaused = !isPaused;
    pendingPauseAction = 0;

    // When pausing, record which relevant keys are currently held so we can suppress them
    if (isPaused) {
        suppressHeldInputs = true;
        suppressLeft = Keyboard::isKeyPressed(Keyboard::Key::Left);
        suppressRight = Keyboard::isKeyPressed(Keyboard::Key::Right);
        suppressShift = Keyboard::isKeyPressed(Keyboard::Key::LShift) || Keyboard::isKeyPressed(Keyboard::Key::RShift);
        suppressSpace = Keyboard::isKeyPressed(Keyboard::Key::Space);
         printf("[DBG] Paused. suppressLeft=%d suppressRight=%d suppressShift=%d suppressSpace=%d\n",
             suppressLeft ? 1 : 0, suppressRight ? 1 : 0, suppressShift ? 1 : 0, suppressSpace ? 1 : 0);
        // Also mark those keys as 'pressed' so future KeyPressed events are ignored until a release
        if (suppressLeft) pressedKeys.insert(Keyboard::Key::Left);
        if (suppressRight) pressedKeys.insert(Keyboard::Key::Right);
        if (suppressShift) { pressedKeys.insert(Keyboard::Key::LShift); pressedKeys.insert(Keyboard::Key::RShift); }
        if (suppressSpace) pressedKeys.insert(Keyboard::Key::Space);
    } else {
        // Safety lock armed when transitioning from pause menu back to active gameplay
        instantDropCooldown = 0.4f;
        // Short input lock to avoid held-key actions immediately after unpausing
        inputLockTimer = inputLockDuration;
        // keep suppression active until respective keys are released
    }
}

bool PlayGame::getPaused() const {
    return isPaused;
}

bool PlayGame::getGameOver() const {
    return isGameOver;
}

void PlayGame::enterGameOver() {
    isGameOver = true;
    pendingGameOverAction = 0;
    isPaused = false;
}

int PlayGame::consumePendingGameOverAction() {
    int action = pendingGameOverAction;
    pendingGameOverAction = 0;
    return action;
}

bool PlayGame::checkCollision() {
    for (int i = 0; i < 4; i++) {
        if (currentPiece[i].x < 0 || currentPiece[i].x >= COLS || currentPiece[i].y >= ROWS) return false;
        if (currentPiece[i].y >= 0 && grid[currentPiece[i].y][currentPiece[i].x] != 0) return false;
    }
    return true;
}

void PlayGame::calculateGhost(TetrisPoint ghost[4]) {
    for (int i = 0; i < 4; i++) ghost[i] = currentPiece[i];
    while (true) {
        for (int i = 0; i < 4; i++) ghost[i].y += 1;
        bool hitObstacle = false;
        for (int i = 0; i < 4; i++) {
            if (ghost[i].x < 0 || ghost[i].x >= COLS || ghost[i].y >= ROWS) hitObstacle = true;
            if (ghost[i].y >= 0 && grid[ghost[i].y][ghost[i].x] != 0) hitObstacle = true;
        }
        if (hitObstacle) {
            for (int i = 0; i < 4; i++) ghost[i].y -= 1;
            break;
        }
    }
}

void PlayGame::spawnPiece() {
    currentType = nextType;
    nextType = rand() % 7 + 1; 
    
    int startX = COLS / 2;
    for (int i = 0; i < 4; i++) {
        currentPiece[i].x = startX + tetrominoes[currentType - 1][i].x;
        currentPiece[i].y = tetrominoes[currentType - 1][i].y;
    }
    canHold = true; 
}

void PlayGame::handleInput(const Event& event, bool& gameOver) {
    if (isGameOver) {
        if (const auto* keyPressed = event.getIf<Event::KeyPressed>()) {
            auto code = keyPressed->code;
            if (code == Keyboard::Key::R || code == Keyboard::Key::Space) {
                pendingGameOverAction = 1;
            } else if (code == Keyboard::Key::Escape) {
                pendingGameOverAction = 2;
            }
        }
        return;
    }

    if (const auto* keyPressed = event.getIf<Event::KeyPressed>()) {
        auto code = keyPressed->code;
        if (isGameOver) {
            if (code == Keyboard::Key::R || code == Keyboard::Key::Space) {
                pendingGameOverAction = 1;
            } else if (code == Keyboard::Key::Escape) {
                pendingGameOverAction = 2;
            }
            return;
        }
        // If we're already committing a placement, ignore all new KeyPressed events
        if (committingPlacement) {
            printf("[DBG] Ignored KeyPressed code=%d because committingPlacement=true\n", static_cast<int>(code));
            return;
        }
        // If we recently unpaused, ignore non-pause keys for a short grace period
        if (inputLockTimer > 0.f && code != Keyboard::Key::P && code != Keyboard::Key::Escape) {
            printf("[DBG] Ignored KeyPressed code=%d due to inputLockTimer=%.3f\n", static_cast<int>(code), inputLockTimer);
            return;
        }
        // Ignore repeated KeyPressed events until corresponding KeyReleased clears them
        if (pressedKeys.find(code) != pressedKeys.end()) {
            printf("[DBG] Ignored repeated KeyPressed code=%d\n", static_cast<int>(code));
            return;
        }
        // mark key as pressed
        pressedKeys.insert(code);
        printf("[DBG] KeyPressed code=%d isPaused=%d suppressHeldInputs=%d instantDropCooldown=%.3f\n",
               static_cast<int>(code), isPaused ? 1 : 0, suppressHeldInputs ? 1 : 0, instantDropCooldown);

        // If we previously recorded keys as held when pausing, ignore their KeyPressed events
        if (suppressHeldInputs) {
            if ((code == Keyboard::Key::Left && suppressLeft) ||
                (code == Keyboard::Key::Right && suppressRight) ||
                ((code == Keyboard::Key::LShift || code == Keyboard::Key::RShift) && suppressShift) ||
                (code == Keyboard::Key::Space && suppressSpace)) {
                printf("[DBG] Suppressing held key press: code=%d\n", static_cast<int>(code));
                return;
            }
        }

        if (code == Keyboard::Key::P || code == Keyboard::Key::Escape) {
            togglePause();
            return;
        }

        if (isPaused) {
            if (code == Keyboard::Key::Up) {
                pauseSelection = (pauseSelection <= 1) ? 3 : pauseSelection - 1;
                return;
            } else if (code == Keyboard::Key::Down) {
                pauseSelection = (pauseSelection >= 3) ? 1 : pauseSelection + 1;
                return;
            } else if (code == Keyboard::Key::Space) {
                pendingPauseAction = pauseSelection; 
                return;
            }
        }
    }

    if (isPaused) return;

    // RESTRICTION: respect per-action cooldowns
    if (hardDropCooldown > 0.f) return;

    if (const auto* keyPressed = event.getIf<Event::KeyPressed>()) {
        int dx = 0;
        bool shouldRotate = false;
        if ((keyPressed->code == Keyboard::Key::Left) && moveCooldown <= 0.f) dx = -1;
        else if ((keyPressed->code == Keyboard::Key::Right) && moveCooldown <= 0.f) dx = 1;
        else if (keyPressed->code == Keyboard::Key::Down) fallDelay = 0.05f; 
        else if ((keyPressed->code == Keyboard::Key::Up) && rotateCooldown <= 0.f) shouldRotate = true;
        else if (keyPressed->code == Keyboard::Key::Space) {
            // Prevent re-entrancy while we commit the placement
            committingPlacement = true;
            // Hard-drop: compute ghost then ensure target cells are free before committing
            TetrisPoint ghostPiece[4];
            calculateGhost(ghostPiece);

            // If any ghost cell would overlap existing blocks, signal game over instead of overwriting
            for (int i = 0; i < 4; i++) {
                if (ghostPiece[i].y >= 0 && grid[ghostPiece[i].y][ghostPiece[i].x] != 0) {
                    gameOver = true;
                    committingPlacement = false;
                    return;
                }
            }

            for (int i = 0; i < 4; i++) currentPiece[i] = ghostPiece[i];
            for (int i = 0; i < 4; i++) {
                if (currentPiece[i].y >= 0) grid[currentPiece[i].y][currentPiece[i].x] = currentType;
            }
            clearLines();
            spawnPiece();
            lockTimer = 0.f;
            timer = 0.f;

            // BUG FIX: Check immediately if the fresh piece spawned inside blocked geometry
            if (!checkCollision()) {
                gameOver = true;
                committingPlacement = false;
                return;
            }
            // Protect against immediate subsequent hard-drops
            hardDropCooldown = hardDropDelay;
            committingPlacement = false;
            return;
        }
        else if (keyPressed->code == Keyboard::Key::LShift || keyPressed->code == Keyboard::Key::RShift) {
            if (canHold) {
                committingPlacement = true;
                if (holdType == 0) {
                    holdType = currentType;
                    spawnPiece();
                    // After spawning, if any of the new piece overlaps existing grid, it's a top-out
                    for (int i = 0; i < 4; i++) {
                        if (currentPiece[i].y >= 0 && grid[currentPiece[i].y][currentPiece[i].x] != 0) {
                            gameOver = true;
                            committingPlacement = false;
                            return;
                        }
                    }
                } else {
                    int temp = currentType;
                    currentType = holdType;
                    holdType = temp;
                    int startX = COLS / 2;
                    for (int i = 0; i < 4; i++) {
                        currentPiece[i].x = startX + tetrominoes[currentType - 1][i].x;
                        currentPiece[i].y = tetrominoes[currentType - 1][i].y;
                    }
                    // verify swapped-in piece doesn't immediately overlap
                    for (int i = 0; i < 4; i++) {
                        if (currentPiece[i].y >= 0 && grid[currentPiece[i].y][currentPiece[i].x] != 0) {
                            gameOver = true;
                            committingPlacement = false;
                            return;
                        }
                    }
                }
                canHold = false; 
                lockTimer = 0.f;

                // Double safety layout to verify edge hold-swap configurations
                if (!checkCollision()) {
                    gameOver = true;
                    committingPlacement = false;
                }
                // Prevent immediate input spam (hold -> avoid instant further holds/drops)
                holdCooldown = holdDelay;
                committingPlacement = false;
            }
        }

        if (dx != 0) {
            for (int i = 0; i < 4; i++) { backupPiece[i] = currentPiece[i]; currentPiece[i].x += dx; }
            if (!checkCollision()) { for (int i = 0; i < 4; i++) currentPiece[i] = backupPiece[i]; }
            else { lockTimer = 0.f; }
            // apply move cooldown
            moveCooldown = moveDelay;
        }

        if (shouldRotate && currentType != 2) { 
            TetrisPoint pivot = currentPiece[1];
            for (int i = 0; i < 4; i++) {
                backupPiece[i] = currentPiece[i];
                int relativeX = currentPiece[i].x - pivot.x;
                int relativeY = currentPiece[i].y - pivot.y;
                currentPiece[i].x = pivot.x - relativeY;
                currentPiece[i].y = pivot.y + relativeX;
            }
            if (!checkCollision()) { for (int i = 0; i < 4; i++) currentPiece[i] = backupPiece[i]; }
            else { lockTimer = 0.f; }
            rotateCooldown = rotateDelay;
        }
    }
    // Note: KeyReleased events are handled via notifyKeyReleased to avoid forwarding issues
}

void PlayGame::handlePauseKey(sf::Keyboard::Key key) {
    if (isGameOver) return;
    if (!isPaused) return;
    if (key == Keyboard::Key::Up) {
        pauseSelection = (pauseSelection <= 1) ? 3 : pauseSelection - 1;
    } else if (key == Keyboard::Key::Down) {
        pauseSelection = (pauseSelection >= 3) ? 1 : pauseSelection + 1;
    } else if (key == Keyboard::Key::Space) {
        pendingPauseAction = pauseSelection;
    }
}

void PlayGame::notifyKeyReleased(sf::Keyboard::Key key) {
    // Called by main for KeyReleased events so suppression flags clear predictably
    if (key == Keyboard::Key::Down) {
        fallDelay = 0.4f;
    }

    // Remove from pressed keys set so subsequent presses register
    if (pressedKeys.find(key) != pressedKeys.end()) {
        pressedKeys.erase(key);
        printf("[DBG] KeyReleased cleared pressedKeys for code=%d\n", static_cast<int>(key));
    }

    if (suppressHeldInputs) {
        if (key == Keyboard::Key::Left) suppressLeft = false;
        if (key == Keyboard::Key::Right) suppressRight = false;
        if (key == Keyboard::Key::LShift || key == Keyboard::Key::RShift) suppressShift = false;
        if (key == Keyboard::Key::Space) suppressSpace = false;
        if (!suppressLeft && !suppressRight && !suppressShift && !suppressSpace) {
            suppressHeldInputs = false;
            printf("[DBG] Cleared suppression flags after releases.\n");
        }
    }
}

void PlayGame::clearLines() {
    int linesClearedThisTurn = 0;
    for (int r = ROWS - 1; r >= 0; r--) {
        bool lineIsFull = true;
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c] == 0) lineIsFull = false;
        }
        if (lineIsFull) {
            linesClearedThisTurn++;
            for (int moveRow = r; moveRow > 0; moveRow--) {
                for (int c = 0; c < COLS; c++) grid[moveRow][c] = grid[moveRow - 1][c];
            }
            for (int c = 0; c < COLS; c++) grid[0][c] = 0;
            r++; 
        }
    }

    if (linesClearedThisTurn > 0) {
        combo++; 
        int currentMultiplier = min(combo, 5); 
        score += linesClearedThisTurn * 100 * currentMultiplier;
        scoreText.setString("SCORE\n" + to_string(score));
        comboText.setString("COMBO\nx" + to_string(currentMultiplier));
        comboText.setFillColor(Color::Yellow);
    } else {
        combo = 0; 
        comboText.setString("COMBO\nx1");
        comboText.setFillColor(Color(150, 150, 150)); 
    }
}

void PlayGame::update(float elapsedTime, bool& gameOver) {
    if (isPaused || isGameOver) return;

    // Progressively countdown input lock delay tracking values
    if (instantDropCooldown > 0.f) {
        instantDropCooldown -= elapsedTime;
    }
    if (inputLockTimer > 0.f) inputLockTimer -= elapsedTime;
    if (moveCooldown > 0.f) moveCooldown -= elapsedTime;
    if (rotateCooldown > 0.f) rotateCooldown -= elapsedTime;
    if (holdCooldown > 0.f) holdCooldown -= elapsedTime;
    if (hardDropCooldown > 0.f) hardDropCooldown -= elapsedTime;

    bool onGround = false;
    for (int i = 0; i < 4; i++) currentPiece[i].y += 1;
    if (!checkCollision()) onGround = true;
    for (int i = 0; i < 4; i++) currentPiece[i].y -= 1;

    if (!onGround) {
        lockTimer = 0.f;
        timer += elapsedTime;
        if (timer > fallDelay) {
            for (int i = 0; i < 4; i++) currentPiece[i].y += 1;
            timer = 0.f;
        }
    } else {
        lockTimer += elapsedTime;
        if (lockTimer >= maxLockDelay) {
            // Atomically commit placement to avoid overlap from simultaneous inputs
            committingPlacement = true;
            for (int i = 0; i < 4; i++) {
                if (currentPiece[i].y >= 0) grid[currentPiece[i].y][currentPiece[i].x] = currentType;
            }
            clearLines();
            spawnPiece();
            lockTimer = 0.f;
            timer = 0.f;
            if (!checkCollision()) {
                gameOver = true;
                enterGameOver();
            }
            committingPlacement = false;
        }
    }
}

void PlayGame::draw(RenderWindow& window) {
    window.draw(boardFrame);
    window.draw(scoreText);
    window.draw(comboText);
    
    window.draw(holdFrame);
    window.draw(holdTitle);
    window.draw(nextFrame);
    window.draw(nextTitle);

    TetrisPoint ghostPiece[4];
    calculateGhost(ghostPiece);
    Color shadowColor = blockColors[currentType];
    shadowColor.a = 50; 
    
    blockSprite.setFillColor(shadowColor);
    for (int i = 0; i < 4; i++) {
        if (ghostPiece[i].y >= 0) {
            blockSprite.setPosition(Vector2f(BOARD_OFFSET.x + ghostPiece[i].x * TILE_SIZE + 1.f, BOARD_OFFSET.y + ghostPiece[i].y * TILE_SIZE + 1.f));
            window.draw(blockSprite);
        }
    }

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c] != 0) {
                blockSprite.setFillColor(blockColors[grid[r][c]]);
                blockSprite.setPosition(Vector2f(BOARD_OFFSET.x + c * TILE_SIZE + 1.f, BOARD_OFFSET.y + r * TILE_SIZE + 1.f));
                window.draw(blockSprite);
            }
        }
    }

    blockSprite.setFillColor(blockColors[currentType]);
    for (int i = 0; i < 4; i++) {
        if (currentPiece[i].y >= 0) {
            blockSprite.setPosition(Vector2f(BOARD_OFFSET.x + currentPiece[i].x * TILE_SIZE + 1.f, BOARD_OFFSET.y + currentPiece[i].y * TILE_SIZE + 1.f));
            window.draw(blockSprite);
        }
    }

    if (holdType != 0) {
        previewBlock.setFillColor(blockColors[holdType]);
        Vector2f holdCenter = holdFrame.getPosition() + Vector2f(50.f, 45.f);
        for (int i = 0; i < 4; i++) {
            float xPos = holdCenter.x + (tetrominoes[holdType - 1][i].x * 20.f) - 10.f;
            float yPos = holdCenter.y + (tetrominoes[holdType - 1][i].y * 20.f) - 10.f;
            previewBlock.setPosition(Vector2f(xPos, yPos));
            window.draw(previewBlock);
        }
    }

    previewBlock.setFillColor(blockColors[nextType]);
    Vector2f nextCenter = nextFrame.getPosition() + Vector2f(50.f, 45.f);
    for (int i = 0; i < 4; i++) {
        float xPos = nextCenter.x + (tetrominoes[nextType - 1][i].x * 20.f) - 10.f;
        float yPos = nextCenter.y + (tetrominoes[nextType - 1][i].y * 20.f) - 10.f;
        previewBlock.setPosition(Vector2f(xPos, yPos));
        window.draw(previewBlock);
    }

    if (isPaused) {
        RectangleShape dimLayer(Vector2f(COLS * TILE_SIZE, ROWS * TILE_SIZE));
        dimLayer.setPosition(BOARD_OFFSET);
        dimLayer.setFillColor(Color(0, 0, 0, 180)); 
        window.draw(dimLayer);
        resumeText.setFillColor(pauseSelection == 1 ? Color::Yellow : Color::White);
        settingsText.setFillColor(pauseSelection == 2 ? Color::Yellow : Color::White);
        quitText.setFillColor(pauseSelection == 3 ? Color::Yellow : Color::White);
        window.draw(resumeText);
        window.draw(settingsText);
        window.draw(quitText);
    }

    if (isGameOver) {
        if (gameOverImageLoaded && gameOverSprite) {
            const auto winSize = window.getSize();
            const auto texSize = gameOverTexture.getSize();
            float scale = max(winSize.x / static_cast<float>(texSize.x), winSize.y / static_cast<float>(texSize.y));
            gameOverSprite->setScale(Vector2f(scale, scale));
            gameOverSprite->setOrigin(Vector2f(texSize.x / 2.f, texSize.y / 2.f));
            gameOverSprite->setPosition(Vector2f(winSize.x / 2.f, winSize.y / 2.f));
            window.draw(*gameOverSprite);
        } else {
            RectangleShape dimLayer(Vector2f(COLS * TILE_SIZE, ROWS * TILE_SIZE));
            dimLayer.setPosition(BOARD_OFFSET);
            dimLayer.setFillColor(Color(0, 0, 0, 200));
            window.draw(dimLayer);
        }
    }
}

int PlayGame::pauseMenuActionAt(const sf::Vector2f& mousePos) const {
    if (!isPaused) return 0;
    if (resumeText.getGlobalBounds().contains(mousePos)) return 1;
    if (settingsText.getGlobalBounds().contains(mousePos)) return 2;
    if (quitText.getGlobalBounds().contains(mousePos)) return 3;
    return 0;
}

int PlayGame::consumePendingPauseAction() {
    int a = pendingPauseAction;
    pendingPauseAction = 0;
    return a;
}



// Look inside your main.cpp event loop for this line and update it:
// if (currentState == Gamestate::Playing) {
//     tetrisGame.handleInput(*event, gameOverSignal); // <-- Pass your game-over variable name here
// }
