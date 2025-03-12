#include <FastLED.h>
#include <led_display.h>
#include <math.h>

// Function declaration for pattern registration
void beachBall(CRGB* leds);

// Global variables for the beach ball pattern
static float rotation = 0.0f;           // Current rotation angle in radians
static const int centerX = 8;           // Center X (for 16x16 grid)
static const int centerY = 8;           // Center Y
static const int radius = 12;            // Radius of the beach ball
static const float rotationSpeed = 0.09f; // Rotation speed, adjust as needed

void beachBall(CRGB* leds) {
    // Clear the display
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    // For each pixel, determine if it's within the beach ball radius and what color it should be
    for (uint8_t y = 0; y < 16; y++) {
        for (uint8_t x = 0; x < 16; x++) {
            // Calculate distance from center to determine if pixel is inside ball
            int16_t dx = x - centerX;
            int16_t dy = y - centerY; 
            float distance = sqrt(dx*dx + dy*dy);
            
            // Only draw pixels inside the beach ball radius
            if (distance <= radius) {
                // Calculate angle in degrees [0..360)
                float angle = atan2f((float)dy, (float)dx) * (180.0f / PI);
                if (angle < 0) {
                    angle += 360.0f;  // shift to [0..360)
                }
                
                // Add the rotation offset (convert rotation from radians to degrees first)
                float rotatedAngle = angle + (rotation * 180.0f / PI);
                while (rotatedAngle >= 360.0f) {
                    rotatedAngle -= 360.0f;
                }
                
                // Map angle directly to hue as requested
                uint8_t hue = map(rotatedAngle, 0, 360, 0, 255);
                
                // Color the pixel using HSV for vibrant colors
                leds[XY(x, y)] = CHSV(hue, 255, 255);
            }
        }
    }
    // Increment rotation for next frame
    rotation += rotationSpeed;
    if (rotation >= 2*PI) {
        rotation -= 2*PI;
    }
    
    // Short delay each frame
    delay(20);
} 