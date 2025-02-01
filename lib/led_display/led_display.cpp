#include "led_display.h"


void led_setup(CRGB* leds, int DATA_PIN, int LED_COUNT){
    // tell FastLED about the LED strip configuration
  FastLED.setBrightness(200);
}

void setBrightness(int brightness){
  FastLED.setBrightness(brightness);
}

void loadArray(const long arr[], CRGB* leds, const long Color /*= 0*/, const long replacementColor /*= 0*/) {
  for(int i = 0; i < NUM_LEDS; i++) {
    long color = pgm_read_dword(&(arr[i]));  // Read array from Flash
    if (color == Color) {
        leds[i] = replacementColor;
    } else {
        leds[i] = color;
    }
  }
  FastLED.show();
}
