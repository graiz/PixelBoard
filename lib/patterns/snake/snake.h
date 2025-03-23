#ifndef SNAKE_H
#define SNAKE_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>

// Direction constants
enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

// Forward declarations for internal functions
void initSnakeGame();
void placeFood();
void setDirection(Direction newDirection);
void updateSnakeGame();
void renderSnakeGame(CRGB* leds);
Direction getAIMove();
void toggleAIModeSnake();

// Function declarations for the snake pattern
void snake(CRGB* leds);
void setupSnakePattern(AsyncWebServer* server);

#endif // SNAKE_H 