#include "snake.h"
#include <led_display.h>
#include <FastLED.h>

// Game constants
#define GRID_SIZE 16
#define MAX_SNAKE_LENGTH 256 // Maximum possible length (16x16 grid)
#define INITIAL_SNAKE_LENGTH 3
#define GAME_SPEED_INIT 200 // Milliseconds between moves (lower = faster)
#define GAME_SPEED_MIN 50   // Minimum delay (maximum speed)
#define SPEED_INCREASE 5    // How much to increase speed after eating food

// Game states
enum GameState {
  WAITING,
  PLAYING,
  GAME_OVER
};

// Game variables
static int snakeX[MAX_SNAKE_LENGTH];
static int snakeY[MAX_SNAKE_LENGTH];
static int snakeLength;
static Direction direction;
static Direction nextDirection; // For handling quick direction changes
static int foodX;
static int foodY;
static unsigned long lastMoveTime;
static unsigned long gameSpeed; // Time between moves in milliseconds
static GameState gameState;
static int score;
static CRGB snakeColor = CRGB::Green;
static CRGB foodColor = CRGB::Red;
static CRGB headColor = CRGB::Yellow;

// AI mode flag
static bool aiMode = false;

// Global flag for game initialization state
static bool g_snakeInitialized = false;

// Place food at a random position not occupied by the snake
void placeFood() {
  bool validPosition;
  
  // Keep trying until we find a valid position
  do {
    validPosition = true;
    foodX = random(GRID_SIZE);
    foodY = random(GRID_SIZE);
    
    // Check if food spawned on snake
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        validPosition = false;
        break;
      }
    }
  } while (!validPosition);
}

// Initialize game
void initSnakeGame() {
  // Initialize snake in the middle of the grid
  snakeLength = INITIAL_SNAKE_LENGTH;
  int middleX = GRID_SIZE / 2;
  int middleY = GRID_SIZE / 2;
  
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = middleX - i;
    snakeY[i] = middleY;
  }
  
  // Initialize direction
  direction = RIGHT;
  nextDirection = RIGHT;
  
  // Place food
  placeFood();
  
  // Initialize timers and game state
  lastMoveTime = millis();
  gameSpeed = GAME_SPEED_INIT;
  gameState = PLAYING;
  score = 0;
  
  Serial.println("Snake game initialized!");
  Serial.printf("Snake head at (%d,%d), direction: RIGHT\n", snakeX[0], snakeY[0]);
  Serial.printf("Food at (%d,%d)\n", foodX, foodY);
}

// Change the snake's direction
void setDirection(Direction newDirection) {
  // Prevent 180-degree turns (e.g., can't go RIGHT if currently going LEFT)
  if ((newDirection == UP && direction != DOWN) ||
      (newDirection == DOWN && direction != UP) ||
      (newDirection == LEFT && direction != RIGHT) ||
      (newDirection == RIGHT && direction != LEFT)) {
    nextDirection = newDirection;
    Serial.printf("Direction changed to: %d\n", (int)newDirection);
  }
}

// Determine AI's next move
Direction getAIMove() {
  int headX = snakeX[0];
  int headY = snakeY[0];
  
  // Simple AI logic:
  // 1. Try to move toward food
  // 2. Avoid walls and self
  
  // Check which directions are safe (no wall or self collision)
  bool canMoveUp = (headY > 0);
  bool canMoveDown = (headY < GRID_SIZE - 1);
  bool canMoveLeft = (headX > 0);
  bool canMoveRight = (headX < GRID_SIZE - 1);
  
  // Check for self collisions
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == headX && snakeY[i] == headY - 1) canMoveUp = false;
    if (snakeX[i] == headX && snakeY[i] == headY + 1) canMoveDown = false;
    if (snakeX[i] == headX - 1 && snakeY[i] == headY) canMoveLeft = false;
    if (snakeX[i] == headX + 1 && snakeY[i] == headY) canMoveRight = false;
  }
  
  // Avoid 180° turns
  if (direction == DOWN) canMoveUp = false;
  if (direction == UP) canMoveDown = false;
  if (direction == RIGHT) canMoveLeft = false;
  if (direction == LEFT) canMoveRight = false;
  
  // Try to move toward food
  if (foodY < headY && canMoveUp) return UP;
  if (foodY > headY && canMoveDown) return DOWN;
  if (foodX < headX && canMoveLeft) return LEFT;
  if (foodX > headX && canMoveRight) return RIGHT;
  
  // If can't move directly toward food, choose any safe direction
  if (canMoveUp) return UP;
  if (canMoveRight) return RIGHT;
  if (canMoveDown) return DOWN;
  if (canMoveLeft) return LEFT;
  
  // If no safe moves, continue in current direction (will likely crash)
  return direction;
}

