#ifndef PATTERNS_H
#define PATTERNS_H

#ifndef NUM_LEDS
#define NUM_LEDS 256
#endif 

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include <led_display.h>
#include <FastLED.h>

// Forward declarations of helper functions
void nap(int wait);




// 1) Define a struct to hold pattern info (name + function pointer).
struct Pattern {
    const char* name;
    void (*func)(CRGB* leds);
};

// 2) Declare an array of Pattern objects
extern Pattern g_patternList[];

// 3) Declare a variable that represents the number of patterns
extern const size_t PATTERN_COUNT;

#endif // PATTERNS_H