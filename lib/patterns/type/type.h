#ifndef TYPE_PATTERN_H
#define TYPE_PATTERN_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>

void type(CRGB* leds);
void setupTypePattern(AsyncWebServer* server);

#define XY(x, y) ((y) % 2 == 0 ?  ((y) * 16 + (15 - (x))) : ((y) * 16 + (x)) )

#endif // TYPE_PATTERN_H 