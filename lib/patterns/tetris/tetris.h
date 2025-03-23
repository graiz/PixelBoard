#ifndef TETRIS_H
#define TETRIS_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>

// Game states
enum TetrisGameState {
    WAITING,
    PLAYING,
    PAUSED,
    GAME_OVER
};

// Tetromino rotation states (0-3 for 4 possible rotations)
enum Rotation {
    ROT_0,
    ROT_90,
    ROT_180,
    ROT_270
};

// Movement directions
enum TetrisDirection {
    T_LEFT,
    T_RIGHT,
    T_DOWN,
    T_ROTATE
};

// Forward declarations for internal functions
void initTetrisGame();
void moveTetromino(TetrisDirection dir);
void updateTetrisGame();
void renderTetrisGame(CRGB* leds);
void rotateTetromino();
void dropTetromino();
void clearLines();
bool checkCollision();
void lockTetromino();
void spawnTetromino();


// Main pattern function
void tetris(CRGB* leds);
void setupTetrisPattern(AsyncWebServer* server);

#endif // TETRIS_H 