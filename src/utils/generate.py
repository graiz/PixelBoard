from PIL import Image
import os

def image_to_hex_array(image_path, output_file):
    try:
        img = Image.open(image_path)
        width, height = img.size
        frames = width // 32
        img_data = img.getdata()
        num_channels = len(img.getbands())

        with open(output_file, "w") as f:
            for frame in range(frames):
                f.write(f"const long wa{frame + 1}[] PROGMEM = {{")
                for row in range(32):
                    for col in range(32):
                        index = row * width + col + frame * 32
                        pixel = img_data[index]
                        if num_channels == 4:
                            r, g, b, a = pixel
                        elif num_channels == 3:
                            r, g, b = pixel
                        elif num_channels == 1:
                            r = g = b = pixel
                        hex_value = (r << 16) + (g << 8) + b
                        f.write(f"0x{hex_value:06X}, ")
                    if row % 2 == 0:
                        f.write("\n")
                f.write("};\n\n")
    except Exception as e:
        print(f"Error: {e}")

image_path = "/Users/graiz/Downloads/Explosion.png"
output_file = "output.txt"

if os.path.exists(image_path):
    image_to_hex_array(image_path, output_file)
    print("Output file generated successfully.")
else:
    print("Image file not found.")
