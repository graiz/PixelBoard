#ifndef TYPE_H
#define TYPE_H

#include <Arduino.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <led_display.h>

// Define structures for the Adafruit GFX font format
/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
  const uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  const GFXglyph *glyph;  ///< Glyph array
  uint8_t first;    ///< ASCII extents (first char)
  uint8_t last;     ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

// Forward declare the font from freemono.h
extern const GFXglyph FreeMono24pt7bGlyphs[] PROGMEM;
extern const uint8_t FreeMono24pt7bBitmaps[] PROGMEM;
extern const GFXfont FreeMono24pt7b PROGMEM;

// Function declarations for the type pattern
void type(CRGB* leds);
void setupTypePattern(AsyncWebServer* server);

#endif 