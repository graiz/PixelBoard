// Font test implementation
#include <stdint.h>
#include "font_test.h"

// ==========================================================================
// LETTER DATA DEFINITIONS
// ==========================================================================
// Each letter is defined as an array of uint8_t values representing pixel data
// The data is structured in rows of 12 bytes per row (representing a 12x24 pixel grid)
// Each byte represents the intensity of a pixel (0-255), with 0 being transparent

// Letter: A
const uint8_t Letter_A[] = {
    // This is a stub for the 'A' letter bitmap - 12x24 pixel grid
    // Row 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Row 2
    0, 0, 0, 0, 0, 17, 17, 0, 0, 0, 0, 0,
    // Row 3
    0, 0, 0, 0, 17, 85, 85, 17, 0, 0, 0, 0,
    // Row 4
    0, 0, 0, 17, 85, 0, 0, 85, 17, 0, 0, 0,
    // Row 5
    0, 0, 0, 17, 85, 0, 0, 85, 17, 0, 0, 0,
    // Row 6
    0, 0, 17, 85, 85, 85, 85, 85, 85, 17, 0, 0,
    // Row 7
    0, 0, 17, 85, 0, 0, 0, 0, 85, 17, 0, 0,
    // Row 8
    0, 0, 17, 85, 0, 0, 0, 0, 85, 17, 0, 0,
    // Remaining rows (9-24) as empty space
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Letter: B
const uint8_t Letter_B[] = {
    // Row 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Row 2
    0, 0, 85, 85, 85, 85, 17, 0, 0, 0, 0, 0,
    // Row 3
    0, 0, 85, 0, 0, 0, 85, 17, 0, 0, 0, 0,
    // Row 4
    0, 0, 85, 0, 0, 0, 85, 17, 0, 0, 0, 0,
    // Row 5
    0, 0, 85, 85, 85, 85, 17, 0, 0, 0, 0, 0,
    // Row 6
    0, 0, 85, 0, 0, 0, 85, 17, 0, 0, 0, 0,
    // Row 7
    0, 0, 85, 0, 0, 0, 85, 17, 0, 0, 0, 0,
    // Row 8
    0, 0, 85, 85, 85, 85, 17, 0, 0, 0, 0, 0,
    // Remaining rows (9-24) as empty space
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Letter: O
const uint8_t Letter_O[] = {
    // Row 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Row 2
    0, 0, 0, 85, 85, 85, 85, 17, 0, 0, 0, 0,
    // Row 3
    0, 0, 85, 0, 0, 0, 0, 85, 17, 0, 0, 0,
    // Row 4
    0, 0, 85, 0, 0, 0, 0, 85, 17, 0, 0, 0,
    // Row 5
    0, 0, 85, 0, 0, 0, 0, 85, 17, 0, 0, 0,
    // Row 6
    0, 0, 85, 0, 0, 0, 0, 85, 17, 0, 0, 0,
    // Row 7
    0, 0, 85, 0, 0, 0, 0, 85, 17, 0, 0, 0,
    // Row 8
    0, 0, 0, 85, 85, 85, 85, 17, 0, 0, 0, 0,
    // Remaining rows (9-24) as empty space
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// ==========================================================================
// FONT TABLE IMPLEMENTATION
// ==========================================================================

// Static array to hold letter data for all ASCII characters (0-127)
static LetterData fontTable[128];
static bool fontInitialized = false;

// Initialize the font table with letter data
void initFontTable() {
    // If already initialized, don't reinitialize
    if (fontInitialized) return;
    
    // Initialize all entries with null data first
    for (int i = 0; i < 128; i++) {
        fontTable[i].data = nullptr;
        fontTable[i].width = 0;
        fontTable[i].height = 0;
    }
    
    // Set up letter A (ASCII 65)
    fontTable[65].data = Letter_A;
    fontTable[65].width = 12;
    fontTable[65].height = 24;
    
    // Set up letter B (ASCII 66)
    fontTable[66].data = Letter_B;
    fontTable[66].width = 12;
    fontTable[66].height = 24;
    
    // Set up letter O (ASCII 79)
    fontTable[79].data = Letter_O;
    fontTable[79].width = 12;
    fontTable[79].height = 24;
    
    // Mark as initialized
    fontInitialized = true;
}

// Get letter data for a character
const LetterData& getLetterData(char c) {
    // Convert lowercase to uppercase for simplicity
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }
    
    // Initialize font table if needed
    if (!fontInitialized) {
        initFontTable();
    }
    
    // Return the fontTable entry if in range and initialized
    if (c >= 0 && c < 128 && fontTable[(int)c].data != nullptr) {
        return fontTable[(int)c];
    }
    
    // Fallback to 'A' if available, or an empty letter if not
    static LetterData emptyLetter = {nullptr, 0, 0};
    
    if (fontTable[65].data != nullptr) {
        return fontTable[65];
    }
    
    return emptyLetter;
}

// ==========================================================================
// DATA STRUCTURE NOTES
// ==========================================================================
/*
To add a new letter to the font table:

1. Define the letter's pixel data as a uint8_t array
   - Each array should be organized as rows of 12 bytes
   - Each byte represents a pixel intensity (0-255)
   - 0 means transparent, higher values are more opaque
   - The standard size is 12×24 pixels (12 bytes × 24 rows)

2. Register the letter in the initFontTable() function:
   - Set the fontTable entry for the letter's ASCII code
   - Assign the data pointer, width, and height

Example for adding the letter 'B':
   
const uint8_t Letter_B[] = {
    // 12×24 pixel grid of intensity values (12 bytes per row)
    // ...
};

// In initFontTable():
fontTable[66].data = Letter_B;
fontTable[66].width = 12;
fontTable[66].height = 24;
*/
