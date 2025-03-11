#ifndef TYPE_PATTERN_H
#define TYPE_PATTERN_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <led_display.h>

void type(CRGB* leds);
void setupTypePattern(AsyncWebServer* server);

#endif // TYPE_PATTERN_H 