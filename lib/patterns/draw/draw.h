#ifndef DRAW_PATTERN_H
#define DRAW_PATTERN_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <led_display.h>

void draw(CRGB* leds);
void setupDrawPattern(AsyncWebServer* server);

#endif // DRAW_PATTERN_H 