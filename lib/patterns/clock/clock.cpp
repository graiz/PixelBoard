#include "clock.h"
#include <led_display.h>
#include <patterns.h>
#include <FastLED.h>

// External variables
extern int g_Speed;  // Add g_Speed extern declaration

// Global variables for clock control
static uint8_t g_secondCount = 0;
static uint8_t g_minuteCount = 0;
static uint16_t g_totalSeconds = 1500; // Default 25 minutes in seconds
static bool g_isPaused = true;  // Start paused
static bool g_isFirstTime = true;
static unsigned long g_lastUpdate = 0;  // Track last update time
static CRGB g_minuteColor = CRGB::Blue;
static CRGB g_secondColor = CRGB::Red;

// Function to get outer edge pixel coordinates
void getOuterEdgePixel(uint8_t position, uint8_t& x, uint8_t& y) {
    const uint8_t EDGE = 15;  // 16x16 matrix, so edge is at 15
    const uint8_t CENTER = 8; // Center position for top edge
    
    // Offset position by 8 to start at center top (12 o'clock)
    position = (position + 8) % 60;
    
    if (position < 16) {          // Top edge
        x = position;
        y = 0;
    } else if (position < 31) {   // Right edge
        x = EDGE;
        y = position - 15;
    } else if (position < 46) {   // Bottom edge
        x = EDGE - (position - 30);
        y = EDGE;
    } else {                      // Left edge
        x = 0;
        y = EDGE - (position - 45);
    }
}

void resetClock() {
    g_secondCount = 0;
    g_minuteCount = 0;
    g_isFirstTime = true;
    g_isPaused = true;
    g_lastUpdate = 0;
}

void clockCountdown(CRGB* leds) {
    // If it's our first time running, reset the counters
    if (g_isFirstTime) {
        g_isFirstTime = false;
        g_secondCount = 0;
        g_minuteCount = 0;
        g_lastUpdate = millis();
    }

    // Update time if not paused
    if (!g_isPaused) {
        unsigned long currentMillis = millis();
        if (currentMillis - g_lastUpdate >= 1000) {  // Check if a second has passed
            g_lastUpdate = currentMillis;  // Update last update time
            
            // Only update if we haven't reached the end
            uint16_t currentTotalSeconds = (g_minuteCount * 60) + g_secondCount;
            if (currentTotalSeconds < g_totalSeconds) {
                g_secondCount++;
                if (g_secondCount >= 60) {
                    g_secondCount = 0;
                    g_minuteCount++;
                }
            }
        }
    }

    // Calculate current total seconds and progress
    uint16_t currentTotalSeconds = (g_minuteCount * 60) + g_secondCount;
    float progressPercent = (float)currentTotalSeconds / (float)g_totalSeconds;

    // Clear all LEDs first
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Draw the progress wedge
    for(uint8_t y = 0; y < 16; y++) {
        for(uint8_t x = 0; x < 16; x++) {
            int16_t dx = x - 8;  // Center is at (8,8) for 16x16 matrix
            int16_t dy = y - 8;
            
            // Calculate angle in degrees [0..360), starting from 12 o'clock
            float angle = atan2f((float)dy, (float)dx) * (180.0f / 3.14159f);
            angle += 90.0f;  // Shift 0° to 12 o'clock position
            if(angle < 0) {
                angle += 360.0f;  // Keep angle in [0..360) range
            }
            
            // The progress wedge extends based on total completion
            float progressAngle = progressPercent * 360.0f;

            if(angle <= progressAngle) {
                // Color the progress wedge with a rainbow effect
                uint8_t hue = map(angle, 0, 360, 0, 255);
                leds[XY(x, y)] = CHSV(hue, 255, 255);
            }
        }
    }

    // Add the second marker dot if timer is over 4 minutes
    if (g_totalSeconds > 240) {  // 4 minutes = 240 seconds
        // Calculate position along the outer edge (60 positions for 60 seconds)
        uint8_t position = g_secondCount;  // 0-59 maps to 60 positions
        // Scale to our 60 outer edge positions (0-59)
        position = map(position, 0, 60, 0, 60);
        
        // Get the x,y coordinates for this position
        uint8_t x, y;
        getOuterEdgePixel(position, x, y);
        
        // Draw the white dot
        leds[XY(x, y)] = CRGB::White;
    }

    // Show the updated frame
    FastLED.show();
    nap(20);
}

