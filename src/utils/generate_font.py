#!/usr/bin/env python3

import sys
from PIL import Image

# In this example, we assume the alphabet is laid out horizontally,
# and that each character can be found by scanning for columns
# that are entirely "blank" (white) to separate letters.
# You will need to adapt this logic to match how your image is arranged.

def is_column_blank(img, x, top, bottom, threshold=250):
    """
    Check if a vertical column in the image is "blank" (near white).
    `threshold` is the grayscale threshold to consider a pixel as blank.
    """
    for y in range(top, bottom):
        if img.getpixel((x, y)) < threshold:
            return False
    return True

def extract_letters(img):
    """
    Extract bounding boxes for each 'letter' by finding blank columns
    that separate letters. Returns a list of (char_image, width) tuples.
    
    NOTE: This is a simple approach for a horizontally laid-out font.
          If your image is arranged differently, you'll need different logic.
    """

    # Convert to grayscale just in case it isn't already
    gray = img.convert("L")
    width, height = gray.size

    # We find the top and bottom by scanning for where actual pixels appear
    # or assume the entire height is used:
    top, bottom = 0, height

    # We’ll keep track of columns that are non-blank
    letters = []
    
    # Suppose you already know the letter order, or you'll define it here:
    # If your image is strictly A-Z in order, you can do:
    alphabet_order = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.!?:;"  # or with more chars if needed

    # We'll scan left-to-right to locate bounding boxes:
    letter_boundaries = []
    in_letter = False
    start_x = 0
    alpha_index = 0  # index in the alphabetical_order

    for x in range(width):
        blank = is_column_blank(gray, x, top, bottom)
        if not blank and not in_letter:
            # found start of a letter
            in_letter = True
            start_x = x
        elif blank and in_letter:
            # found end of a letter
            in_letter = False
            # store bounding box
            letter_boundaries.append((start_x, x))
            # If we already reached last letter we care about, break
            alpha_index += 1
            if alpha_index >= len(alphabet_order):
                break

    # if the image does not have as many blank columns, we may
    # have a trailing letter at the end
    if in_letter:
        letter_boundaries.append((start_x, width))

    # Now extract each letter image
    extracted_letters = []
    for i, (l, r) in enumerate(letter_boundaries):
        # Crop letter from l..r in X dimension, top..bottom in Y dimension
        letter_img = gray.crop((l, top, r, bottom))
        extracted_letters.append(letter_img)

    return extracted_letters

def image_to_byte_array(img):
    """
    Convert the provided grayscale PIL Image into a flat list of 8-bit values.
    Top-left to bottom-right (row-major order).
    """
    width, height = img.size
    pixels = list(img.getdata())
    # `pixels` is a list of grayscale values, each 0..255
    return pixels

def generate_cpp(alphabet_images, alphabet_order, output_file):
    """
    Generate a C++ source file with static const uint8_t arrays and
    a helper function to retrieve a letter’s data and width.
    """

    # We assume each image in `alphabet_images` corresponds to the
    # letter in `alphabet_order` at the same index.
    # If that’s not the case in your logic, adjust accordingly.
    
    with open(output_file, 'w') as f:
        f.write('// Auto-generated font data\n')
        f.write('#include <stdint.h>\n\n')

        # We'll keep track of widths and pointer names for the final helper.
        letter_info = []

        for i, letter_img in enumerate(alphabet_images):
            letter_char = alphabet_order[i]
            letter_name = f'Letter_{letter_char}'
            width, height = letter_img.size

            # Flatten pixel data
            pixel_data = image_to_byte_array(letter_img)

            # Write out the array
            f.write(f'// Letter: {letter_char}\n')
            f.write(f'static const uint8_t {letter_name}[] = {{\n    ')

            # We can format 12 bytes per line, for example
            for idx, val in enumerate(pixel_data):
                # clamp to 0..255 just in case
                v = max(0, min(val, 255))
                f.write(f'{v:3d}, ')
                if (idx + 1) % 12 == 0 and (idx + 1) < len(pixel_data):
                    f.write('\n    ')
            f.write('\n};\n\n')

            # Keep track in a list
            letter_info.append((letter_char, letter_name, width, height))

        # Now write out helper function data
        # For example, we can create arrays that map letter => pointer, letter => width, etc.
        f.write('struct LetterData {\n')
        f.write('    const uint8_t* data;\n')
        f.write('    uint16_t width;\n')
        f.write('    uint16_t height;\n')
        f.write('};\n\n')

        # We’ll create a simple table of LetterData. For unknown chars, we can default to '?' or so.
        f.write('static LetterData fontTable[128]; // we only fill A-Z, for example\n\n')

        f.write('static void initFontTable() {\n')
        for (ch, arr_name, w, h) in letter_info:
            ascii_val = ord(ch)
            f.write(f'    fontTable[{ascii_val}].data = {arr_name};\n')
            f.write(f'    fontTable[{ascii_val}].width = {w};\n')
            f.write(f'    fontTable[{ascii_val}].height = {h};\n')
        f.write('}\n\n')

        # Provide a function to retrieve letter data
        f.write('const LetterData& getLetterData(char c)\n')
        f.write('{\n')
        f.write('    // If you haven’t called initFontTable, you might want to do it lazily.\n')
        f.write('    // For a real implementation, consider a static bool or other approach.\n')
        f.write('    static bool initialized = false;\n')
        f.write('    if (!initialized) {\n')
        f.write('        initFontTable();\n')
        f.write('        initialized = true;\n')
        f.write('    }\n')
        f.write('    // Return the fontTable entry if in range, else fallback.\n')
        f.write('    unsigned char uc = (unsigned char)c;\n')
        f.write('    return fontTable[uc];\n')
        f.write('}\n')

    print(f"Generated C++ file: {output_file}")


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} input_image output_cpp [alphabet_string]")
        sys.exit(1)

    input_image_path = sys.argv[1]
    output_cpp = sys.argv[2]

    # Optionally override the alphabet order on the command line
    if len(sys.argv) == 4:
        alphabet_str = sys.argv[3]
    else:
        # Default to uppercase A-Z
        alphabet_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    # Load the image
    img = Image.open(input_image_path)

    # Extract images for each letter
    # (In practice, you'll probably want to pass the alphabet_str to
    #  extract_letters if you want a more strict letter-per-boundary approach.)
    extracted = extract_letters(img)

    # Ensure the number of extracted letters matches your alphabet order
    if len(extracted) != len(alphabet_str):
        print("WARNING: The number of extracted letters does not match the alphabet string size!")
        print("You may need to adjust your extraction logic or your alphabet string.")
        print(f"Extracted: {len(extracted)}, Alphabet: {len(alphabet_str)}")

    # Generate the C++ file
    generate_cpp(extracted, alphabet_str, output_cpp)


if __name__ == "__main__":
    main()