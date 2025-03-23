#ifndef FONT_TEST_H
#define FONT_TEST_H

#include <stdint.h>

// Define letter data struct
typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
} LetterData;

// Declare letter arrays
extern const uint8_t Letter_A[];
// Add declarations for other letters as needed...

// Function to get letter data for a character
const LetterData& getLetterData(char c);

// Function to initialize the font table
void initFontTable();

#endif // FONT_TEST_H 