#include "snake.h"
#include <led_display.h>
#include <FastLED.h>

// Game constants
#define GRID_SIZE 16
#define MAX_SNAKE_LENGTH 256 // Maximum possible length (16x16 grid)
#define INITIAL_SNAKE_LENGTH 3
#define GAME_SPEED 150      // Movement speed in milliseconds (constant)

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
static unsigned long gameOverTime; // Time when game ended for auto-restart

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
  gameSpeed = GAME_SPEED;
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
  if (gameState == WAITING) {
    // In WAITING state, wait for user to start
    return;
  }
  
  if (gameState == GAME_OVER) {
    // If in AI mode and game over, auto-restart after 5 seconds
    if (aiMode && (millis() - gameOverTime >= 5000)) {
      g_snakeInitialized = false;
      initSnakeGame();
      return;
    }
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Remove the early return to ensure more responsive updates
  if (currentTime - lastMoveTime >= gameSpeed) {
    // Update the last move time
    lastMoveTime = currentTime;
    
    // If AI mode is active, get the AI's next move
    if (aiMode) {
      nextDirection = getAIMove();
    }
    
    // Apply the nextDirection immediately for more responsive controls
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
      gameOverTime = millis(); // Record when game ended
      return;
    }
    
    // Check for collision with self
    for (int i = 0; i < snakeLength; i++) {
      if (newHeadX == snakeX[i] && newHeadY == snakeY[i]) {
        Serial.println("Game over: Self collision!");
        gameState = GAME_OVER;
        gameOverTime = millis(); // Record when game ended
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
      
      // Place new food
      placeFood();
      Serial.printf("New food placed at (%d,%d)\n", foodX, foodY);
    }
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
void toggleAIModeSnake() {
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
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="header">
        <div class="header-left">
            <h1>PixelBoard Snake</h1>
            <div class="score">Score: <span id="scoreValue">0</span></div>
            <div class="status" id="gameStatus">Press Start to Play</div>
        </div>
        <div class="header-right">
            <button class="d-btn" id="btnStart">Start Game</button>
            <button class="d-btn" id="btnRestart">Restart</button>
            <button class="d-btn" id="btnAI">Enable AI</button>
        </div>
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
    </div>

    <div style="text-align: center; margin: 5px; font-size: 0.8rem; color: #aaa;">
        Keyboard: Use arrow keys to control
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
        
        // Fetch game state from server
        function fetchGameState() {
            fetch('/snakeState')
                .then(response => response.json())
                .then(data => {
                    // Update game state
                    gameState = data.state;
                    
                    // Update score
                    document.getElementById('scoreValue').textContent = data.score;
                    
                    // Update game status text
                    const statusElement = document.getElementById('gameStatus');
                    switch (data.state) {
                        case 'waiting':
                            statusElement.textContent = 'Press Start to Play';
                            break;
                        case 'playing':
                            statusElement.textContent = 'Game In Progress';
                            break;
                        case 'gameover':
                            statusElement.textContent = 'Game Over! Press Restart';
                            break;
                    }
                    
                    // Update AI mode if it changed
                    if (aiMode !== data.aiMode) {
                        aiMode = data.aiMode;
                        updateAIButton();
                    }
                })
                .catch(error => console.error('Error fetching game state:', error));
        }
        
        // Send direction immediately without tracking current direction
        function sendDirection(direction) {
            fetch(`/snakeControl?dir=${direction}`)
                .then(response => response.text())
                .catch(error => console.error('Error sending direction:', error));
        }
        
        function startGame() {
            fetch('/snakeControl?action=start')
                .then(response => response.text())
                .then(() => {
                    gameState = 'playing';
                    document.getElementById('gameStatus').textContent = 'Game In Progress';
                })
                .catch(error => console.error('Error starting game:', error));
        }
        
        function restartGame() {
            fetch('/snakeControl?action=restart')
                .then(response => response.text())
                .then(() => {
                    gameState = 'playing';
                    document.getElementById('gameStatus').textContent = 'Game In Progress';
                    document.getElementById('scoreValue').textContent = '0';
                })
                .catch(error => console.error('Error restarting game:', error));
        }
        
        // Toggle AI mode
        function toggleAIMode() {
            aiMode = !aiMode;
            updateAIButton();
            
            fetch(`/snakeControl?action=${aiMode ? 'aiOn' : 'aiOff'}`)
                .then(response => response.text())
                .catch(error => console.error('Error toggling AI mode:', error));
        }
        
        // Update AI button appearance
        function updateAIButton() {
            const aiButton = document.getElementById('btnAI');
            if (aiMode) {
                aiButton.textContent = 'Disable AI';
                aiButton.classList.add('active');
                // When AI mode is enabled, disable manual controls except AI and Restart buttons
                document.querySelectorAll('.d-btn:not(#btnAI):not(#btnRestart)').forEach(btn => {
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
        document.getElementById('btnUp').addEventListener('click', () => {
            if (!aiMode && gameState === 'playing') {
                sendDirection('up');
            } else if (gameState === 'waiting') {
                startGame();
                sendDirection('up');
            }
        });
        
        document.getElementById('btnDown').addEventListener('click', () => {
            if (!aiMode && gameState === 'playing') {
                sendDirection('down');
            } else if (gameState === 'waiting') {
                startGame();
                sendDirection('down');
            }
        });
        
        document.getElementById('btnLeft').addEventListener('click', () => {
            if (!aiMode && gameState === 'playing') {
                sendDirection('left');
            } else if (gameState === 'waiting') {
                startGame();
                sendDirection('left');
            }
        });
        
        document.getElementById('btnRight').addEventListener('click', () => {
            if (!aiMode && gameState === 'playing') {
                sendDirection('right');
            } else if (gameState === 'waiting') {
                startGame();
                sendDirection('right');
            }
        });
        
        document.getElementById('btnStart').addEventListener('click', startGame);
        document.getElementById('btnRestart').addEventListener('click', restartGame);
        document.getElementById('btnAI').addEventListener('click', toggleAIMode);
        
        // Add keyboard controls
        document.addEventListener('keydown', function(e) {
            if (!aiMode && (gameState === 'playing' || gameState === 'waiting')) {
                switch(e.key) {
                    case 'ArrowUp':
                        if (gameState === 'waiting') startGame();
                        sendDirection('up');
                        break;
                    case 'ArrowDown':
                        if (gameState === 'waiting') startGame();
                        sendDirection('down');
                        break;
                    case 'ArrowLeft':
                        if (gameState === 'waiting') startGame();
                        sendDirection('left');
                        break;
                    case 'ArrowRight':
                        if (gameState === 'waiting') startGame();
                        sendDirection('right');
                        break;
                }
            }
        });
        
        // Initialize with faster preview updates
        document.addEventListener('DOMContentLoaded', function() {
            createPreviewGrid();
            updatePreview();
            previewUpdateInterval = setInterval(updatePreview, 50);
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