#include <patterns.h>
#include <led_display.h>

// Include complex patterns with UI components
#include "draw/draw.h"
#include "video/video.h"
#include "type/type.h"
#include "snake/snake.h"
#include "tetris/tetris.h"
#include "clock/clock.h"  // Add new clock pattern header

// Include simple patterns directly
#include "twinkle.cpp"
#include "dvdbounce.cpp"
#include "beachball.cpp"
#include "games.cpp"

// Forward declarations of all pattern functions
void pac(CRGB* leds);
void qbert(CRGB* leds);
void mario(CRGB* leds);
void ghost(CRGB* leds);
void water(CRGB* leds);
void rainbow(CRGB* leds);
void rainbowWithGlitter(CRGB* leds);
void pulse(CRGB* leds);
void confetti(CRGB* leds); 
void sinelon(CRGB* leds);
void bpm(CRGB* leds);
void watermatrix(CRGB* leds);
void juggle(CRGB* leds); 
void firefunction(CRGB* leds);
void greenBlackLoop(CRGB* leds);
void explode(CRGB* leds);
void sleepLED(CRGB* leds);
void swirl(CRGB* leds);
void meteorRain(CRGB* leds);
void colorWipe(CRGB* leds);
void clockCountdown(CRGB* leds);  // Keep declaration
void dvdBounce(CRGB* leds);
void beachBall(CRGB* leds);
void randomPattern(CRGB* leds);
void sparkler(CRGB* leds);  // New sparkler pattern

// Provide the actual array definition
Pattern g_patternList[] = {
    { "Fire",              firefunction },
    { "The Matrix",        greenBlackLoop },
    { "Pac Man Ghost",     ghost },
    { "Qbert",             qbert },
    { "DVD Bounce",        dvdBounce },
    { "Ms Pac-Man",        pac },
    { "Jelly Fish",        water },
    { "Super Mario",       mario },
    { "Rainbow Drift",     rainbow },
    { "Pixel Swaps",       watermatrix },
    { "Rainbow Glitter",   rainbowWithGlitter },
    { "Confetti",          confetti },
    { "Up Down Rainbow",   sinelon },
    { "Juggle",            juggle },
    { "Twinkle",           twinkle },
    { "Sleep Device",      sleepLED },
    { "Swirl",             swirl },
    { "Game of Life",      meteorRain },
    { "Color Wipe",        colorWipe },
    { "Beach Ball",        beachBall },
    { "Clock Countdown",   clockCountdown },  // Keep pattern entry
    { "Draw",              draw },
    { "Video",             video },
    { "Type",              type },
    { "Random",            randomPattern },
    { "Snake Game",         snake },
    { "Tetris Game",       tetris },
    { "Sparkler",          sparkler }  // Replace spirograph with sparkler
};

// And the size of that array
const size_t PATTERN_COUNT = sizeof(g_patternList) / sizeof(g_patternList[0]);

// FastLED "100-lines-of-code" demo reel, initial inspiration via Mark Kriegsman, December 2014

extern int g_Speed;

extern uint8_t g_hue; // rotating "base color" used by many of the patterns


void nap(int wait){
   float Delay;
   Delay = (2000/g_Speed)+wait; 
   FastLED.delay(Delay);
}

void sleepLED(CRGB* leds) {
    // Turn off all LEDs
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    
    // Set brightness to minimum
    FastLED.setBrightness(0);
    FastLED.show();
    
    // Enter low power mode
    delay(100); // Short delay to ensure display is off
    
    // Keep WiFi on but in modem sleep mode
    WiFi.setSleep(true);
    
    // Configure light sleep wake-up sources
    esp_sleep_enable_timer_wakeup(600 * 1000000); // 10 minutes failsafe timer
    esp_sleep_enable_wifi_wakeup();  // Enable wake on WiFi activity
    
    // Disable Bluetooth for power saving
    btStop();
    
    // Enter light sleep (maintains WiFi connection)
    esp_light_sleep_start();
    
    // After wake-up:
    WiFi.setSleep(false); // Return WiFi to normal mode
    
    // Restore normal brightness
    FastLED.setBrightness(g_Brightness);
    FastLED.show();
}