// Update game state
void updateSnakeGame() {
  if (gameState == WAITING || gameState == GAME_OVER) {
    // In WAITING or GAME_OVER state, wait for user to start/restart
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime < gameSpeed) {
    // Not time to move yet
    return;
  }
  
  // Update the last move time
  lastMoveTime = currentTime;
  
  // If AI mode is active, get the AI's next move
  if (aiMode) {
    nextDirection = getAIMove();
  }
  
  // Apply the nextDirection
  direction = nextDirection;
  
  // Calculate new head position
  int newHeadX = snakeX[0];
  int newHeadY = snakeY[0];
  
  // Update head position based on direction
  switch (direction) {
    case UP:
      newHeadY--;
      break;
    case DOWN:
      newHeadY++;
      break;
    case LEFT:
      newHeadX--;
      break;
    case RIGHT:
      newHeadX++;
      break;
  }
  
  // Debug info
  Serial.printf("Snake moving: Head from (%d,%d) to (%d,%d), Dir: %d\n", 
                snakeX[0], snakeY[0], newHeadX, newHeadY, (int)direction);
  
  // Check for wall collision
  if (newHeadX < 0 || newHeadX >= GRID_SIZE ||
      newHeadY < 0 || newHeadY >= GRID_SIZE) {
    Serial.println("Game over: Wall collision!");
    gameState = GAME_OVER;
    return;
  }
  
  // Check for collision with self
  for (int i = 0; i < snakeLength; i++) {
    if (newHeadX == snakeX[i] && newHeadY == snakeY[i]) {
      Serial.println("Game over: Self collision!");
      gameState = GAME_OVER;
      return;
    }
  }
  
  // Check if food eaten
  bool foodEaten = (newHeadX == foodX && newHeadY == foodY);
  
  // Move the snake: shift all segments one position back
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  
  // Update head position
  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;
  
  // If food eaten, grow snake and place new food
  if (foodEaten) {
    Serial.println("Food eaten! Snake growing.");
    
    // Properly initialize the new tail segment
    // Copy the position of the current tail to the new segment
    snakeX[snakeLength] = snakeX[snakeLength-1];
    snakeY[snakeLength] = snakeY[snakeLength-1];
    
    // Now it's safe to increase the length
    snakeLength++;
    
    if (snakeLength >= MAX_SNAKE_LENGTH) {
      // Snake reached maximum size - you win!
      Serial.println("You win! Maximum snake length reached.");
      gameState = GAME_OVER;
      return;
    }
    
    // Update score
    score++;
    
    // Speed up the game
    if (gameSpeed > GAME_SPEED_MIN) {
      gameSpeed -= SPEED_INCREASE;
      Serial.printf("Speed increased. New game speed: %lu\n", gameSpeed);
    }
    
    // Place new food
    placeFood();
    Serial.printf("New food placed at (%d,%d)\n", foodX, foodY);
  }
}

