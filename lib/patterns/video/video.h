#ifndef VIDEO_H
#define VIDEO_H

#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <led_display.h>
void video(CRGB* leds);
void setupVideoPlayer(AsyncWebServer* server);

#endif // VIDEO_PATTERN_H 