#pragma once
#include <FastLED.h>
#include <ESPAsyncWebServer.h>

void draw(CRGB* leds);
void setupDrawPattern(AsyncWebServer* server);

#define XY(x, y) ((y) % 2 == 0 ?  ((y) * 16 + (15 - (x))) : ((y) * 16 + (x)) ) 