// Render the game on the LED matrix
void renderSnakeGame(CRGB* leds) {
  // Clear the display
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Draw the food - brighter to make it more visible
  CRGB brightFood = foodColor;
  brightFood.maximizeBrightness();
  leds[XY(foodX, foodY)] = brightFood;
  
  // Draw the snake body
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[i] >= 0 && snakeX[i] < GRID_SIZE && 
        snakeY[i] >= 0 && snakeY[i] < GRID_SIZE) {
      leds[XY(snakeX[i], snakeY[i])] = snakeColor;
    }
  }
  
  // Draw the snake head - always last to ensure it's visible
  if (snakeX[0] >= 0 && snakeX[0] < GRID_SIZE && 
      snakeY[0] >= 0 && snakeY[0] < GRID_SIZE) {
    // Make head brighter than body
    CRGB brightHead = headColor;
    brightHead.maximizeBrightness();
    leds[XY(snakeX[0], snakeY[0])] = brightHead;
  }
  
  // If game over, flash the display
  if (gameState == GAME_OVER) {
    if ((millis() / 500) % 2 == 0) { // Flash at 2Hz (every 500ms)
      for (int i = 0; i < NUM_LEDS; i++) {
        if (leds[i]) { // If LED is on
          // Make red brighter for better visibility
          leds[i] = CRGB::Red;
          leds[i].maximizeBrightness();
        }
      }
    }
  }
}

// Main pattern function that will be called from patterns.cpp
void snake(CRGB* leds) {
  // Initialize game if needed
  static unsigned long lastDebugTime = 0;
  
  if (!g_snakeInitialized) {
    initSnakeGame();
    g_snakeInitialized = true;
    Serial.println("Snake game started.");
  }
  
  // Debug: Print game state every 5 seconds
  unsigned long currentTime = millis();
  if (currentTime - lastDebugTime > 5000) {
    Serial.printf("Snake game state: %d, Score: %d, Length: %d\n", 
                  (int)gameState, score, snakeLength);
    Serial.printf("Snake position: Head(%d,%d), Food(%d,%d)\n", 
                  snakeX[0], snakeY[0], foodX, foodY);
    lastDebugTime = currentTime;
  }
  
  // Update game state
  updateSnakeGame();
  
  // Render game
  renderSnakeGame(leds);
}

// Toggle AI mode
void toggleAIMode() {
  aiMode = !aiMode;
  Serial.printf("AI mode %s\n", aiMode ? "enabled" : "disabled");
}

