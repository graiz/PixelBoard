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
        .control-panel {
            width: min(80%, 600px);
            margin-bottom: 20px;
            padding: 20px;
            background: #3b3f47;
            border-radius: 10px;
            border: 1px solid #61dafb;
        }
        .section-title {
            font-size: 18px;
            color: #61dafb;
            margin: 0 0 15px 0;
        }
        .controls-layout {
            display: flex;
            gap: 20px;
        }
        .controls-left {
            flex: 1;
        }
        .preview-box {
            width: 120px;
            height: 120px;
            background: #282c34;
            border-radius: 4px;
            border: 1px solid #61dafb;
            overflow: hidden;
        }
        .preview-box video {
            width: 100%;
            height: 100%;
            object-fit: cover;
        }
        .slider-row {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 15px;
        }
        .slider-row label {
            width: 70px;
            color: #ffffff;
        }
        .slider-row input[type="range"] {
            flex: 1;
            -webkit-appearance: none;
            height: 4px;
            background: #282c34;
            border-radius: 2px;
            outline: none;
        }
        .slider-row input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 16px;
            height: 16px;
            border-radius: 50%;
            background: #61dafb;
            cursor: pointer;
        }
        .slider-row .value {
            width: 60px;
            text-align: right;
            color: #61dafb;
        }
        .progress-row {
            margin: 15px 0;
            padding: 15px 0;
            border-top: 1px solid #61dafb;
            border-bottom: 1px solid #61dafb;
        }
        .progress-row input[type="range"] {
            width: 100%;
            -webkit-appearance: none;
            height: 4px;
            background: #282c34;
            border-radius: 2px;
            outline: none;
        }
        .progress-row input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 16px;
            height: 16px;
            border-radius: 50%;
            background: #61dafb;
            cursor: pointer;
        }
        .button-row {
            display: flex;
            gap: 10px;
            margin-top: 15px;
            padding-top: 15px;
            border-top: 1px solid #61dafb;
        }
        .action-btn {
            background: #282c34;
            color: #61dafb;
            border: 1px solid #61dafb;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: normal;
            font-size: 14px;
            transition: all 0.2s;
        }
        .action-btn:hover:not(:disabled) {
            background: #61dafb;
            color: #282c34;
        }
        .action-btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(16, 1fr);
            gap: 2px;
            background: #3b3f47;
            padding: 20px;
            border-radius: 10px;
            aspect-ratio: 1;
            width: min(80%, 600px);
        }
        .pixel {
            aspect-ratio: 1;
            background: #282c34;
            border-radius: 2px;
        }
        #videoInput {
            display: none;
        }
        #preview-canvas {
            display: none;
        }
    </style>
