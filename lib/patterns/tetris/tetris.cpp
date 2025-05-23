#include "tetris.h"
#include <led_display.h>
#include <FastLED.h>

// Game constants
#define GRID_WIDTH 16
#define GRID_HEIGHT 16
#define TETROMINO_SIZE 4
#define INITIAL_SPEED 800    // Initial drop speed in milliseconds
#define MIN_SPEED 100       // Maximum speed (minimum delay)
#define SPEED_INCREASE 50   // How much to decrease delay after each level
#define LINES_PER_LEVEL 10  // Number of lines needed to increase level

// Tetromino definitions (4x4 grids)
const uint8_t TETROMINOS[7][4][4] = {
    // I-piece
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    // O-piece
    {
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // T-piece
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // S-piece
    {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // Z-piece
    {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // J-piece
    {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // L-piece
    {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

// Tetromino colors (classic NES Tetris colors)
const CRGB TETROMINO_COLORS[7] = {
    CRGB(0, 240, 240),   // I-piece - Cyan
    CRGB(240, 240, 0),   // O-piece - Yellow
    CRGB(160, 0, 240),   // T-piece - Purple
    CRGB(0, 240, 0),     // S-piece - Green
    CRGB(240, 0, 0),     // Z-piece - Red
    CRGB(0, 0, 240),     // J-piece - Blue
    CRGB(240, 160, 0)    // L-piece - Orange
};

// Game variables
static uint8_t gameBoard[GRID_HEIGHT][GRID_WIDTH] = {0};  // 0 = empty, 1-7 = tetromino type
static uint8_t currentPiece[TETROMINO_SIZE][TETROMINO_SIZE];
static uint8_t currentType;
static int currentX, currentY;
static Rotation currentRotation;
static TetrisGameState gameState = WAITING;
static unsigned long lastMoveTime;
static unsigned long dropInterval;
static int score = 0;
static int level = 1;
static int linesCleared = 0;
static bool g_tetrisInitialized = false;
static bool aiMode = false;

// Backup variables for simulation
static uint8_t backupPiece[TETROMINO_SIZE][TETROMINO_SIZE];
static int backupX, backupY;
static Rotation backupRotation;

// Initialize game
void initTetrisGame() {
    // Clear the game board
    memset(gameBoard, 0, sizeof(gameBoard));
    
    // Reset game variables
    dropInterval = INITIAL_SPEED;
    score = 0;
    level = 1;
    linesCleared = 0;
    gameState = PLAYING;
    
    // Spawn first piece
    spawnTetromino();
    
    lastMoveTime = millis();
    Serial.println("Tetris game initialized!");
}

// Spawn a new tetromino at the top of the board
void spawnTetromino() {
    // Choose random piece
    currentType = random(7);
    
    // Copy piece data
    memcpy(currentPiece, TETROMINOS[currentType], sizeof(currentPiece));
    
    // Set initial position (centered at top)
    currentX = (GRID_WIDTH - TETROMINO_SIZE) / 2;
    currentY = 0;
    currentRotation = ROT_0;
    
    // Check if piece can be placed - if not, game over
    if (checkCollision()) {
        gameState = GAME_OVER;
        Serial.println("Game Over!");
    }
}

// Check if current piece collides with board boundaries or other pieces
bool checkCollision() {
    for (int y = 0; y < TETROMINO_SIZE; y++) {
        for (int x = 0; x < TETROMINO_SIZE; x++) {
            if (currentPiece[y][x]) {
                int boardX = currentX + x;
                int boardY = currentY + y;
                
                // Check boundaries
                if (boardX < 0 || boardX >= GRID_WIDTH || 
                    boardY < 0 || boardY >= GRID_HEIGHT) {
                    return true;
                }
                
                // Check collision with locked pieces
                if (gameBoard[boardY][boardX]) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Rotate the current piece
void rotateTetromino() {
    uint8_t temp[TETROMINO_SIZE][TETROMINO_SIZE];
    
    // Copy current piece
    memcpy(temp, currentPiece, sizeof(temp));
    
    // Rotate 90 degrees clockwise
    for (int y = 0; y < TETROMINO_SIZE; y++) {
        for (int x = 0; x < TETROMINO_SIZE; x++) {
            currentPiece[x][TETROMINO_SIZE-1-y] = temp[y][x];
        }
    }
    
    // If rotation causes collision, revert
    if (checkCollision()) {
        memcpy(currentPiece, temp, sizeof(currentPiece));
    } else {
        currentRotation = (Rotation)((currentRotation + 1) % 4);
    }
}

// Move the current piece
void moveTetromino(TetrisDirection dir) {
    switch (dir) {
        case T_LEFT:
            currentX--;
            if (checkCollision()) currentX++;
            break;
            
        case T_RIGHT:
            currentX++;
            if (checkCollision()) currentX--;
            break;
            
        case T_DOWN:
            currentY++;
            if (checkCollision()) {
                currentY--;
                lockTetromino();
            }
            break;
            
        case T_ROTATE:
            rotateTetromino();
            break;
    }
}

// Lock the current piece in place
void lockTetromino() {
    // Copy piece to board
    for (int y = 0; y < TETROMINO_SIZE; y++) {
        for (int x = 0; x < TETROMINO_SIZE; x++) {
            if (currentPiece[y][x]) {
                gameBoard[currentY + y][currentX + x] = currentType + 1;
            }
        }
    }
    
    // Clear any completed lines
    clearLines();
    
    // Spawn new piece
    spawnTetromino();
}

// Clear completed lines and update score
void clearLines() {
    int linesCleared = 0;
    
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        bool lineComplete = true;
        
        // Check if line is complete
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!gameBoard[y][x]) {
                lineComplete = false;
                break;
            }
        }
        
        if (lineComplete) {
            linesCleared++;
            
            // Move all lines above down
            for (int moveY = y; moveY > 0; moveY--) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    gameBoard[moveY][x] = gameBoard[moveY-1][x];
                }
            }
            
            // Clear top line
            for (int x = 0; x < GRID_WIDTH; x++) {
                gameBoard[0][x] = 0;
            }
            
            y++; // Check the same line again as everything moved down
        }
    }
    
    // Update score and level
    if (linesCleared > 0) {
        score += linesCleared * linesCleared * 100 * level;
        linesCleared += linesCleared;
        
        // Check for level up
        level = (linesCleared / LINES_PER_LEVEL) + 1;
        
        // Update speed
        dropInterval = max(MIN_SPEED, INITIAL_SPEED - (level - 1) * SPEED_INCREASE);
        
        Serial.printf("Lines cleared: %d, Score: %d, Level: %d\n", 
                     linesCleared, score, level);
    }
}

// AI helper functions
void backupPosition() {
    memcpy(backupPiece, currentPiece, sizeof(backupPiece));
    backupX = currentX;
    backupY = currentY;
    backupRotation = currentRotation;
}

void restorePosition() {
    memcpy(currentPiece, backupPiece, sizeof(currentPiece));
    currentX = backupX;
    currentY = backupY;
    currentRotation = backupRotation;
}

void simulateMove(TetrisDirection dir, bool& collision) {
    backupPosition();
    
    switch (dir) {
        case T_LEFT:
            currentX--;
            collision = checkCollision();
            if (collision) currentX++;
            break;
        case T_RIGHT:
            currentX++;
            collision = checkCollision();
            if (collision) currentX--;
            break;
        case T_DOWN:
            currentY++;
            collision = checkCollision();
            if (collision) currentY--;
            break;
        case T_ROTATE:
            rotateTetromino();
            collision = false;
            break;
    }
}

float evaluatePosition() {
    float score = 0;
    int heights[GRID_WIDTH] = {0};
    int holes = 0;
    int completeLines = 0;
    int maxHeight = 0;
    int totalHeight = 0;
    int blockedHoles = 0;
    int wellDepths[GRID_WIDTH] = {0};
    float bumpiness = 0;
    
    // Calculate column heights, holes, and wells
    for (int x = 0; x < GRID_WIDTH; x++) {
        bool foundBlock = false;
        int columnHoles = 0;
        int wellDepth = 0;
        
        for (int y = 0; y < GRID_HEIGHT; y++) {
            bool isFilled = gameBoard[y][x] || 
                (currentY + TETROMINO_SIZE > y && 
                 currentX + TETROMINO_SIZE > x && 
                 currentX <= x && 
                 currentPiece[y - currentY][x - currentX]);
                 
            if (!foundBlock && isFilled) {
                heights[x] = GRID_HEIGHT - y;
                maxHeight = std::max(maxHeight, heights[x]);
                totalHeight += heights[x];
                foundBlock = true;
            }
            
            if (foundBlock && !isFilled) {
                holes++;
                columnHoles++;
                wellDepth++;
            } else if (isFilled) {
                if (wellDepth > 0) {
                    wellDepths[x] = std::max(wellDepths[x], wellDepth);
                }
                wellDepth = 0;
            }
        }
        
        if (columnHoles > 1) {
            holes += columnHoles * 4;
        }
        
        if (x > 0) {
            float diff = std::abs(heights[x] - heights[x-1]);
            bumpiness += diff;
        }
    }
    
    // Count complete lines
    for (int y = 0; y < GRID_HEIGHT; y++) {
        bool complete = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!gameBoard[y][x] && 
                !(currentY + TETROMINO_SIZE > y && 
                  currentX + TETROMINO_SIZE > x && 
                  currentX <= x && 
                  currentPiece[y - currentY][x - currentX])) {
                complete = false;
                break;
            }
        }
        if (complete) completeLines++;
    }
    
    float aggregateHeight = 0;
    for (int x = 0; x < GRID_WIDTH; x++) {
        aggregateHeight += heights[x];
    }
    
    float wellPenalty = 0;
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (wellDepths[x] > 2) {
            wellPenalty += wellDepths[x] * wellDepths[x];
        }
    }
    
    float pieceBonus = 0;
    switch (currentType) {
        case 0:
            if (completeLines == 4) {
                pieceBonus += 2000.0f;
            }
            if (heights[GRID_WIDTH-1] <= maxHeight - 4) {
                pieceBonus += 100.0f;
            }
            break;
            
        case 1:
            if (currentY > GRID_HEIGHT - 4) {
                pieceBonus += 50.0f;
            }
            break;
            
        case 2:
            if (maxHeight < GRID_HEIGHT - 3 && bumpiness < 3) {
                pieceBonus += 80.0f;
            }
            break;
            
        default:
            if (bumpiness < 2 && holes == 0) {
                pieceBonus += 60.0f;
            }
    }
    
    score = 
        completeLines * 1000.0f +
        pieceBonus +
        -holes * 100.0f +
        -blockedHoles * 50.0f +
        -bumpiness * 40.0f +
        -aggregateHeight * 20.0f +
        -maxHeight * 30.0f +
        -wellPenalty * 40.0f;
    
    if (bumpiness < 3) {
        score += 100.0f;
    }
    
    if (maxHeight < GRID_HEIGHT/2) {
        score += 200.0f;
    }
    
    for (int x = 1; x < GRID_WIDTH-1; x++) {
        if (heights[x] > heights[x-1] + 2 && heights[x] > heights[x+1] + 2) {
            score -= 150.0f;
        }
    }
    
    return score;
}

TetrisDirection getTetrisAIMove() {
    float bestScore = -999999;
    TetrisDirection bestMove = T_DOWN;
    
    // Look ahead simulation
    for (int rotations = 0; rotations < 4; rotations++) {
        backupPosition();
        
        for (int i = 0; i < rotations; i++) {
            rotateTetromino();
        }
        
        for (int moveX = -TETROMINO_SIZE; moveX <= GRID_WIDTH; moveX++) {
            backupPosition();
            
            while (currentX < moveX) {
                currentX++;
                if (checkCollision()) {
                    currentX--;
                    break;
                }
            }
            while (currentX > moveX) {
                currentX--;
                if (checkCollision()) {
                    currentX++;
                    break;
                }
            }
            
            if (!checkCollision()) {
                while (!checkCollision()) {
                    currentY++;
                }
                currentY--;
                
                float score = evaluatePosition();
                
                float centerPreference = -abs(currentX - GRID_WIDTH/2) * 0.1f;
                score += centerPreference;
                
                bool hasOverhang = false;
                bool createsGap = false;
                
                for (int x = 0; x < TETROMINO_SIZE; x++) {
                    bool hasBlock = false;
                    for (int y = TETROMINO_SIZE-1; y >= 0; y--) {
                        if (currentPiece[y][x]) {
                            hasBlock = true;
                        } else if (hasBlock) {
                            hasOverhang = true;
                            break;
                        }
                    }
                }
                
                for (int x = 0; x < TETROMINO_SIZE; x++) {
                    for (int y = 0; y < TETROMINO_SIZE; y++) {
                        if (currentPiece[y][x] && currentY + y < GRID_HEIGHT - 1) {
                            if (!gameBoard[currentY + y + 1][currentX + x]) {
                                createsGap = true;
                                break;
                            }
                        }
                    }
                    if (createsGap) break;
                }
                
                if (hasOverhang) score -= 50.0f;
                if (createsGap) score -= 100.0f;
                
                if (score > bestScore) {
                    bestScore = score;
                    if (rotations > 0) {
                        bestMove = T_ROTATE;
                    } else if (moveX < backupX) {
                        bestMove = T_LEFT;
                    } else if (moveX > backupX) {
                        bestMove = T_RIGHT;
                    } else {
                        bestMove = T_DOWN;
                    }
                }
            }
            
            restorePosition();
        }
        
        restorePosition();
    }
    
    return bestMove;
}

void toggleAIMode() {
    aiMode = !aiMode;
    Serial.printf("AI mode %s\n", aiMode ? "enabled" : "disabled");
}

// Update game state
void updateTetrisGame() {
    if (gameState != PLAYING) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // If AI mode is active, make moves more frequently
    if (aiMode && currentTime - lastMoveTime >= dropInterval / 4) {
        TetrisDirection aiMove = getTetrisAIMove();
        
        // Make the move
        switch (aiMove) {
            case T_LEFT:
                currentX--;
                if (checkCollision()) currentX++;
                break;
            case T_RIGHT:
                currentX++;
                if (checkCollision()) currentX--;
                break;
            case T_ROTATE:
                rotateTetromino();
                break;
            case T_DOWN:
                currentY++;
                if (checkCollision()) {
                    currentY--;
                    lockTetromino();
                }
                break;
        }
        
        // Always try to move down after any move
        if (aiMove != T_DOWN) {
            currentY++;
            if (checkCollision()) {
                currentY--;
                lockTetromino();
            }
        }
        
        lastMoveTime = currentTime;
        return;
    }
    
    // Normal game update
    if (currentTime - lastMoveTime >= dropInterval) {
        moveTetromino(T_DOWN);
        lastMoveTime = currentTime;
    }
}

// Render the game
void renderTetrisGame(CRGB* leds) {
    // Clear display
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    // Draw board
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (gameBoard[y][x]) {
                leds[XY(x, y)] = TETROMINO_COLORS[gameBoard[y][x] - 1];
            }
        }
    }
    
    // Draw current piece
    if (gameState == PLAYING) {
        for (int y = 0; y < TETROMINO_SIZE; y++) {
            for (int x = 0; x < TETROMINO_SIZE; x++) {
                if (currentPiece[y][x]) {
                    int boardX = currentX + x;
                    int boardY = currentY + y;
                    if (boardY >= 0) { // Only draw if on screen
                        leds[XY(boardX, boardY)] = TETROMINO_COLORS[currentType];
                    }
                }
            }
        }
    }
    
    // Flash display if game over
    if (gameState == GAME_OVER) {
        if ((millis() / 500) % 2 == 0) {
            for (int i = 0; i < NUM_LEDS; i++) {
                if (leds[i]) {
                    leds[i] = CRGB::Red;
                }
            }
        }
    }
}

