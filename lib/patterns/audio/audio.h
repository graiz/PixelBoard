#ifndef AUDIO_H
#define AUDIO_H

#include <FastLED.h>
#include <arduinoFFT.h>
#include <ESPAsyncWebServer.h>
#include "led_display.h"

#define SAMPLES 1024          // Must be a power of 2
#define SAMPLING_FREQ 40000   // Hz, must be 40000 or less due to ADC conversion time
#define MIC_PIN 34           // Signal in on this pin
#define NUM_BANDS 16         // Number of frequency bands
#define TOP 16              // Maximum height of bars

// Function declarations
void setupAudio();
void setupAudioPattern(AsyncWebServer* server);
void audio(CRGB* leds);

// Pattern-specific functions
void audioRainbowBars(CRGB* leds, int band, int barHeight);
void audioPurpleBars(CRGB* leds, int band, int barHeight);
void audioCenterBars(CRGB* leds, int band, int barHeight);
void audioChangingBars(CRGB* leds, int band, int barHeight);
void audioWaterfall(CRGB* leds, int band);
void audioWhitePeak(CRGB* leds, int band, int peakHeight);
void audioOutrunPeak(CRGB* leds, int band, int peakHeight);

#endif // AUDIO_H 