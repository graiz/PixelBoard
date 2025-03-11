#include "dvdbounce.h"
#include <FastLED.h>
#include <led_display.h>

// Global state for the DVD bounce pattern
static float x = 0;
static float y = 0;
static float dx = 0.5;  // X velocity
static float dy = 0.3;  // Y velocity
static float hue = 0; // Current hue for color shifting
static const int rectWidth = 4;  // Small rectangle for 16x16 grid
static const int rectHeight = 2; // Small rectangle for 16x16 grid

void dvdBounce(CRGB* leds) {
    // Update position
    x += dx;
    y += dy;

    // Bounce off edges
    if (x <= 0 || x + rectWidth >= 16) {
        dx = -dx;
        hue += 30; // Change color on bounce
    }
    if (y <= 0 || y + rectHeight >= 16) {
        dy = -dy;
        hue += 30; // Change color on bounce
    }

    // Keep hue in valid range (0-360)
    if (hue >= 360) hue -= 360;

    // Clear the display (fill with black)
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Convert HSV to RGB for current color
    CHSV hsv(hue, 255, 255);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);

    // Draw rectangle
    for (int i = x; i < x + rectWidth && i < 16; i++) {
        for (int j = y; j < y + rectHeight && j < 16; j++) {
            if (i >= 0 && j >= 0) {
                leds[XY(i, j)] = rgb;
            }
        }
    }
} 