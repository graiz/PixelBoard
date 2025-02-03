#include <Arduino.h>
#include <FastLED.h>
#include <led_display.h>
#include "WifiServer.h"
#include <Preferences.h>
#include <patterns.h>  
#include "SPIFFS.h"

#define DATA_PIN 26
#define NUM_LEDS 256
#define COLOR_ORDER GRB

Preferences preferences;

uint8_t g_hue = 0; 
CRGB leds[NUM_LEDS];
int g_Brightness = 96;
int g_Speed = 120;
uint8_t g_current_pattern_number = 0;

// The single place where patterns + names are stored (patterns_index.h/.cpp):
extern Pattern g_patternList[];          // [ "Fire", firefunction ], ...
extern const size_t PATTERN_COUNT;       // number of patterns

void savePrefs() {
   static int last_pattern_number = -1;
   if (g_current_pattern_number != last_pattern_number) { 
      preferences.begin("pixelboard", false);
      preferences.putInt("patternNumber", g_current_pattern_number);
      preferences.putInt("brightness", g_Brightness);
      preferences.putInt("speed", g_Speed);    
      preferences.end();
      last_pattern_number = g_current_pattern_number;
   }
}

void loadPrefs() {
    preferences.begin("pixelboard", false);
    g_current_pattern_number = preferences.getInt("patternNumber", 0);
    g_Brightness = preferences.getInt("brightness", 100);
    g_Speed      = preferences.getInt("speed", 100);
    preferences.end();

    // Ensure pattern index is valid
    if (g_current_pattern_number >= PATTERN_COUNT) {
      g_current_pattern_number = 0;
    }
}

void next_pattern() {
  g_current_pattern_number = (g_current_pattern_number + 1) % PATTERN_COUNT;
  Serial.println(g_current_pattern_number);
}

void setup() {
  Serial.begin(460800);
  loadPrefs();
  
  // 1) Configure the strip first
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);

  // 2) Immediately turn brightness to 0 and show() to ensure it's off
  FastLED.setBrightness(0);
  FastLED.show();   // Now the strip is definitely off

  // 3) Wait while you do WiFi
  wifiServerSetup();  // connect to WiFi, start server, etc.

  // 4) Possibly wait a moment if needed
  nap(2000);

  // 5) After WiFi is stable, set your normal brightness
  FastLED.setBrightness(g_Brightness);

  if(!SPIFFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
}

void loop()
{
  // Call the function pointer from the struct
  g_patternList[g_current_pattern_number].func(leds);

  FastLED.show();
  FastLED.setBrightness(g_Brightness);

  EVERY_N_MILLISECONDS(10) { g_hue++; } // Slowly cycle the base color

  // Periodic saves
  EVERY_N_SECONDS(15) { savePrefs(); }

  nap(1);
}