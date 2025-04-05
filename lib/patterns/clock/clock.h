#ifndef CLOCK_H
#define CLOCK_H

#include <FastLED.h>
#include <ESPAsyncWebServer.h>

// Function declarations
void nap(int wait);
void clockCountdown(CRGB* leds);
void setupClockPattern(AsyncWebServer* server);
void resetClock();

#endif // CLOCK_H 