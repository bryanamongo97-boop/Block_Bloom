#pragma once
#include <SFML/Graphics.hpp>
#include <set>
#include <memory>

const int ROWS = 20;
const int COLS = 10;
const int TILE_SIZE = 30; 
const sf::Vector2f BOARD_OFFSET(150.f, 50.f); 

struct TetrisPoint {
    int x, y;
};

class PlayGame {
public:
    PlayGame(const sf::Font& font);
    
    void reset();
    // Keep the game-over signal in update/handle signatures so callers can respond immediately
    void handleInput(const sf::Event& event, bool& gameOver);
    void update(float elapsedTime, bool& gameOver);
    void draw(sf::RenderWindow& window);
    
    void togglePause();
    bool getPaused() const;
    bool getGameOver() const;
    void enterGameOver();
    int pauseMenuActionAt(const sf::Vector2f& mousePos) const;
    void handlePauseKey(sf::Keyboard::Key key);
    int consumePendingPauseAction();
    int consumePendingGameOverAction();
    void notifyKeyReleased(sf::Keyboard::Key key);

private:
    bool checkCollision();
    void spawnPiece();
    void clearLines();
    void calculateGhost(TetrisPoint ghost[4]); 

    // Core Grid Matrix Engines
    int grid[ROWS][COLS];
    TetrisPoint currentPiece[4];
    TetrisPoint backupPiece[4];
    
    int currentType;
    int nextType;    
    int holdType;    
    bool canHold;    
    
    int score;
    int combo;       
    float timer;
    float fallDelay;
    
    // Lock Delay Clock Trackers
    float lockTimer;
    const float maxLockDelay = 1.0f; // 1.0 second grace period for ground adjustments
    
    // Instant drop cooldown + input lock
    float instantDropCooldown;
    const float instantDropDelay = 0.05f; // 0.05 second delay between instant drops (faster)
    float inputLockTimer;
    const float inputLockDuration = 0.25f;

    const sf::Font& fontRef;

    // SFML Draw Components
    sf::RectangleShape boardFrame;
    sf::RectangleShape blockSprite;
    sf::RectangleShape previewBlock; 
    sf::Text scoreText;
    sf::Text comboText; 

    // UI Hold & Next Containers
    sf::RectangleShape holdFrame;
    sf::RectangleShape nextFrame;
    sf::Text holdTitle;
    sf::Text nextTitle;

    // Game Over image
    sf::Texture gameOverTexture;
    std::unique_ptr<sf::Sprite> gameOverSprite;
    bool gameOverImageLoaded;

    // Pause Graphic Overlays
    bool isPaused;
    bool isGameOver;
    sf::Text resumeText;
    sf::Text settingsText;
    sf::Text quitText;
    sf::Text gameOverText;
    sf::Text gameOverRestartText;
    sf::Text gameOverMenuText;
    int pauseSelection; // 1=Resume,2=Settings,3=Quit
    int pendingPauseAction; // set when player confirms via keyboard
    int pendingGameOverAction; // 1=Restart,2=Menu
    // Suppress held inputs that were active when pausing to avoid accidental actions on unpause
    bool suppressHeldInputs;
    bool suppressLeft;
    bool suppressRight;
    bool suppressShift;
    bool suppressSpace;
    // Per-action cooldowns to debounce rapid inputs
    float moveCooldown;
    float rotateCooldown;
    float holdCooldown;
    float hardDropCooldown;
    const float moveDelay = 0.06f;
    const float rotateDelay = 0.12f;
    const float holdDelay = 0.4f;
    const float hardDropDelay = 0.1f;

    // Track currently pressed keys to ignore repeated KeyPressed events until release
    std::set<sf::Keyboard::Key> pressedKeys;
    // Prevent re-entrant placement operations (hard-drop / lock / hold) from overlapping
    bool committingPlacement;
};
