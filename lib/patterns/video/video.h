#ifndef VIDEO_H
#define VIDEO_H

#include <ESPAsyncWebServer.h>
#include <FastLED.h>

void video(CRGB* leds);
void setupVideoPlayer(AsyncWebServer* server);

#define XY(x, y) ((y) % 2 == 0 ?  ((y) * 16 + (15 - (x))) : ((y) * 16 + (x)) ) 

#endif // VIDEO_H 