</head>
<body>
    <div class="control-panel">
        <div class="section-title">Playback Controls</div>
        <div class="controls-layout">
            <div class="controls-left">
                <div class="progress-row">
                    <input type="range" id="progressSlider" min="0" max="100" value="0" step="0.1">
                </div>
                <div class="slider-row">
                    <label>Speed:</label>
                    <input type="range" id="speedSlider" min="1" max="30" value="10">
                    <span class="value" id="fpsValue">10 FPS</span>
                </div>
                <div class="slider-row">
                    <label>Zoom:</label>
                    <input type="range" id="zoomSlider" min="50" max="300" value="100">
                    <span class="value" id="zoomValue">100%</span>
                </div>
                <div class="slider-row">
                    <label>Pan X:</label>
                    <input type="range" id="panXSlider" min="-64" max="64" value="0">
                    <span class="value" id="panXValue">0px</span>
                </div>
                <div class="slider-row">
                    <label>Pan Y:</label>
                    <input type="range" id="panYSlider" min="-64" max="64" value="0">
                    <span class="value" id="panYValue">0px</span>
                </div>
            </div>
            <div class="preview-box">
                <video id="video"></video>
            </div>
        </div>
        <div class="button-row">
            <button id="playBtn" class="action-btn" disabled>Play</button>
            <button id="pauseBtn" class="action-btn" disabled>Pause</button>
            <button id="stopBtn" class="action-btn" disabled>Stop</button>
            <button id="clearBtn" class="action-btn">Clear Board</button>
            <label class="action-btn" for="videoInput">Choose Video File</label>
            <input type="file" id="videoInput" accept="video/*">
        </div>
    </div>

    <canvas id="preview-canvas" width="160" height="160"></canvas>
    <div class="grid" id="pixelGrid"></div>

    <script>
        // DOM elements
        const videoInput = document.getElementById('videoInput');
        const video = document.getElementById('video');
        const canvas = document.getElementById('preview-canvas');
        const ctx = canvas.getContext('2d', { willReadFrequently: true });
        const pixelGrid = document.getElementById('pixelGrid');
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
        const progressSlider = document.getElementById('progressSlider');
        
        // Variables
        let isPlaying = false;
        let animationId = null;
        let fps = 10;
        let lastFrameTime = 0;
        let zoomLevel = 100;
        let panX = 0;
        let panY = 0;
        let isDraggingProgress = false;
        
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
            
            video.onloadedmetadata = function() {
                playBtn.disabled = false;
                progressSlider.max = video.duration;
                updatePixelboardPreview();
            };
        });
        
        // Progress slider
        progressSlider.addEventListener('mousedown', () => {
            isDraggingProgress = true;
            if (isPlaying) {
                video.pause();
                cancelAnimationFrame(animationId);
            }
        });
        
        progressSlider.addEventListener('mouseup', () => {
            isDraggingProgress = false;
            if (isPlaying) {
                video.play();
                animateVideo();
            }
        });
        
        progressSlider.addEventListener('input', function() {
            video.currentTime = parseFloat(this.value);
            updatePixelboardPreview();
        });
        
        // Update progress bar during playback
        video.addEventListener('timeupdate', function() {
            if (!isDraggingProgress) {
                progressSlider.value = video.currentTime;
            }
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
            panXValue.textContent = `${panX}px`;
            updatePixelboardPreview();
        });
        
        // Pan Y slider
        panYSlider.addEventListener('input', function() {
            panY = parseInt(this.value);
            panYValue.textContent = `${panY}px`;
            updatePixelboardPreview();
        });
        
        // Play button
        playBtn.addEventListener('click', function() {
            if (isPlaying) return;
            
            fetch('/videocontrol?action=play&fps=' + fps)
                .then(response => response.text())
                .then(text => {
                    console.log("Playing video");
                });
            
            isPlaying = true;
            playBtn.disabled = true;
            pauseBtn.disabled = false;
            stopBtn.disabled = false;
            
            video.play();
            animateVideo();
        });
        
        // Pause button
        pauseBtn.addEventListener('click', function() {
            if (!isPlaying) return;
            
            fetch('/videocontrol?action=pause')
                .then(response => response.text())
                .then(text => {
                    console.log("Paused video");
                });
            
            isPlaying = false;
            playBtn.disabled = false;
            pauseBtn.disabled = true;
            
            video.pause();
            cancelAnimationFrame(animationId);
        });
        
        // Stop button
        stopBtn.addEventListener('click', function() {
            fetch('/videocontrol?action=stop')
                .then(response => response.text())
                .then(text => {
                    console.log("Stopped video");
                });
            
            isPlaying = false;
            playBtn.disabled = false;
            pauseBtn.disabled = true;
            stopBtn.disabled = true;
            
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
                    console.log("Cleared board");
                    const pixels = document.getElementsByClassName('pixel');
                    for(let pixel of pixels) {
                        pixel.style.backgroundColor = '#282c34';
                    }
                });
        });
        
        // Function to animate video frames
        function animateVideo() {
            const now = performance.now();
            const elapsed = now - lastFrameTime;
            
            if (elapsed > 1000 / fps) {
                lastFrameTime = now;
                updatePixelboardPreview();
                sendFrameToDevice();
            }
            
            if (isPlaying) {
                animationId = requestAnimationFrame(animateVideo);
            }
        }
        
        // Update the preview with current video frame
        function updatePixelboardPreview() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            const videoAspect = video.videoWidth / video.videoHeight;
            const canvasAspect = canvas.width / canvas.height;
            
            let baseWidth, baseHeight, baseX, baseY;
            
            // Calculate base dimensions to fill canvas while maintaining aspect ratio
            if (videoAspect > canvasAspect) {
                baseWidth = canvas.width;
                baseHeight = canvas.width / videoAspect;
                baseX = 0;
                baseY = (canvas.height - baseHeight) / 2;
            } else {
                baseHeight = canvas.height;
                baseWidth = canvas.height * videoAspect;
                baseX = (canvas.width - baseWidth) / 2;
                baseY = 0;
            }
            
            // Apply zoom from center
            const zoomFactor = zoomLevel / 100;
            const zoomedWidth = baseWidth * zoomFactor;
            const zoomedHeight = baseHeight * zoomFactor;
            
            // Center the zoomed image and apply pixel-based pan
            const centerOffsetX = (zoomedWidth - baseWidth) / 2;
            const centerOffsetY = (zoomedHeight - baseHeight) / 2;
            
            // Calculate final position with pan
            const adjustedX = baseX - centerOffsetX + panX;
            const adjustedY = baseY - centerOffsetY + panY;
            
            try {
                ctx.drawImage(video, adjustedX, adjustedY, zoomedWidth, zoomedHeight);
                
                const pixelSize = 10;
                for(let y = 0; y < 16; y++) {
                    for(let x = 0; x < 16; x++) {
                        const imageData = ctx.getImageData(x * pixelSize + 5, y * pixelSize + 5, 1, 1).data;
                        const pixel = document.querySelector(`.pixel[data-x="${x}"][data-y="${y}"]`);
                        pixel.style.backgroundColor = `rgb(${imageData[0]},${imageData[1]},${imageData[2]})`;
                    }
                }
            } catch (e) {
                console.error("Error updating preview:", e);
            }
        }
        
        // Send current frame to device
        function sendFrameToDevice() {
            const pixelSize = 10;
            const buffer = new Uint8Array(16 * 16 * 3); // 256 pixels * 3 bytes (RGB)
            
            for(let y = 0; y < 16; y++) {
                for(let x = 0; x < 16; x++) {
                    // For even rows, get pixels from right to left (serpentine pattern)
                    const sampleX = (y % 2 === 0) ? (15 - x) : x;
                    const imageData = ctx.getImageData(sampleX * pixelSize + 5, y * pixelSize + 5, 1, 1).data;
                    
                    // Calculate buffer index: (y * 16 + x) * 3 for RGB values
                    const bufferIndex = (y * 16 + x) * 3;
                    buffer[bufferIndex] = imageData[0];     // R
                    buffer[bufferIndex + 1] = imageData[1]; // G
                    buffer[bufferIndex + 2] = imageData[2]; // B
                }
            }
            
            // Send binary data
            fetch('/videoframe', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/octet-stream'
                },
                body: buffer
            });
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

    // Handle video frame data - updated to handle binary data
    server->on("/videoframe", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200); // Send response immediately
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (len == 768) { // 256 pixels * 3 bytes (RGB)
            for (size_t i = 0; i < 256; i++) {
                const size_t dataIndex = i * 3;
                pixelStates[i] = CRGB(data[dataIndex], data[dataIndex + 1], data[dataIndex + 2]);
                leds[i] = pixelStates[i];
            }
            FastLED.show();
        }
    });
}