void water(CRGB* leds) {
  loadArray(wa1,leds); nap(50);
  loadArray(wa2,leds); nap(50);
  loadArray(wa3,leds); nap(50);
  loadArray(wa4,leds); nap(50);
  loadArray(wa5,leds), nap(50);
  loadArray(wa6,leds); nap(50);
  loadArray(wa7,leds); nap(50);
  loadArray(wa8,leds); nap(50);
  loadArray(wa9,leds); nap(50);
  loadArray(wa10,leds); nap(50);
 }
void rainbow(CRGB* leds) 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, g_hue, 7);
}
void addGlitter( fract8 chanceOfGlitter,CRGB* leds) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
void rainbowWithGlitter(CRGB* leds) 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow(leds);
  addGlitter(80,leds);
}


void pulse(CRGB* leds) {
  static uint8_t hue = 0;
  static uint16_t time = 0;
  uint8_t brightness = (sin8(time) + 255) / 2;
  CRGB color = CHSV(hue, 255, brightness);
  fill_solid(leds, NUM_LEDS, color);
  hue += 1;
  time += 10;
}

void confetti(CRGB* leds) 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( g_hue + random8(64), 200, 255);
}
void sinelon(CRGB* leds)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( g_hue, 255, 192);
}
void bpm(CRGB* leds)
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, g_hue+(i*2), beat-g_hue+(i*10));
  }
}


void firefunction(CRGB* leds)
{
  const uint8_t cols = 16;
  const uint8_t rows = 16;

  uint8_t cooling = 25;
  static uint8_t heat[cols][rows];
  nap(20);
  // Step 1. Cool down every cell a little
  for (int i = 0; i < cols; i++) {
    for (int j = 0; j < rows; j++) {
      heat[i][j] = qsub8(heat[i][j], random8(0, ((cooling * 10) / rows) + 2));
    }
  }

  // Step 2. Heat from each cell drifts 'up' and diffuses a little
  for (int j = 0; j < rows - 1; j++) {
    for (int i = 0; i < cols; i++) {
      heat[i][j] = (heat[i][j + 1] + heat[(i + 1) % cols][j +1] + heat[(i + cols - 1) % cols][j + 1] + heat[i][j + 1]) / 4;
    }
  }

  // Step 3. Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < 90) {
    int sparkX = random8(cols);
    heat[sparkX][rows - 1] = qadd8(heat[sparkX][rows - 1], random8(160, 200));
  }

  // Step 4. Map from heat cells to LED colors
  for (int i = 0; i < cols; i++) {
    for (int j = 0; j < rows; j++) {
      int colorindex = scale8(heat[i][j], 240);
      leds[XY(i, j)] = ColorFromPalette(HeatColors_p, colorindex);
    }
  }
}


// Define a structure to represent a falling dot
struct FallingDot {
  int8_t col; // Current column
  int8_t row; // Current row
  uint8_t speed; // Speed of the dot (1 to 5)
  uint8_t counter; // Frame counter for animation
  bool active; // Whether the dot is active or not
  uint8_t brightness; 
};

