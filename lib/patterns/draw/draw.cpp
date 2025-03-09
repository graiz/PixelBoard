#include "draw.h"
#include <led_display.h>

// Add the external reference to g_LEDs
extern CRGB leds[];

// Array to track pixel states (RGB values for each pixel)
static CRGB pixelStates[NUM_LEDS] = {0};

void draw(CRGB* leds) {
    // Update all pixels from our state array
    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = pixelStates[i];
    }
    FastLED.show();
    FastLED.delay(100);
}

void setupDrawPattern(AsyncWebServer* server) {
    server->on("/draw", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>PixelBoard Draw</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #282c34;
            color: #ffffff;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .toolbar {
            width: min(80%, 600px);
            margin-bottom: 20px;
            padding: 15px;
            background: #3b3f47;
            border-radius: 10px;
            border: 1px solid #61dafb;
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            align-items: center;
        }
        .tool-group {
            display: flex;
            gap: 5px;
            align-items: center;
            padding-right: 15px;
            border-right: 1px solid #61dafb;
        }
        .tool-group:last-child {
            border-right: none;
            padding-right: 0;
        }
        .color-btn {
            width: 32px;
            height: 32px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: all 0.2s;
            padding: 0;
            position: relative;
        }
        .color-btn.active {
            transform: scale(1.1);
            box-shadow: 0 0 0 2px #61dafb;
        }
        .color-btn:hover {
            transform: scale(1.1);
        }
        #white { background-color: #fff; }
        #red { background-color: #ff4d4d; }
        #green { background-color: #4dff4d; }
        #blue { background-color: #4d4dff; }
        .action-btn {
            background-color: #282c34;
            color: #61dafb;
            border: 1px solid #61dafb;
            padding: 8px 12px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.2s;
            height: 32px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .action-btn:hover {
            background-color: #61dafb;
            color: #282c34;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(16, 1fr);
            gap: 2px;
            background-color: #3b3f47;
            padding: 20px;
            border-radius: 10px;
            aspect-ratio: 1;
            width: min(80%, 600px);
            box-shadow: 2px 0 5px rgba(0, 0, 0, 0.1);
        }
        .pixel {
            aspect-ratio: 1;
            background-color: #282c34;
            border-radius: 2px;
            cursor: pointer;
            transition: background-color 0.2s ease;
        }
        #imageInput {
            display: none;
        }
        .preview {
            display: none;
            margin: 10px 0;
            border: 1px solid #61dafb;
            border-radius: 4px;
            max-width: 100%;
        }
        .divider {
            width: 1px;
            height: 24px;
            background-color: #61dafb;
            margin: 0 10px;
        }
    </style>
</head>
<body>
    <div class="toolbar">
        <div class="tool-group">
            <button class="color-btn active" id="white" title="White"></button>
            <button class="color-btn" id="red" title="Red"></button>
            <button class="color-btn" id="green" title="Green"></button>
            <button class="color-btn" id="blue" title="Blue"></button>
        </div>
        <div class="tool-group">
            <button class="action-btn" id="clear">Clear</button>
            <label class="action-btn" for="imageInput">Upload Image</label>
            <input type="file" id="imageInput" accept="image/*">
        </div>
    </div>
    
    <div class="grid" id="pixelGrid"></div>
    <canvas id="preview" class="preview" width="160" height="160"></canvas>

    <script>
        const grid = document.getElementById('pixelGrid');
        let isDrawing = false;
        let currentColor = {r: 255, g: 255, b: 255}; // Default white

        // Create 16x16 grid
        for (let y = 0; y < 16; y++) {
            for (let x = 0; x < 16; x++) {
                const pixel = document.createElement('div');
                pixel.className = 'pixel';
                pixel.dataset.x = x;
                pixel.dataset.y = y;
                
                pixel.addEventListener('mousedown', (e) => {
                    isDrawing = true;
                    togglePixel(e.target);
                });
                
                pixel.addEventListener('mouseover', (e) => {
                    if (isDrawing) {
                        togglePixel(e.target);
                    }
                });
                
                grid.appendChild(pixel);
            }
        }
        
        document.addEventListener('mouseup', () => {
            isDrawing = false;
        });

        // Color button handlers
        const colorButtons = document.querySelectorAll('.color-btn');
        colorButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                // Remove active class from all buttons
                colorButtons.forEach(b => b.classList.remove('active'));
                // Add active class to clicked button
                btn.classList.add('active');
                
                switch(btn.id) {
                    case 'white':
                        currentColor = {r: 255, g: 255, b: 255};
                        break;
                    case 'red':
                        currentColor = {r: 255, g: 77, b: 77};
                        break;
                    case 'green':
                        currentColor = {r: 77, g: 255, b: 77};
                        break;
                    case 'blue':
                        currentColor = {r: 77, g: 77, b: 255};
                        break;
                }
            });
        });

        document.getElementById('clear').addEventListener('click', () => {
            const pixels = document.getElementsByClassName('pixel');
            for(let pixel of pixels) {
                pixel.style.backgroundColor = '#282c34';
            }
            fetch('/drawclear');
        });
        
        function togglePixel(pixel) {
            const x = pixel.dataset.x;
            const y = pixel.dataset.y;
            pixel.style.backgroundColor = `rgb(${currentColor.r},${currentColor.g},${currentColor.b})`;
            fetch(`/drawpixel?x=${x}&y=${y}&r=${currentColor.r}&g=${currentColor.g}&b=${currentColor.b}`);
        }

        // Add image handling code
        document.getElementById('imageInput').addEventListener('change', function(e) {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = function(event) {
                    const img = new Image();
                    img.onload = function() {
                        // Get the preview canvas
                        const canvas = document.getElementById('preview');
                        const ctx = canvas.getContext('2d', { willReadFrequently: true });
                        canvas.style.display = 'block';
                        
                        // Clear canvas
                        ctx.clearRect(0, 0, canvas.width, canvas.height);
                        
                        // Calculate scaling to maintain aspect ratio
                        const scale = Math.min(160 / img.width, 160 / img.height);
                        const width = img.width * scale;
                        const height = img.height * scale;
                        
                        // Center the image
                        const x = (160 - width) / 2;
                        const y = (160 - height) / 2;
                        
                        // Draw scaled image
                        ctx.drawImage(img, x, y, width, height);
                        
                        // Sample the image at 16x16 resolution
                        const pixelSize = 10; // 160/16 = 10
                        for(let py = 0; py < 16; py++) {
                            for(let px = 0; px < 16; px++) {
                                const imageData = ctx.getImageData(px * pixelSize + 5, py * pixelSize + 5, 1, 1).data;
                                const r = imageData[0];
                                const g = imageData[1];
                                const b = imageData[2];
                                
                                // Update the grid
                                const pixel = document.querySelector(`.pixel[data-x="${px}"][data-y="${py}"]`);
                                pixel.style.backgroundColor = `rgb(${r},${g},${b})`;
                            }
                        }
                        // Send all pixels at once
                        sendFullImage();
                    };
                    img.src = event.target.result;
                };
                reader.readAsDataURL(file);
            }
        });

        function sendFullImage() {
            const pixels = document.getElementsByClassName('pixel');
            let pixelData = '';
            
            for (let pixel of pixels) {
                const style = getComputedStyle(pixel);
                const rgb = style.backgroundColor.match(/\d+/g);
                // Convert RGB to hex string
                pixelData += (
                    (parseInt(rgb[0]).toString(16).padStart(2, '0')) +
                    (parseInt(rgb[1]).toString(16).padStart(2, '0')) +
                    (parseInt(rgb[2]).toString(16).padStart(2, '0'))
                );
            }
            
            fetch('/drawimage?pixels=' + pixelData);
        }
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", html);
    });

    server->on("/drawclear", HTTP_GET, [](AsyncWebServerRequest *request) {
        fill_solid(pixelStates, NUM_LEDS, CRGB::Black);
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        request->send(200);
    });

    server->on("/drawpixel", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("x") && request->hasParam("y") && 
            request->hasParam("r") && request->hasParam("g") && request->hasParam("b")) {
            int x = request->getParam("x")->value().toInt();
            int y = request->getParam("y")->value().toInt();
            uint8_t r = request->getParam("r")->value().toInt();
            uint8_t g = request->getParam("g")->value().toInt();
            uint8_t b = request->getParam("b")->value().toInt();
            
            if (x >= 0 && x < 16 && y >= 0 && y < 16) {
                int ledIndex = XY(x, y);
                pixelStates[ledIndex] = CRGB(r, g, b);
                leds[ledIndex] = CRGB(r, g, b);
                FastLED.show();
            }
        }
        request->send(200);
    });

    server->on("/drawimage", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("pixels")) {
            String pixelData = request->getParam("pixels")->value();
            int index = 0;
            
            // Process groups of 6 characters (RRGGBB)
            for (int i = 0; i < pixelData.length(); i += 6) {
                if (index >= NUM_LEDS) break;
                
                // Convert hex string to RGB values
                uint32_t rgb = strtoul(pixelData.substring(i, i + 6).c_str(), NULL, 16);
                uint8_t r = (rgb >> 16) & 0xFF;
                uint8_t g = (rgb >> 8) & 0xFF;
                uint8_t b = rgb & 0xFF;
                
                // Update both states and LEDs
                pixelStates[index] = CRGB(r, g, b);
                leds[index] = CRGB(r, g, b);
                index++;
            }
            
            FastLED.show();
        }
        request->send(200);
    });
} 


