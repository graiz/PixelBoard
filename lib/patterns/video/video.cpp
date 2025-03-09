#include "video.h"
#include <led_display.h>

// Add the external reference to g_LEDs
extern CRGB leds[];

// Array to track pixel states (RGB values for each pixel)
static CRGB pixelStates[NUM_LEDS] = {0};

// Frame rate control
static unsigned long lastFrameTime = 0;
int frameDelay = 1000 / 30;  // default 30 fps

void video(CRGB* leds) {
    // Update all pixels from our state array
    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = pixelStates[i];
    }
    FastLED.show();
    FastLED.delay(33); // ~30fps
}

void setupVideoPlayer(AsyncWebServer* server) {
    server->on("/video", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>PixelBoard Video Player</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #222;
            color: #fff;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(16, 20px);
            gap: 1px;
            background-color: #333;
            padding: 10px;
            width: fit-content;
            user-select: none;
            margin: 20px 0;
        }
        .pixel {
            width: 20px;
            height: 20px;
            background-color: #000;
            border: 1px solid #444;
        }
        .controls {
            margin: 20px 0;
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
        }
        button {
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            background-color: #4a90e2;
            color: white;
            font-weight: bold;
        }
        button:disabled {
            background-color: #666;
            cursor: not-allowed;
        }
        #video-container {
            margin: 20px 0;
            display: none;
        }
        video {
            max-width: 100%;
            border: 1px solid #444;
        }
        #preview-canvas {
            border: 1px solid #444;
            margin: 20px 0;
            display: none;
        }
        #status {
            margin: 10px 0;
            padding: 10px;
            background-color: #333;
            border-radius: 5px;
        }
        .slider-container {
            display: flex;
            align-items: center;
            gap: 10px;
            margin: 10px 0;
        }
        input[type="range"] {
            flex-grow: 1;
        }
        input[type="file"] {
            display: block;
            margin: 20px 0;
            padding: 10px;
            background-color: #333;
            border-radius: 5px;
            width: 100%;
            box-sizing: border-box;
        }
        .control-panel {
            background-color: #333;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .control-panel h3 {
            margin-top: 0;
            margin-bottom: 15px;
            color: #4a90e2;
        }
    </style>