void setupClockPattern(AsyncWebServer* server) {
    server->on("/clock", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>PixelBoard Clock Countdown</title>
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
            gap: 15px;
            align-items: center;
            justify-content: center;
            flex-wrap: wrap;
        }
        .tool-group {
            display: flex;
            gap: 10px;
            align-items: center;
            padding-right: 15px;
            border-right: 1px solid #61dafb;
        }
        .tool-group:last-child {
            border-right: none;
            padding-right: 0;
        }
        .btn {
            background-color: #282c34;
            color: #61dafb;
            border: 1px solid #61dafb;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.2s;
            height: 32px;
            display: flex;
            align-items: center;
            justify-content: center;
            min-width: 80px;
        }
        .btn:hover {
            background-color: #61dafb;
            color: #282c34;
        }
        .btn.active {
            background-color: #61dafb;
            color: #282c34;
        }
        input[type="number"] {
            background: #282c34;
            border: 1px solid #61dafb;
            color: #ffffff;
            padding: 4px 8px;
            border-radius: 4px;
            width: 60px;
            text-align: center;
            height: 24px;
        }
        input[type="number"]:focus {
            outline: none;
            border-color: #61dafb;
            box-shadow: 0 0 0 2px rgba(97, 218, 251, 0.2);
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
        }
        .pixel {
            aspect-ratio: 1;
            background-color: #282c34;
            border-radius: 2px;
            transition: background-color 0.3s ease;
        }
        .timer-display {
            font-size: 24px;
            font-weight: bold;
            color: #61dafb;
            margin: 10px 0;
            font-family: monospace;
        }
    </style>
</head>
<body>
    <div class="toolbar">
        <div class="tool-group">
            <label>Minutes:</label>
            <input type="number" id="minutesInput" min="0" max="59" value="25">
            <label>Seconds:</label>
            <input type="number" id="secondsInput" min="0" max="59" value="0">
        </div>
        <div class="tool-group">
            <button class="btn" id="startBtn">Start</button>
            <button class="btn" id="pauseBtn">Pause</button>
            <button class="btn" id="resetBtn">Reset</button>
        </div>
    </div>

    <div class="timer-display" id="timerDisplay">25:00</div>
    <div class="grid" id="pixelGrid"></div>

    <script>
        let previewUpdateInterval;
        let timerUpdateInterval;
        
        // Create preview grid
        function createPreviewGrid() {
            const grid = document.getElementById('pixelGrid');
            for (let i = 0; i < 256; i++) {
                const pixel = document.createElement('div');
                pixel.className = 'pixel';
                pixel.id = 'pixel-' + i;
                grid.appendChild(pixel);
            }
        }
        
        // Update the preview grid
        function updatePreview() {
            fetch('/pixelStatus')
                .then(response => response.arrayBuffer())
                .then(buffer => {
                    const pixels = new Uint8Array(buffer);
                    for (let i = 0; i < 256; i++) {
                        const baseIndex = i * 3;
                        const r = pixels[baseIndex];
                        const g = pixels[baseIndex + 1];
                        const b = pixels[baseIndex + 2];
                        
                        const pixelElement = document.getElementById('pixel-' + i);
                        if (pixelElement) {
                            pixelElement.style.backgroundColor = `rgb(${r},${g},${b})`;
                        }
                    }
                })
                .catch(error => console.error('Error updating preview:', error));
        }
        
        // Update timer display
        function updateTimerDisplay() {
            fetch('/clockstatus')
                .then(response => response.json())
                .then(data => {
                    const minutes = String(data.minutes).padStart(2, '0');
                    const seconds = String(data.seconds).padStart(2, '0');
                    document.getElementById('timerDisplay').textContent = `${minutes}:${seconds}`;
                    
                    // Update button states based on pause status
                    const startBtn = document.getElementById('startBtn');
                    const pauseBtn = document.getElementById('pauseBtn');
                    if (data.paused) {
                        startBtn.classList.remove('active');
                        pauseBtn.classList.add('active');
                    } else {
                        startBtn.classList.add('active');
                        pauseBtn.classList.remove('active');
                    }
                })
                .catch(error => console.error('Error updating timer:', error));
        }
        
        document.addEventListener('DOMContentLoaded', function() {
            const startBtn = document.getElementById('startBtn');
            const pauseBtn = document.getElementById('pauseBtn');
            const resetBtn = document.getElementById('resetBtn');
            const minutesInput = document.getElementById('minutesInput');
            const secondsInput = document.getElementById('secondsInput');
            
            startBtn.addEventListener('click', function() {
                const minutes = parseInt(minutesInput.value) || 0;
                const seconds = parseInt(secondsInput.value) || 0;
                const totalSeconds = (minutes * 60) + seconds;
                if (totalSeconds > 0) {
                    fetch(`/clockcontrol?action=start&minutes=${minutes}&seconds=${seconds}`);
                }
            });
            
            pauseBtn.addEventListener('click', function() {
                fetch('/clockcontrol?action=pause');
            });
            
            resetBtn.addEventListener('click', function() {
                fetch('/clockcontrol?action=reset');
            });
            
            minutesInput.addEventListener('change', function() {
                if (this.value < 0) this.value = 0;
                if (this.value > 59) this.value = 59;
            });
            
            secondsInput.addEventListener('change', function() {
                if (this.value < 0) this.value = 0;
                if (this.value > 59) this.value = 59;
            });
            
            // Initialize
            createPreviewGrid();
            updatePreview();
            updateTimerDisplay();
            previewUpdateInterval = setInterval(updatePreview, 100);
            timerUpdateInterval = setInterval(updateTimerDisplay, 1000);
        });
        
        // Clean up
        window.addEventListener('unload', function() {
            if (previewUpdateInterval) clearInterval(previewUpdateInterval);
            if (timerUpdateInterval) clearInterval(timerUpdateInterval);
        });
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", html);
    });

    // Endpoint to get current clock status
    server->on("/clockstatus", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"minutes\":" + String(g_minuteCount) + 
                     ",\"seconds\":" + String(g_secondCount) + 
                     ",\"total_minutes\":" + String(g_totalSeconds / 60) + 
                     ",\"paused\":" + String(g_isPaused ? "true" : "false") + "}";
        request->send(200, "application/json", json);
    });

    // Endpoint to control the clock
    server->on("/clockcontrol", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("action")) {
            String action = request->getParam("action")->value();
            
            if (action == "start") {
                if (request->hasParam("minutes") && request->hasParam("seconds")) {
                    int minutes = request->getParam("minutes")->value().toInt();
                    int seconds = request->getParam("seconds")->value().toInt();
                    g_totalSeconds = (minutes * 60) + seconds;
                    g_secondCount = 0;
                    g_minuteCount = 0;
                    g_isFirstTime = true;
                    g_lastUpdate = millis();
                }
                g_isPaused = false;
            } else if (action == "pause") {
                g_isPaused = !g_isPaused;
                if (!g_isPaused) {
                    g_lastUpdate = millis();  // Reset the timer reference point when unpausing
                }
            } else if (action == "reset") {
                resetClock();
            }
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing action parameter");
        }
    });
} 