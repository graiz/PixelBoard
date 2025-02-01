#ifndef LED_DISPLAY_H
#define LED_DISPLAY_H
#define NUM_LEDS    256
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
extern int BRIGHTNESS;

#include <FastLED.h>

void setBrightness(int brightness);
void led_setup(CRGB* leds, int DATA_PIN, int LED_COUNT);
void loadArray(const long arr[], CRGB* leds, const long Color = 0 , const long replacementColor = 0);

#endif // LED_DISPLAY_H



