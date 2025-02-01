#include "twinkle.h"
#include <led_display.h>

void twinkle(CRGB* leds) {
    // Randomly select LEDs to twinkle
    for (int i = 0; i < NUM_LEDS; i++) {
        if (random8() < 10) {  // 10/256 chance to start twinkling
            leds[i] = CRGB::White;
            leds[i].nscale8(random8(100, 255));  // Random brightness
        } else {
            // Gradually dim existing lights
            leds[i].nscale8(250);  // Slight dimming each frame
        }
    }
} 