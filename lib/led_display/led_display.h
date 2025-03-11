#ifndef LED_DISPLAY_H
#define LED_DISPLAY_H
#define NUM_LEDS    256
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
extern int BRIGHTNESS;

// XY mapping for 16x16 matrix with alternating row directions
#define XY(x, y) ((y) % 2 == 0 ?  ((y) * 16 + (15 - (x))) : ((y) * 16 + (x)) )

#include <FastLED.h>

void setBrightness(int brightness);
void led_setup(CRGB* leds, int DATA_PIN, int LED_COUNT);
void loadArray(const long arr[], CRGB* leds, const long Color = 0 , const long replacementColor = 0);

#endif // LED_DISPLAY_H



