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
    <style>
        .grid {
            display: grid;
            grid-template-columns: repeat(16, 20px);
            gap: 1px;
            background-color: #333;
            padding: 10px;
            width: fit-content;
            user-select: none;
        }
        .pixel {
            width: 20px;
            height: 20px;
            background-color: #000;
            border: 1px solid #444;
            cursor: pointer;
        }
        .controls {
            margin: 10px 0;
        }
        .color-btn {
            padding: 10px 20px;
            margin: 0 5px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        #white { background-color: #fff; color: #000; }
        #red { background-color: #f00; color: #fff; }
        #green { background-color: #0f0; color: #000; }
        #blue { background-color: #00f; color: #fff; }
        #clear { background-color: #666; color: #fff; }
    </style>
</head>
<body>
    <h1>PixelBoard Draw</h1>
    <div class="controls">
        <button class="color-btn" id="white">White</button>
        <button class="color-btn" id="red">Red</button>
        <button class="color-btn" id="green">Green</button>
        <button class="color-btn" id="blue">Blue</button>
        <button class="color-btn" id="clear">Clear</button>
    </div>
    <div class="grid" id="pixelGrid"></div>

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
        document.getElementById('white').addEventListener('click', () => {
            currentColor = {r: 255, g: 255, b: 255};
        });
        document.getElementById('red').addEventListener('click', () => {
            currentColor = {r: 255, g: 0, b: 0};
        });
        document.getElementById('green').addEventListener('click', () => {
            currentColor = {r: 0, g: 255, b: 0};
        });
        document.getElementById('blue').addEventListener('click', () => {
            currentColor = {r: 0, g: 0, b: 255};
        });
        document.getElementById('clear').addEventListener('click', () => {
            const pixels = document.getElementsByClassName('pixel');
            for(let pixel of pixels) {
                pixel.style.backgroundColor = '#000';
            }
            fetch('/drawclear');
        });
        
        function togglePixel(pixel) {
            const x = pixel.dataset.x;
            const y = pixel.dataset.y;
            pixel.style.backgroundColor = `rgb(${currentColor.r},${currentColor.g},${currentColor.b})`;
            fetch(`/drawpixel?x=${x}&y=${y}&r=${currentColor.r}&g=${currentColor.g}&b=${currentColor.b}`);
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
} 