void greenBlackLoop(CRGB* leds) {
  const uint8_t cols = 16;
  const uint8_t rows = 16;
  const uint8_t numDots = 10; // Number of falling dots
  static FallingDot dots[numDots];
  static bool isInitialized = false;

  // Initialize the falling dots with random starting positions and speeds
  if (!isInitialized) {
    for (int i = 0; i < numDots; i++) {
      dots[i].col = random(cols);
      dots[i].row = random(rows);
      dots[i].speed = random(1, 6); // Random speed from 1 to 5
      dots[i].counter = 0;
      dots[i].brightness = random(10, 256); 
      dots[i].active = true;
    }
    isInitialized = true;
  }
  fadeToBlackBy(leds, NUM_LEDS, 80);

    // Update the position of each dot and render it
    for (int i = 0; i < numDots; i++) {
      if (dots[i].active) {
        // Increment the counter
        dots[i].counter++;

        // Check if it's time to move the dot down based on its speed
        if (dots[i].counter >= dots[i].speed) {

          // Move the dot down by one row
          dots[i].row++;

          // If the dot reaches the bottom, deactivate it
          if (dots[i].row >= rows) {
            dots[i].active = false;
          } else {
            // Render the dot at the new position
            leds[XY(dots[i].col, dots[i].row)] = CRGB(0, dots[i].brightness, 0); // Use dot's brightness for the green component
          }

          // Reset the counter
          dots[i].counter = 0;
        }
      }
    }

    // Randomly activate an inactive dot in a random column at the top
    int inactiveDotIndex = -1;
    for (int i = 0; i < numDots; i++) {
      if (!dots[i].active) {
        inactiveDotIndex = i;
        break;
      }
    }
    if (inactiveDotIndex >= 0) {
      dots[inactiveDotIndex].col = random(cols);
      dots[inactiveDotIndex].row = 0;
      dots[inactiveDotIndex].speed = random(1, 5); // Random speed from 1 to 5
      dots[inactiveDotIndex].active = true;
      leds[XY(dots[inactiveDotIndex].col, dots[inactiveDotIndex].row)] = CRGB::Green;
    }

    // Update the display
    FastLED.show();
    nap(5); // Adjust this delay to control the speed of the animation
  }