</head>
<body>
    <h1>PixelBoard Video Player</h1>
    
    <div id="status">Upload a video file to display on the PixelBoard</div>
    
    <input type="file" id="videoInput" accept="video/*">
    
    <div id="video-container">
        <video id="video" controls></video>
    </div>
    
    <canvas id="preview-canvas" width="160" height="160"></canvas>
    
    <div class="control-panel">
        <h3>Playback Controls</h3>
        <div class="slider-container">
            <span>Speed:</span>
            <input type="range" id="speedSlider" min="1" max="30" value="10">
            <span id="fpsValue">10 FPS</span>
        </div>
        
        <div class="controls">
            <button id="playBtn" disabled>Play on PixelBoard</button>
            <button id="pauseBtn" disabled>Pause</button>
            <button id="stopBtn" disabled>Stop</button>
            <button id="clearBtn">Clear Board</button>
        </div>
    </div>
    
    <div class="control-panel">
        <h3>View Controls</h3>
        <div class="slider-container">
            <span>Zoom:</span>
            <input type="range" id="zoomSlider" min="50" max="300" value="100">
            <span id="zoomValue">100%</span>
        </div>
        
        <div class="slider-container">
            <span>Pan X:</span>
            <input type="range" id="panXSlider" min="-100" max="100" value="0">
            <span id="panXValue">0%</span>
        </div>
        
        <div class="slider-container">
            <span>Pan Y:</span>
            <input type="range" id="panYSlider" min="-100" max="100" value="0">
            <span id="panYValue">0%</span>
        </div>
        
        <button id="resetViewBtn">Reset View</button>
    </div>
    
    <div class="grid" id="pixelGrid"></div>

    <script>
        // DOM elements
        const videoInput = document.getElementById('videoInput');
        const video = document.getElementById('video');
        const videoContainer = document.getElementById('video-container');
        const canvas = document.getElementById('preview-canvas');
        const ctx = canvas.getContext('2d', { willReadFrequently: true });
        const pixelGrid = document.getElementById('pixelGrid');
        const statusEl = document.getElementById('status');
        const playBtn = document.getElementById('playBtn');
        const pauseBtn = document.getElementById('pauseBtn');
        const stopBtn = document.getElementById('stopBtn');
        const clearBtn = document.getElementById('clearBtn');
        const speedSlider = document.getElementById('speedSlider');
        const fpsValue = document.getElementById('fpsValue');
        const zoomSlider = document.getElementById('zoomSlider');
        const zoomValue = document.getElementById('zoomValue');
        const panXSlider = document.getElementById('panXSlider');
        const panXValue = document.getElementById('panXValue');
        const panYSlider = document.getElementById('panYSlider');
        const panYValue = document.getElementById('panYValue');
        const resetViewBtn = document.getElementById('resetViewBtn');
        
        // Variables
        let isPlaying = false;
        let animationId = null;
        let fps = 10;
        let lastFrameTime = 0;
        let zoomLevel = 100;
        let panX = 0;
        let panY = 0;
        
        // Create 16x16 grid for preview
        for (let y = 0; y < 16; y++) {
            for (let x = 0; x < 16; x++) {
                const pixel = document.createElement('div');
                pixel.className = 'pixel';
                pixel.dataset.x = x;
                pixel.dataset.y = y;
                pixelGrid.appendChild(pixel);
            }
        }
        
        // Handle video file selection
        videoInput.addEventListener('change', function(e) {
            const file = e.target.files[0];
            if (!file) return;
            
            const url = URL.createObjectURL(file);
            video.src = url;
            videoContainer.style.display = 'block';
            canvas.style.display = 'block';
            statusEl.textContent = `Video loaded: ${file.name}`;
            
            // Enable controls when video is loaded
            video.onloadedmetadata = function() {
                playBtn.disabled = false;
                updatePixelboardPreview();
            };
        });
        
        // Speed slider
        speedSlider.addEventListener('input', function() {
            fps = parseInt(this.value);
            fpsValue.textContent = `${fps} FPS`;
        });
        
        // Zoom slider
        zoomSlider.addEventListener('input', function() {
            zoomLevel = parseInt(this.value);
            zoomValue.textContent = `${zoomLevel}%`;
            updatePixelboardPreview();
        });
        
        // Pan X slider
        panXSlider.addEventListener('input', function() {
            panX = parseInt(this.value);
            panXValue.textContent = `${panX}%`;
            updatePixelboardPreview();
        });
        
        // Pan Y slider
        panYSlider.addEventListener('input', function() {
            panY = parseInt(this.value);
            panYValue.textContent = `${panY}%`;
            updatePixelboardPreview();
        });
        
        // Reset view button
        resetViewBtn.addEventListener('click', function() {
            zoomLevel = 100;
            panX = 0;
            panY = 0;
            zoomSlider.value = 100;
            panXSlider.value = 0;
            panYSlider.value = 0;
            zoomValue.textContent = '100%';
            panXValue.textContent = '0%';
            panYValue.textContent = '0%';
            updatePixelboardPreview();
        });
        
        // Play button
        playBtn.addEventListener('click', function() {
            if (isPlaying) return;
            
            // Start playback on device
            fetch('/videocontrol?action=play&fps=' + fps)
                .then(response => response.text())
                .then(text => {
                    statusEl.textContent = "Playing on PixelBoard";
                });
            
            // Start local preview
            isPlaying = true;
            playBtn.disabled = true;
            pauseBtn.disabled = false;
            stopBtn.disabled = false;
            
            // Start playback and animation
            video.play();
            animateVideo();
        });
        
        // Pause button
        pauseBtn.addEventListener('click', function() {
            if (!isPlaying) return;
            
            // Pause on device
            fetch('/videocontrol?action=pause')
                .then(response => response.text())
                .then(text => {
                    statusEl.textContent = "Paused";
                });
            
            // Pause local preview
            isPlaying = false;
            playBtn.disabled = false;
            pauseBtn.disabled = true;
            
            // Pause video and animation
            video.pause();
            cancelAnimationFrame(animationId);
        });
        
        // Stop button
        stopBtn.addEventListener('click', function() {
            // Stop on device
            fetch('/videocontrol?action=stop')
                .then(response => response.text())
                .then(text => {
                    statusEl.textContent = "Stopped";
                });
            
            // Stop local preview
            isPlaying = false;
            playBtn.disabled = false;
            pauseBtn.disabled = true;
            stopBtn.disabled = true;
            
            // Reset video and animation
            video.pause();
            video.currentTime = 0;
            cancelAnimationFrame(animationId);
            updatePixelboardPreview();
        });
        
        // Clear button
        clearBtn.addEventListener('click', function() {
            fetch('/videocontrol?action=clear')
                .then(response => response.text())
                .then(text => {
                    statusEl.textContent = "Board cleared";
                    // Clear preview grid
                    const pixels = document.getElementsByClassName('pixel');
                    for(let pixel of pixels) {
                        pixel.style.backgroundColor = '#000';
                    }
                });
        });
        
        // Function to animate video frames
        function animateVideo() {
            const now = performance.now();
            const elapsed = now - lastFrameTime;
            
            // Only process frame if enough time has passed (based on FPS)
            if (elapsed > 1000 / fps) {
                lastFrameTime = now;
                updatePixelboardPreview();
                
                // Send frame to device
                sendFrameToDevice();
            }
            
            // Continue animation if still playing
            if (isPlaying) {
                animationId = requestAnimationFrame(animateVideo);
            }
        }
        
        // Update the preview with current video frame
        function updatePixelboardPreview() {
            // Draw current video frame to canvas
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            // Calculate base dimensions and offsets for video
            const videoAspect = video.videoWidth / video.videoHeight;
            const canvasAspect = canvas.width / canvas.height;
            
            let baseWidth, baseHeight, baseX, baseY;
            
            if (videoAspect > canvasAspect) {
                // Video is wider than canvas
                baseWidth = canvas.width;
                baseHeight = canvas.width / videoAspect;
                baseX = 0;
                baseY = (canvas.height - baseHeight) / 2;
            } else {
                // Video is taller than canvas
                baseHeight = canvas.height;
                baseWidth = canvas.height * videoAspect;
                baseX = (canvas.width - baseWidth) / 2;
                baseY = 0;
            }
            
            // Apply zoom and pan adjustments
            const zoomFactor = zoomLevel / 100;
            
            // Calculate new dimensions with zoom
            const zoomedWidth = baseWidth * zoomFactor;
            const zoomedHeight = baseHeight * zoomFactor;
            
            // Calculate max pan range based on zoom
            const maxPanX = (zoomedWidth - baseWidth) / 2;
            const maxPanY = (zoomedHeight - baseHeight) / 2;
            
            // Calculate adjusted position with pan
            const adjustedX = baseX - (maxPanX * panX / 100);
            const adjustedY = baseY - (maxPanY * panY / 100);
            
            // Draw video frame with zoom and pan applied
            try {
                ctx.drawImage(
                    video,
                    adjustedX, adjustedY, zoomedWidth, zoomedHeight
                );
            } catch (e) {
                console.error("Error drawing video frame:", e);
            }
            
            // Sample canvas at 16x16 resolution and update grid
            const pixelSize = 10; // 160/16 = 10
            for(let y = 0; y < 16; y++) {
                for(let x = 0; x < 16; x++) {
                    // Sample from center of each grid cell
                    const imageData = ctx.getImageData(x * pixelSize + 5, y * pixelSize + 5, 1, 1).data;
                    const r = imageData[0];
                    const g = imageData[1];
                    const b = imageData[2];
                    
                    // Update preview grid
                    const pixel = document.querySelector(`.pixel[data-x="${x}"][data-y="${y}"]`);
                    pixel.style.backgroundColor = `rgb(${r},${g},${b})`;
                }
            }
        }
        
        // Send current frame to device
        function sendFrameToDevice() {
            let pixelData = '';
            
            // Process grid at 16x16 resolution
            for(let y = 0; y < 16; y++) {
                for(let x = 0; x < 16; x++) {
                    // For even rows, get pixels from right to left (serpentine pattern)
                    const sampleX = (y % 2 === 0) ? (15 - x) : x;
                    
                    // Sample directly from the canvas
                    const imageData = ctx.getImageData(sampleX * 10 + 5, y * 10 + 5, 1, 1).data;
                    const r = imageData[0];
                    const g = imageData[1];
                    const b = imageData[2];
                    
                    // Convert RGB to hex string
                    pixelData += (
                        r.toString(16).padStart(2, '0') +
                        g.toString(16).padStart(2, '0') +
                        b.toString(16).padStart(2, '0')
                    );
                }
            }
            
            // Send frame data to device
            fetch('/videoframe?pixels=' + pixelData);
        }
        
        // Update grid when video is seeked
        video.addEventListener('seeked', updatePixelboardPreview);
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", html);
    });

    // Handle video control commands
    server->on("/videocontrol", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("action")) {
            String action = request->getParam("action")->value();
            
            if (action == "play") {
                // Get FPS if provided
                int fps = 10; // Default
                if (request->hasParam("fps")) {
                    fps = request->getParam("fps")->value().toInt();
                    if (fps < 1) fps = 1;
                    if (fps > 30) fps = 30;
                }
                
                // Set frame delay based on FPS
                frameDelay = 1000 / fps;
                
                request->send(200, "text/plain", "Playing at " + String(fps) + " FPS");
            }
            else if (action == "pause") {
                request->send(200, "text/plain", "Paused");
            }
            else if (action == "stop") {
                request->send(200, "text/plain", "Stopped");
            }
            else if (action == "clear") {
                fill_solid(pixelStates, NUM_LEDS, CRGB::Black);
                fill_solid(leds, NUM_LEDS, CRGB::Black);
                FastLED.show();
                request->send(200, "text/plain", "Cleared");
            }
            else {
                request->send(400, "text/plain", "Invalid action");
            }
        } else {
            request->send(400, "text/plain", "Missing action parameter");
        }
    });

    // Handle video frame data
    server->on("/videoframe", HTTP_GET, [](AsyncWebServerRequest *request) {
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