// Main pattern function
void tetris(CRGB* leds) {
    if (!g_tetrisInitialized) {
        initTetrisGame();
        g_tetrisInitialized = true;
        Serial.println("Tetris game started");
    }
    
    updateTetrisGame();
    renderTetrisGame(leds);
}

// Web server setup function
void setupTetrisPattern(AsyncWebServer* server) {
    // Serve the tetris game control page
    server->on("/tetris", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>PixelBoard Tetris</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="header">
        <div class="header-left">
            <h1>PixelBoard Tetris</h1>
            <div class="score">Score: <span id="scoreValue">0</span></div>
            <div class="level">Level: <span id="levelValue">1</span></div>
        </div>
        <div class="header-right">
            <button class="d-btn" id="btnStart">Start Game</button>
            <button class="d-btn" id="btnPause">Pause</button>
            <button class="d-btn" id="btnRestart">Restart</button>
            <button class="d-btn" id="btnAI">Enable AI</button>
        </div>
    </div>
    
    <div class="preview-container">
        <div class="preview-grid" id="previewGrid"></div>
    </div>
    
    <div class="controls">
        <div class="d-pad">
            <button class="d-btn up" id="btnRotate"><span class="key-icon">&uarr;</span></button>
            <button class="d-btn left" id="btnLeft"><span class="key-icon">&larr;</span></button>
            <div class="center"></div>
            <button class="d-btn right" id="btnRight"><span class="key-icon">&rarr;</span></button>
            <button class="d-btn down" id="btnDown"><span class="key-icon">&darr;</span></button>
        </div>
    </div>

    <div style="text-align: center; margin: 5px; font-size: 0.8rem; color: #aaa;">
        Keyboard: Arrow keys to move, Up to rotate
    </div>

    <script>
        let previewUpdateInterval;
        let gameState = 'waiting';
        let aiMode = false;
        
        // Create preview grid
        function createPreviewGrid() {
            const grid = document.getElementById('previewGrid');
            for (let i = 0; i < 256; i++) {
                const pixel = document.createElement('div');
                pixel.className = 'preview-pixel';
                pixel.id = 'pixel-' + i;
                grid.appendChild(pixel);
            }
        }
        
        // Update the preview grid
        function updatePreview() {
            fetch('/pixelStatus')
                .then(response => response.arrayBuffer())
                .then(buffer => {
                    const pixels = new Uint8Array(buffer);
                    for (let i = 0; i < 256; i++) {
                        const baseIndex = i * 3;
                        const r = pixels[baseIndex];
                        const g = pixels[baseIndex + 1];
                        const b = pixels[baseIndex + 2];
                        
                        const pixelElement = document.getElementById('pixel-' + i);
                        if (pixelElement) {
                            pixelElement.style.backgroundColor = `rgb(${r},${g},${b})`;
                        }
                    }
                })
                .catch(error => console.error('Error updating preview:', error));
                
            // Update game state
            fetchGameState();
        }
        
        // Fetch game state
        function fetchGameState() {
            fetch('/tetrisState')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('scoreValue').textContent = data.score;
                    document.getElementById('levelValue').textContent = data.level;
                    gameState = data.state;
                    
                    // Update pause button text
                    const pauseBtn = document.getElementById('btnPause');
                    pauseBtn.textContent = gameState === 'paused' ? 'Resume' : 'Pause';
                })
                .catch(error => console.error('Error fetching game state:', error));
        }
        
        // Control functions
        function sendControl(action) {
            fetch(`/tetrisControl?action=${action}`)
                .then(response => response.text())
                .catch(error => console.error('Error sending control:', error));
        }
        
        // Toggle AI mode
        function toggleAIMode() {
            aiMode = !aiMode;
            currentDirection = '';  // Reset direction when toggling AI
            updateAIButton();
            
            fetch(`/tetrisControl?action=${aiMode ? 'aiOn' : 'aiOff'}`)
                .then(response => response.text())
                .catch(error => console.error('Error toggling AI mode:', error));
        }
        
        // Update AI button appearance
        function updateAIButton() {
            const aiButton = document.getElementById('btnAI');
            if (aiMode) {
                aiButton.textContent = 'Disable AI';
                aiButton.classList.add('active');
                // When AI mode is enabled, disable manual controls
                document.querySelectorAll('.d-btn:not(#btnAI):not(#btnStart):not(#btnRestart)').forEach(btn => {
                    btn.disabled = true;
                    btn.style.opacity = 0.5;
                });
            } else {
                aiButton.textContent = 'Enable AI';
                aiButton.classList.remove('active');
                // Re-enable manual controls
                document.querySelectorAll('.d-btn').forEach(btn => {
                    btn.disabled = false;
                    btn.style.opacity = 1;
                });
            }
        }
        
        // Add event listeners
        document.getElementById('btnStart').addEventListener('click', () => sendControl('start'));
        document.getElementById('btnPause').addEventListener('click', () => sendControl('pause'));
        document.getElementById('btnRestart').addEventListener('click', () => sendControl('restart'));
        document.getElementById('btnLeft').addEventListener('click', () => sendControl('left'));
        document.getElementById('btnRight').addEventListener('click', () => sendControl('right'));
        document.getElementById('btnDown').addEventListener('click', () => sendControl('down'));
        document.getElementById('btnRotate').addEventListener('click', () => sendControl('rotate'));
        document.getElementById('btnAI').addEventListener('click', toggleAIMode);
        
        // Add keyboard controls
        document.addEventListener('keydown', function(e) {
            if (!aiMode && gameState === 'playing') {
                switch(e.key) {
                    case 'ArrowLeft':
                        sendControl('left');
                        break;
                    case 'ArrowRight':
                        sendControl('right');
                        break;
                    case 'ArrowDown':
                        sendControl('down');
                        break;
                    case 'ArrowUp':
                        sendControl('rotate');
                        break;
                }
            }
        });
        
        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            createPreviewGrid();
            updatePreview();
            previewUpdateInterval = setInterval(updatePreview, 100);
        });
        
        // Clean up
        window.addEventListener('unload', function() {
            if (previewUpdateInterval) {
                clearInterval(previewUpdateInterval);
            }
        });
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", html);
    });
    
    // API endpoint to control the game
    server->on("/tetrisControl", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("action")) {
            String action = request->getParam("action")->value();
            
            if (action == "aiOn") {
                toggleAIMode();
            }
            else if (action == "aiOff") {
                toggleAIMode();
            }
            else if (action == "start") {
                if (gameState == WAITING || gameState == GAME_OVER) {
                    initTetrisGame();
                }
            }
            else if (action == "pause") {
                if (gameState == PLAYING) {
                    gameState = PAUSED;
                } else if (gameState == PAUSED) {
                    gameState = PLAYING;
                }
            }
            else if (action == "restart") {
                g_tetrisInitialized = false;
                initTetrisGame();
            }
            else if (gameState == PLAYING) {
                if (action == "left") {
                    moveTetromino(T_LEFT);
                }
                else if (action == "right") {
                    moveTetromino(T_RIGHT);
                }
                else if (action == "down") {
                    moveTetromino(T_DOWN);
                }
                else if (action == "rotate") {
                    moveTetromino(T_ROTATE);
                }
            }
        }
        
        request->send(200, "text/plain", "OK");
    });
    
    // API endpoint to get game state
    server->on("/tetrisState", HTTP_GET, [](AsyncWebServerRequest *request) {
        String state;
        switch (gameState) {
            case WAITING: state = "waiting"; break;
            case PLAYING: state = "playing"; break;
            case PAUSED: state = "paused"; break;
            case GAME_OVER: state = "gameover"; break;
        }
        
        String json = "{\"score\":" + String(score) + 
                     ",\"level\":" + String(level) + 
                     ",\"state\":\"" + state + "\"}";
        request->send(200, "application/json", json);
    });
} 