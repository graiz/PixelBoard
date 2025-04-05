#include <Arduino.h>
#include <FastLED.h>
#include <led_display.h>
#include "WifiServer.h"
#include <Preferences.h>
#include <patterns.h>  
#include "SPIFFS.h"
#include <esp_sleep.h>
#include <WiFi.h>

#define DATA_PIN 26
#define NUM_LEDS 256
#define COLOR_ORDER GRB
#define BUTTON_PIN 0  // Built-in boot button on most ESP32 dev boards

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

void clearWiFiCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  Serial.println("WiFi credentials cleared!");
  ESP.restart();
}

void setup() {
  Serial.begin(460800);
  
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Woke up from timer");
      break;
      
    case ESP_SLEEP_WAKEUP_WIFI:
      Serial.println("Woke up from WiFi event");
      // No need to reconnect WiFi as we're using light sleep
      break;
      
    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
      Serial.println("Normal boot");
      break;
  }
  
  // Load preferences regardless of wake-up reason
  loadPrefs();
  
  // Configure the LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);

  // Start with display off
  FastLED.setBrightness(0);
  FastLED.show();

  // Setup WiFi and server if this is a normal boot or we need to reconnect
  if (wakeup_reason != ESP_SLEEP_WAKEUP_WIFI) {
    wifiServerSetup();  // connect to WiFi, start server, etc.
    nap(2000); // Wait for WiFi to stabilize
  }

  // Set normal brightness
  FastLED.setBrightness(g_Brightness);

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  // Check if button is pressed during boot
  pinMode(BUTTON_PIN, INPUT);
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(3000);  // Wait to confirm it's not a glitch
    if (digitalRead(BUTTON_PIN) == LOW) {
      clearWiFiCredentials();
    }
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

  // Check for serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "clearwifi") {
      clearWiFiCredentials();
    }
  }

  nap(1);
}