void watermatrix(CRGB* leds) { // random switching blocks
  int n1 = random(0, NUM_LEDS);  // Select random pixel
  int n2 = random(0, NUM_LEDS);  // Select second random pixel
  CRGB temp = leds[n1];  // Store first pixel
  leds[n1] = leds[n2];  // Assign second pixel to first
  leds[n2] = temp;      // Assign first pixel to second
}
void juggle(CRGB* leds) {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

///////////////////////////////////////////////////////////////////////////
// 1) Swirl - GPT Generated
//    Creates concentric color "rings" that rotate over time around the center.
///////////////////////////////////////////////////////////////////////////
void swirl(CRGB* leds) {
  static uint16_t angle = 0;          // Controls rotation over time
  angle += 2;                         // Increase angle to make swirl move

  const uint8_t centerX = 8;          // For a 16x16 matrix, middle is (8,8)
  const uint8_t centerY = 8;

  // Fill the matrix with swirl pattern
  for(uint8_t y = 0; y < 16; y++) {
    for(uint8_t x = 0; x < 16; x++) {
      int16_t dx = x - centerX;
      int16_t dy = y - centerY;
      uint16_t dist = sqrt16(dx * dx + dy * dy); // integer-based distance
      uint8_t hue = (dist * 8 + angle) & 0xFF;   // swirl calculation
      leds[ XY(x, y) ] = CHSV(hue, 255, 255);
    }
  }

  FastLED.show();
  nap(20); // Adjust delay to control rotation speed
}


///////////////////////////////////////////////////////////////////////////
// Game of Life (replacing "meteorRain")
// Standard Conway's rules, on a 16x16 matrix
///////////////////////////////////////////////////////////////////////////
void meteorRain(CRGB* leds) {
  // In this version, 'meteorRain' is replaced by 'Game of Life' logic.
  // Now with a reset every 60 seconds.

  static bool initialized = false;
  static bool board[16][16];
  static bool nextBoard[16][16];

  // Initialize board randomly once
  if(!initialized) {
    initialized = true;
    for(uint8_t y = 0; y < 16; y++){
      for(uint8_t x = 0; x < 16; x++){
        // ~1/3 chance alive
        board[y][x] = (random8() < 85); 
      }
    }
  }

  // Clear nextBoard before computing next generation
  for(uint8_t y = 0; y < 16; y++){
    for(uint8_t x = 0; x < 16; x++){
      nextBoard[y][x] = false;
    }
  }

  // Compute the next generation using Conway's rules
  for(uint8_t y = 0; y < 16; y++){
    for(uint8_t x = 0; x < 16; x++){
      // Count the 8 neighbors
      uint8_t neighbors = 0;
      for(int8_t dy = -1; dy <= 1; dy++){
        for(int8_t dx = -1; dx <= 1; dx++){
          if(dx == 0 && dy == 0) continue; // skip self
          int8_t nx = x + dx;
          int8_t ny = y + dy;
          // Skips out-of-bounds cells
          if(nx >= 0 && nx < 16 && ny >= 0 && ny < 16){
            if(board[ny][nx]) {
              neighbors++;
            }
          }
        }
      }

      // Apply Conway's rules:
      bool alive = board[y][x];
      if(alive && (neighbors == 2 || neighbors == 3)) {
        nextBoard[y][x] = true;
      } else if(!alive && (neighbors == 3)) {
        nextBoard[y][x] = true;
      } else {
        nextBoard[y][x] = false;
      }
    }
  }

  // Copy nextBoard into board
  for(uint8_t y = 0; y < 16; y++){
    for(uint8_t x = 0; x < 16; x++){
      board[y][x] = nextBoard[y][x];
    }
  }

  // Display: alive cells = White, dead cells = Black
  for(uint8_t y = 0; y < 16; y++){
    for(uint8_t x = 0; x < 16; x++){
      if(board[y][x]) {
        leds[ XY(x, y) ] = CRGB::White;
      } else {
        leds[ XY(x, y) ] = CRGB::Black;
      }
    }
  }

  FastLED.show();

  // Delay between generations
  nap(600);

  // --- RESET AFTER 60 SECONDS ---
  EVERY_N_SECONDS(30) {
    // Randomize the board again
    for(uint8_t y = 0; y < 16; y++){
      for(uint8_t x = 0; x < 16; x++){
        board[y][x] = (random8() < 85); 
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////
// 3) Color Wipe
//    Lights up pixels one by one, then turns them off, repeatedly.
//    Randomly picks a new color each cycle.
///////////////////////////////////////////////////////////////////////////
void colorWipe(CRGB* leds) {
  static uint8_t currentPixel = 0;
  static CRGB currentColor = CRGB::Red;  // Start color

  // Light up the current pixel
  leds[currentPixel] = currentColor;

  FastLED.show();
  nap(30); // Pause between wiping each pixel

  // Move to the next pixel
  currentPixel++;
  if(currentPixel >= NUM_LEDS) {
    // Once we've lit them all, turn everything off again
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    nap(300);  // A brief pause with all off

    // Reset to the first pixel
    currentPixel = 0;
    // Pick a new random color
    currentColor = CHSV(random8(), 255, 255);
  }
}

// Random pattern that changes every 1 minute
void randomPattern(CRGB* leds) {
  static unsigned long lastPatternChange = 0;
  static int currentPatternIndex = -1;
  static const char* excludedPatterns[] = {"Draw", "Video", "Type", "Random"};
  static const size_t excludedCount = sizeof(excludedPatterns) / sizeof(excludedPatterns[0]);
  
  unsigned long currentMillis = millis();
  
  // Check if it's time to change patterns (1 minute = 60000 milliseconds)
  if (currentMillis - lastPatternChange > 60000 || currentPatternIndex == -1) {
    // Save the time we changed patterns
    lastPatternChange = currentMillis;
    
    // Choose a random pattern that's not in the excluded list
    bool validPattern = false;
    while (!validPattern) {
      // Pick a random pattern
      int newPatternIndex = random(PATTERN_COUNT);
      
      // Skip if it's the same as the current one
      if (newPatternIndex == currentPatternIndex) {
        continue;
      }
      
      // Check if this pattern is excluded
      bool isExcluded = false;
      for (size_t i = 0; i < excludedCount; i++) {
        if (strcmp(g_patternList[newPatternIndex].name, excludedPatterns[i]) == 0) {
          isExcluded = true;
          break;
        }
      }
      
      // If not excluded, use this pattern
      if (!isExcluded) {
        currentPatternIndex = newPatternIndex;
        validPattern = true;
      }
    }
  }
  
  // Run the selected pattern
  if (currentPatternIndex >= 0 && currentPatternIndex < PATTERN_COUNT) {
    g_patternList[currentPatternIndex].func(leds);
  }
}

void sparkler(CRGB* leds) {
    static float originX = 8.0;
    static float originY = 8.0;
    static float moveAngle = 0;
    
    // Fade existing pixels for trail effect
    fadeToBlackBy(leds, NUM_LEDS, 60);
    
    // Move origin point in a slow circular pattern
    float moveRadius = 4.0;
    float targetX = 8 + cos(moveAngle) * moveRadius;
    float targetY = 8 + sin(moveAngle) * moveRadius;
    
    // Smooth origin movement
    originX += (targetX - originX) * 0.01;
    originY += (targetY - originY) * 0.01;
    moveAngle += 0.005 * (g_Speed / 128.0);
    
    // Generate new sparks each frame
    uint8_t numNewSparks = random8(3, 8);  // Random number of new sparks per frame
    
    // Static array to store active sparks
    static struct {
        float x;
        float y;
        float velX;
        float velY;
        uint8_t hue;
        uint8_t life;
        bool active;
    } sparks[50];  // Maximum 50 active sparks
    
    // Update existing sparks
    for (int i = 0; i < 50; i++) {
        if (sparks[i].active) {
            // Update position based on velocity
            sparks[i].x += sparks[i].velX;
            sparks[i].y += sparks[i].velY;
            
            // Decrease life
            if (sparks[i].life > 0) sparks[i].life--;
            else sparks[i].active = false;
            
            // Check if spark is still in bounds
            if (sparks[i].x < 0 || sparks[i].x >= 16 || sparks[i].y < 0 || sparks[i].y >= 16) {
                sparks[i].active = false;
                continue;
            }
            
            // Draw the spark
            uint8_t brightness = map(sparks[i].life, 0, 255, 0, 255);
            leds[XY((uint8_t)sparks[i].x, (uint8_t)sparks[i].y)] += CHSV(sparks[i].hue, 255, brightness);
            
            // Add subtle glow to neighbors if spark is bright enough
            if (brightness > 127) {
                for (int8_t dx = -1; dx <= 1; dx++) {
                    for (int8_t dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int8_t newX = (uint8_t)sparks[i].x + dx;
                        int8_t newY = (uint8_t)sparks[i].y + dy;
                        
                        if (newX >= 0 && newX < 16 && newY >= 0 && newY < 16) {
                            leds[XY(newX, newY)] += CHSV(sparks[i].hue, 255, brightness/3);
                        }
                    }
                }
            }
        }
    }
    
    // Create new sparks
    for (int i = 0; i < numNewSparks; i++) {
        // Find an inactive spark slot
        for (int j = 0; j < 50; j++) {
            if (!sparks[j].active) {
                // Initialize new spark
                sparks[j].x = originX;
                sparks[j].y = originY;
                
                // Random angle and speed
                float angle = random(256) * (TWO_PI / 256.0);  // Random angle in radians
                float speed = 0.2 + (random(100) / 50.0);      // Random speed between 0.2 and 2.2
                
                // Calculate velocities
                sparks[j].velX = cos(angle) * speed;
                sparks[j].velY = sin(angle) * speed;
                
                // Random color and life
                sparks[j].hue = random8();
                sparks[j].life = random8(100, 255);
                sparks[j].active = true;
                break;
            }
        }
    }
    
    FastLED.show();
    nap(5);
}