// Web server setup function
void setupSnakePattern(AsyncWebServer* server) {
  // Serve the snake game control page
  server->on("/snake", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>PixelBoard Snake Game</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #282c34;
            color: #ffffff;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            overflow: hidden; /* Prevent scrolling in iframe */
        }
        .header {
            background: #3b3f47;
            padding: 10px 15px;
            border-bottom: 1px solid #61dafb;
            text-align: center;
        }
        h1 {
            margin: 0;
            font-size: 1.4rem;
        }
        .game-info {
            background: #3b3f47;
            padding: 8px 15px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .score {
            font-size: 1.1rem;
            font-weight: bold;
        }
        .controls {
            padding: 10px;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 8px;
        }
        .d-pad {
            display: grid;
            grid-template-columns: repeat(3, 60px);
            grid-template-rows: repeat(3, 60px);
            gap: 5px;
        }
        .d-btn {
            background-color: #444;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            cursor: pointer;
            user-select: none;
            -webkit-user-select: none;
            transition: all 0.2s;
            box-shadow: 0 3px 0 #333, 0 4px 5px rgba(0,0,0,0.3);
            text-shadow: 0 1px 2px rgba(0,0,0,0.5);
        }
        .d-btn:active {
            background-color: #555;
            box-shadow: 0 1px 0 #333, 0 2px 3px rgba(0,0,0,0.3);
            transform: translateY(2px);
        }
        .d-btn.up {
            grid-column: 2;
            grid-row: 1;
        }
        .d-btn.left {
            grid-column: 1;
            grid-row: 2;
        }
        .d-btn.right {
            grid-column: 3;
            grid-row: 2;
        }
        .d-btn.down {
            grid-column: 2;
            grid-row: 3;
        }
        .center {
            grid-column: 2;
            grid-row: 2;
            background-color: #333;
            border-radius: 6px;
        }
        .key-icon {
            font-family: monospace;
            font-weight: bold;
            font-size: 18px;
            padding: 4px 8px;
            background-color: #222;
            border-radius: 4px;
            border: 1px solid #555;
            box-shadow: inset 0 0 3px rgba(0,0,0,0.5);
        }
        .game-buttons {
            display: flex;
            gap: 15px;
            margin-top: 15px;
        }
        .game-btn {
            background-color: #444;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            font-size: 15px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
            box-shadow: 0 3px 0 #333, 0 4px 5px rgba(0,0,0,0.3);
        }
        .game-btn:hover {
            background-color: #555;
        }
        .game-btn:active {
            background-color: #555;
            box-shadow: 0 1px 0 #333, 0 2px 3px rgba(0,0,0,0.3);
            transform: translateY(2px);
        }
        .ai-mode {
            margin-top: 15px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
        }
        .ai-btn {
            background-color: #444;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.2s;
            box-shadow: 0 3px 0 #333, 0 4px 5px rgba(0,0,0,0.3);
        }
        .ai-btn.active {
            background-color: #61dafb;
            color: #282c34;
        }
        .ai-btn:hover {
            background-color: #555;
        }
        .ai-btn:active {
            background-color: #555;
            box-shadow: 0 1px 0 #333, 0 2px 3px rgba(0,0,0,0.3);
            transform: translateY(2px);
        }
        .preview-container {
            margin: 10px;
            display: flex;
            justify-content: center;
            flex: 1;
        }
        .preview-grid {
            display: grid;
            grid-template-columns: repeat(16, 1fr);
            gap: 2px;
            padding: 8px;
            background: #3b3f47;
            border-radius: 10px;
            aspect-ratio: 1;
            width: min(90%, 400px);
            max-height: 80vh;
        }
        .preview-pixel {
            aspect-ratio: 1;
            background: #282c34;
            border-radius: 2px;
            transition: background-color 0.3s ease;
        }
        /* Iframe-specific adjustments */
        @media screen and (max-height: 700px) {
            .header {
                padding: 5px;
            }
            h1 {
                font-size: 1.2rem;
            }
            .game-info {
                padding: 5px 10px;
            }
            .d-pad {
                grid-template-columns: repeat(3, 50px);
                grid-template-rows: repeat(3, 50px);
            }
            .controls {
                padding: 5px;
            }
            .game-buttons {
                margin-top: 10px;
            }
            .game-btn {
                padding: 8px 15px;
                font-size: 14px;
            }
            .key-icon {
                font-size: 16px;
                padding: 3px 6px;
            }
            .ai-mode {
                margin-top: 8px;
            }
            .ai-btn {
                padding: 6px 12px;
                font-size: 12px;
            }
        }
        
        @media screen and (max-height: 600px) {
            .d-pad {
                grid-template-columns: repeat(3, 40px);
                grid-template-rows: repeat(3, 40px);
            }
            .key-icon {
                font-size: 14px;
                padding: 2px 4px;
            }
            .game-btn {
                padding: 6px 12px;
                font-size: 12px;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>PixelBoard Snake Game</h1>
    </div>
    
    <div class="game-info">
        <div class="score">Score: <span id="scoreValue">0</span></div>
        <div class="status" id="gameStatus">Press Start to Play</div>
    </div>
    
    <div class="preview-container">
        <div class="preview-grid" id="previewGrid"></div>
    </div>
    
    <div class="controls">
        <div class="d-pad">
            <button class="d-btn up" id="btnUp"><span class="key-icon">&uarr;</span></button>
            <button class="d-btn left" id="btnLeft"><span class="key-icon">&larr;</span></button>
            <div class="center"></div>
            <button class="d-btn right" id="btnRight"><span class="key-icon">&rarr;</span></button>
            <button class="d-btn down" id="btnDown"><span class="key-icon">&darr;</span></button>
        </div>
        
        <div class="game-buttons">
            <button class="game-btn" id="btnStart">Start Game</button>
            <button class="game-btn" id="btnRestart">Restart</button>
        </div>
        
        <!-- Add AI mode toggle -->
        <div class="ai-mode">
            <button class="ai-btn" id="btnAI">Enable AI Mode</button>
        </div>
    </div>

    <!-- Add keyboard shortcut info -->
    <div style="text-align: center; margin-top: 10px; font-size: 0.8rem; color: #aaa;">
        Keyboard: Use arrow keys to control
    </div>

    <script>
        let previewUpdateInterval;
        let score = 0;
        let gameState = 'waiting'; // 'waiting', 'playing', 'gameover'
        let lastDirection = ''; // Track last direction sent
        let controlInterval; // Interval for continuous control
        let aiMode = false; // Track AI mode state
        
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
                
            // Also update the game state
            fetchGameState();
        }
        
        // Fetch game state (score and status)
        function fetchGameState() {
            fetch('/snakeState')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('scoreValue').textContent = data.score;
                    
                    const statusElem = document.getElementById('gameStatus');
                    if (data.state === 'waiting') {
                        statusElem.textContent = 'Press Start to Play';
                        gameState = 'waiting';
                    } else if (data.state === 'playing') {
                        statusElem.textContent = 'Game In Progress';
                        gameState = 'playing';
                    } else if (data.state === 'gameover') {
                        statusElem.textContent = 'Game Over!';
                        gameState = 'gameover';
                        clearInterval(controlInterval); // Stop continuous control
                    }
                    
                    // Update AI mode button if state changed from backend
                    if (data.aiMode !== undefined && data.aiMode !== aiMode) {
                        aiMode = data.aiMode;
                        updateAIButton();
                    }
                })
                .catch(error => console.error('Error fetching game state:', error));
        }
        
        // Control functions
        function sendDirection(direction) {
            lastDirection = direction; // Store last direction
            
            fetch(`/snakeControl?dir=${direction}`)
                .then(response => response.text())
                .catch(error => console.error('Error sending direction:', error));
        }
        
        // Start continuous control - sends last direction every 100ms to keep snake moving
        function startContinuousControl() {
            if (controlInterval) {
                clearInterval(controlInterval);
            }
            
            controlInterval = setInterval(() => {
                if (gameState === 'playing' && lastDirection) {
                    sendDirection(lastDirection);
                }
            }, 100); // Send every 100ms
        }
        
        function startGame() {
            fetch('/snakeControl?action=start')
                .then(response => response.text())
                .then(() => {
                    gameState = 'playing';
                    document.getElementById('gameStatus').textContent = 'Game In Progress';
                    startContinuousControl(); // Start continuous control
                })
                .catch(error => console.error('Error starting game:', error));
        }
        
        function restartGame() {
            fetch('/snakeControl?action=restart')
                .then(response => response.text())
                .then(() => {
                    gameState = 'playing'; // Set to playing since we modified initSnakeGame
                    document.getElementById('gameStatus').textContent = 'Game In Progress';
                    document.getElementById('scoreValue').textContent = '0';
                    startContinuousControl(); // Restart continuous control
                })
                .catch(error => console.error('Error restarting game:', error));
        }
        
        // Toggle AI mode
        function toggleAIMode() {
            aiMode = !aiMode;
            updateAIButton();
            
            // Send mode change to server
            fetch(`/snakeControl?action=${aiMode ? 'aiOn' : 'aiOff'}`)
                .then(response => response.text())
                .catch(error => console.error('Error toggling AI mode:', error));
        }
        
        // Update AI button appearance
        function updateAIButton() {
            const aiButton = document.getElementById('btnAI');
            if (aiMode) {
                aiButton.textContent = 'Disable AI Mode';
                aiButton.classList.add('active');
                // When AI mode is enabled, disable manual controls
                document.querySelectorAll('.d-btn').forEach(btn => {
                    btn.disabled = true;
                    btn.style.opacity = 0.5;
                });
            } else {
                aiButton.textContent = 'Enable AI Mode';
                aiButton.classList.remove('active');
                // Re-enable manual controls
                document.querySelectorAll('.d-btn').forEach(btn => {
                    btn.disabled = false;
                    btn.style.opacity = 1;
                });
            }
        }
        
        // Add event listeners
        document.getElementById('btnUp').addEventListener('click', () => {
            sendDirection('up');
            if (gameState === 'waiting') startGame(); // Auto-start on direction
        });
        
        document.getElementById('btnDown').addEventListener('click', () => {
            sendDirection('down');
            if (gameState === 'waiting') startGame(); // Auto-start on direction
        });
        
        document.getElementById('btnLeft').addEventListener('click', () => {
            sendDirection('left');
            if (gameState === 'waiting') startGame(); // Auto-start on direction
        });
        
        document.getElementById('btnRight').addEventListener('click', () => {
            sendDirection('right');
            if (gameState === 'waiting') startGame(); // Auto-start on direction
        });
        
        document.getElementById('btnStart').addEventListener('click', startGame);
        document.getElementById('btnRestart').addEventListener('click', restartGame);
        document.getElementById('btnAI').addEventListener('click', toggleAIMode);
        
        // Add keyboard controls
        document.addEventListener('keydown', function(e) {
            // Skip keyboard controls if AI mode is enabled
            if (aiMode) return;
            
            let dirSent = false;
            
            switch(e.key) {
                case 'ArrowUp':
                    sendDirection('up');
                    dirSent = true;
                    break;
                case 'ArrowDown':
                    sendDirection('down');
                    dirSent = true;
                    break;
                case 'ArrowLeft':
                    sendDirection('left');
                    dirSent = true;
                    break;
                case 'ArrowRight':
                    sendDirection('right');
                    dirSent = true;
                    break;
            }
            
            // Auto-start game on first direction key
            if (dirSent && gameState === 'waiting') {
                startGame();
            }
        });
        
        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            createPreviewGrid();
            updatePreview();
            // Update more frequently for better responsiveness
            previewUpdateInterval = setInterval(updatePreview, 100);
            startContinuousControl(); // Start continuous control
        });
        
        // Clean up interval when page is unloaded
        window.addEventListener('unload', function() {
            if (previewUpdateInterval) {
                clearInterval(previewUpdateInterval);
            }
            if (controlInterval) {
                clearInterval(controlInterval);
            }
        });
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });
  
  // API endpoint to control the snake
  server->on("/snakeControl", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Handle direction commands
    if (request->hasParam("dir")) {
      String dir = request->getParam("dir")->value();
      
      // If game is in WAITING state, automatically start it on first direction command
      if (gameState == WAITING) {
        gameState = PLAYING;
        Serial.println("Game state changed to PLAYING via direction command");
      }
      
      if (dir == "up") {
        setDirection(UP);
        Serial.println("Direction set to UP");
      } else if (dir == "down") {
        setDirection(DOWN);
        Serial.println("Direction set to DOWN");
      } else if (dir == "left") {
        setDirection(LEFT);
        Serial.println("Direction set to LEFT");
      } else if (dir == "right") {
        setDirection(RIGHT);
        Serial.println("Direction set to RIGHT");
      }
    }
    
    // Handle game actions
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();
      
      if (action == "start") {
        // Always allow starting the game
        gameState = PLAYING;
        Serial.println("Game state changed to PLAYING via start action");
      } else if (action == "restart") {
        // Reset the initialized flag to force reinitialization
        g_snakeInitialized = false;
        
        initSnakeGame();
        Serial.println("Game restarted");
      } else if (action == "aiOn") {
        // Enable AI mode
        aiMode = true;
        Serial.println("AI mode enabled");
      } else if (action == "aiOff") {
        // Disable AI mode
        aiMode = false;
        Serial.println("AI mode disabled");
      }
    }
    
    request->send(200, "text/plain", "OK");
  });
  
  // API endpoint to get game state
  server->on("/snakeState", HTTP_GET, [](AsyncWebServerRequest *request) {
    String state;
    switch (gameState) {
      case WAITING:
        state = "waiting";
        break;
      case PLAYING:
        state = "playing";
        break;
      case GAME_OVER:
        state = "gameover";
        break;
    }
    
    String json = "{\"score\":" + String(score) + ",\"state\":\"" + state + "\",\"aiMode\":" + (aiMode ? "true" : "false") + "}";
    request->send(200, "application/json", json);
  });
} 