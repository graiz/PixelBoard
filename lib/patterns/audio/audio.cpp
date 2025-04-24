#include "audio.h"
#include "led_display.h"
#include "patterns.h"
#include "arduinoFFT.h"

#define MIC_PIN 34
#define SAMPLES 1024          // Must be a power of 2
#define SAMPLING_FREQ 40000   // Hz, must be 40000 or less due to ADC conversion time
#define NUM_BANDS 16          // Number of frequency bands
#define TOP 16               // Maximum height of bars
#define DEBUG_INTERVAL 100   // Debug print interval in ms
#define CENTER_BOOST 1.5     // Center column boost factor

// FFT and sampling variables
static double vReal[SAMPLES];
static double vImag[SAMPLES];
static uint16_t bandValues[NUM_BANDS];    // Changed to uint16_t to handle larger values
static uint8_t peak[NUM_BANDS];          // Peak values for each band
static unsigned long lastDebug = 0;
static unsigned long samplingPeriodUs;
static ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

// Exponential moving average for noise floor
static float noiseFloor[NUM_BANDS];

// Pattern state
static uint8_t currentPattern = 0;              // Current visualization pattern

// Color palettes
static const TProgmemRGBGradientPalette_byte purpleGradient[] = {
    0,   0, 212, 255,   //blue
    255, 179, 0, 255    //purple
};

static const TProgmemRGBGradientPalette_byte outrunGradient[] = {
    0,   141, 0,   100,  //purple
    127, 255, 192, 0,    //yellow
    255, 0,   5,   255   //blue
};

static const TProgmemRGBGradientPalette_byte greenblueGradient[] = {
    0,   0,   255, 60,   //green
    64,  0,   236, 255,  //cyan
    128, 0,   5,   255,  //blue
    192, 0,   236, 255,  //cyan
    255, 0,   255, 60    //green
};

static const TProgmemRGBGradientPalette_byte redyellowGradient[] = {
    0,   200, 200, 200,  //white
    64,  255, 218, 0,    //yellow
    128, 231, 0,   0,    //red
    192, 255, 218, 0,    //yellow
    255, 200, 200, 200   //white
};

static CRGBPalette16 purplePal(purpleGradient);
static CRGBPalette16 outrunPal(outrunGradient);
static CRGBPalette16 greenbluePal(greenblueGradient);
static CRGBPalette16 heatPal(redyellowGradient);

// Adjustable parameters (now controlled via UI)
static struct {
    uint16_t noiseThreshold = 348;     // For noise reduction
    uint16_t minAmplitude = 70;        // Minimum amplitude to register
    uint16_t maxAmplitude = 5000;      // Maximum amplitude to register
    uint8_t scaleFactor = 1;           // For scaling display height
    float noiseAlpha = 0.45;           // For noise floor tracking
    float smoothingFactor = 0.46;      // For smooth transitions
} params;

void setupAudio() {
    samplingPeriodUs = round(1000000 * (1.0 / SAMPLING_FREQ));
    pinMode(MIC_PIN, INPUT);
}

void setupAudioPattern(AsyncWebServer* server) {
    // Handle parameter updates
    server->on("/audioupdate", HTTP_GET, [](AsyncWebServerRequest *request) { 
        if (request->hasParam("noiseThreshold")) {
            params.noiseThreshold = request->getParam("noiseThreshold")->value().toInt();
        }
        if (request->hasParam("minAmplitude")) {
            params.minAmplitude = request->getParam("minAmplitude")->value().toInt();
        }
        if (request->hasParam("maxAmplitude")) {
            params.maxAmplitude = request->getParam("maxAmplitude")->value().toInt();
        }
        if (request->hasParam("scaleFactor")) {
            params.scaleFactor = request->getParam("scaleFactor")->value().toInt();
        }
        if (request->hasParam("noiseAlpha")) {
            params.noiseAlpha = request->getParam("noiseAlpha")->value().toFloat() * 0.01;
        }
        if (request->hasParam("smoothingFactor")) {
            params.smoothingFactor = request->getParam("smoothingFactor")->value().toFloat() * 0.01;
        }
        if (request->hasParam("pattern")) {
            currentPattern = request->getParam("pattern")->value().toInt();
        }
        request->send(200, "text/plain", "OK");
    });

    // Serve the control panel HTML
    server->on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Audio Visualizer Controls</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .control { margin: 20px 0; }
        .slider-container { margin: 10px 0; }
        .slider-label { display: inline-block; width: 150px; }
        .button { padding: 10px; margin: 5px; cursor: pointer; }
    </style>
</head>
<body>
    <h2>Audio Visualizer Controls</h2>
    
    <div class="control">
        <div class="slider-container">
            <span class="slider-label">Noise Threshold:</span>
            <input type="range" min="0" max="1000" value="348" id="noiseThreshold">
            <span id="noiseThresholdValue">348</span>
        </div>
        
        <div class="slider-container">
            <span class="slider-label">Min Amplitude:</span>
            <input type="range" min="0" max="1000" value="70" id="minAmplitude">
            <span id="minAmplitudeValue">70</span>
        </div>
        
        <div class="slider-container">
            <span class="slider-label">Max Amplitude:</span>
            <input type="range" min="1000" max="5000" value="5000" id="maxAmplitude">
            <span id="maxAmplitudeValue">5000</span>
        </div>
        
        <div class="slider-container">
            <span class="slider-label">Scale Factor:</span>
            <input type="range" min="1" max="10" value="1" id="scaleFactor">
            <span id="scaleFactorValue">1</span>
        </div>
        
        <div class="slider-container">
            <span class="slider-label">Noise Alpha (%):</span>
            <input type="range" min="1" max="100" value="45" id="noiseAlpha">
            <span id="noiseAlphaValue">45</span>
        </div>
        
        <div class="slider-container">
            <span class="slider-label">Smoothing (%):</span>
            <input type="range" min="0" max="100" value="46" id="smoothingFactor">
            <span id="smoothingFactorValue">46</span>
        </div>
    </div>
    
    <div class="control">
        <button class="button" onclick="changePattern(0)">Rainbow Bars</button>
        <button class="button" onclick="changePattern(1)">Peaks Only</button>
        <button class="button" onclick="changePattern(2)">Purple Bars</button>
        <button class="button" onclick="changePattern(3)">Center Bars</button>
        <button class="button" onclick="changePattern(4)">Changing Bars</button>
        <button class="button" onclick="changePattern(5)">Waterfall</button>
    </div>
    
    <script>
        function updateSlider(id) {
            const slider = document.getElementById(id);
            const valueSpan = document.getElementById(id + 'Value');
            valueSpan.textContent = slider.value;
            
            fetch('/audioupdate?' + id + '=' + slider.value)
                .then(response => response.text())
                .then(data => console.log('Updated:', id, data));
        }
        
        function changePattern(pattern) {
            fetch('/audioupdate?pattern=' + pattern)
                .then(response => response.text())
                .then(data => console.log('Pattern changed:', data));
        }
        
        // Set up slider event listeners
        const sliders = ['noiseThreshold', 'minAmplitude', 'maxAmplitude', 
                        'scaleFactor', 'noiseAlpha', 'smoothingFactor'];
        
        sliders.forEach(id => {
            const slider = document.getElementById(id);
            slider.oninput = () => {
                document.getElementById(id + 'Value').textContent = slider.value;
            };
            slider.onchange = () => updateSlider(id);
        });
    </script>
</body>
</html>)rawliteral";
        request->send(200, "text/html", html);
    });
}

// Get the current sound level from the microphone
uint16_t getSoundLevel() {
    uint16_t signalMax = 0;
    uint16_t signalMin = 4095;
    
    // Sample the audio input
    for (int i = 0; i < SAMPLES; i++) {
        unsigned long startMicros = micros();
        
        uint16_t sample = analogRead(MIC_PIN);
        vReal[i] = sample;
        vImag[i] = 0;
        
        // Track min and max values
        if (sample > signalMax) signalMax = sample;
        if (sample < signalMin) signalMin = sample;
        
        // Wait for the sampling period
        while(micros() - startMicros < samplingPeriodUs) {
            yield();
        }
    }
    
    return signalMax - signalMin;
}

// Calculate center bias for each band
float getCenterBias(uint8_t band) {
    float center = (NUM_BANDS - 1) / 2.0f;
    float distance = abs(band - center);
    float normalizedDist = distance / center;
    return 1.0f + (CENTER_BOOST - 1.0f) * (1.0f - normalizedDist);
}

// Process audio and update the display
void audio(CRGB* leds) {
    unsigned long currentMillis = millis();
    
    // Get sound level and run FFT
    uint16_t soundLevel = getSoundLevel();
    FFT.dcRemoval(vReal, SAMPLES);
    FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);
    
    // Process each frequency band
    for (uint8_t band = 0; band < NUM_BANDS; band++) {
        // Calculate band value from FFT data
        float value = 0;
        uint16_t lowBin = band == 0 ? 2 : (band * 2 + 1);
        uint16_t highBin = band == 0 ? 3 : (band * 2 + 2);
        
        for (uint16_t i = lowBin; i <= highBin; i++) {
            if (i < SAMPLES/2) {
                value += vReal[i];
            }
        }
        value /= (highBin - lowBin + 1);
        
        // Update noise floor using exponential moving average
        noiseFloor[band] = noiseFloor[band] * (1 - params.noiseAlpha) + value * params.noiseAlpha;
        
        // Calculate final band value with noise reduction
        value = max(0.0f, value - noiseFloor[band] - params.noiseThreshold);
        value = constrain(value, 0, params.maxAmplitude);
        value = map(value, params.minAmplitude, params.maxAmplitude, 0, TOP * params.scaleFactor);
        
        // Apply center bias
        float bias = getCenterBias(band);
        value *= bias;
        
        // Update band value with smoothing
        bandValues[band] = bandValues[band] * params.smoothingFactor + 
                          value * (1 - params.smoothingFactor);
        
        // Update peak
        if (bandValues[band] > peak[band]) {
            peak[band] = bandValues[band];
        } else {
            peak[band] = peak[band] * 0.95; // Decay peaks
        }
    }
    
    // Debug output every 100ms
    if (currentMillis - lastDebug >= DEBUG_INTERVAL) {
        Serial.printf("Raw Level: %d, Center Band: %d\n", soundLevel, bandValues[NUM_BANDS/2]);
        lastDebug = currentMillis;
    }
    
    // Update display based on current pattern
    switch (currentPattern) {
        case 0:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                uint8_t barHeight = constrain(bandValues[band] / params.scaleFactor, 0, TOP);
                audioRainbowBars(leds, band, barHeight);
            }
            break;
        case 1:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                uint8_t peakHeight = constrain(peak[band] / params.scaleFactor, 0, TOP-1);
                audioWhitePeak(leds, band, peakHeight);
            }
            break;
        case 2:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                uint8_t barHeight = constrain(bandValues[band] / params.scaleFactor, 0, TOP);
                audioPurpleBars(leds, band, barHeight);
            }
            break;
        case 3:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                uint8_t barHeight = constrain(bandValues[band] / params.scaleFactor, 0, TOP);
                audioCenterBars(leds, band, barHeight);
            }
            break;
        case 4:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                uint8_t barHeight = constrain(bandValues[band] / params.scaleFactor, 0, TOP);
                audioChangingBars(leds, band, barHeight);
            }
            break;
        case 5:
            for (uint8_t band = 0; band < NUM_BANDS; band++) {
                audioWaterfall(leds, band);
            }
            break;
    }
    
    FastLED.show();
    delay(10); // Small delay to prevent flickering
}

// Pattern-specific functions
void audioRainbowBars(CRGB* leds, int band, int barHeight) {
    for (uint8_t y = 0; y < barHeight; y++) {
        leds[XY(band, TOP-1-y)] = CHSV(band * 16, 255, 255);
    }
    // Clear above the bar
    for (uint8_t y = barHeight; y < TOP; y++) {
        leds[XY(band, TOP-1-y)] = CRGB::Black;
    }
}

void audioWhitePeak(CRGB* leds, int band, int peakHeight) {
    // Clear the column
    for (uint8_t y = 0; y < TOP; y++) {
        leds[XY(band, TOP-1-y)] = CRGB::Black;
    }
    // Draw just the peak
    if (peakHeight > 0 && peakHeight < TOP) {
        leds[XY(band, TOP-1-peakHeight)] = CRGB::White;
    }
}

void audioPurpleBars(CRGB* leds, int band, int barHeight) {
    for (uint8_t y = 0; y < barHeight; y++) {
        uint8_t colorIndex = map(y, 0, TOP-1, 0, 255);
        leds[XY(band, TOP-1-y)] = ColorFromPalette(purplePal, colorIndex);
    }
    // Clear above the bar
    for (uint8_t y = barHeight; y < TOP; y++) {
        leds[XY(band, TOP-1-y)] = CRGB::Black;
    }
}

void audioCenterBars(CRGB* leds, int band, int barHeight) {
    uint8_t centerY = TOP / 2;
    uint8_t halfHeight = barHeight / 2;
    
    // Draw up from center
    for (uint8_t y = 0; y <= halfHeight; y++) {
        leds[XY(band, centerY-y)] = ColorFromPalette(outrunPal, y * 32);
    }
    // Draw down from center
    for (uint8_t y = 0; y <= halfHeight; y++) {
        leds[XY(band, centerY+y)] = ColorFromPalette(outrunPal, y * 32);
    }
    // Clear remaining pixels
    for (uint8_t y = halfHeight + 1; y < centerY; y++) {
        leds[XY(band, centerY-y)] = CRGB::Black;
        leds[XY(band, centerY+y)] = CRGB::Black;
    }
}

void audioChangingBars(CRGB* leds, int band, int barHeight) {
    static uint8_t colorOffset = 0;
    
    for (uint8_t y = 0; y < barHeight; y++) {
        uint8_t colorIndex = (y * 16 + colorOffset) % 255;
        leds[XY(band, TOP-1-y)] = ColorFromPalette(greenbluePal, colorIndex);
    }
    // Clear above the bar
    for (uint8_t y = barHeight; y < TOP; y++) {
        leds[XY(band, TOP-1-y)] = CRGB::Black;
    }
    
    if (band == NUM_BANDS-1) {
        colorOffset += 2; // Update color offset once per frame
    }
}

void audioWaterfall(CRGB* leds, int band) {
    // Shift existing pixels down
    for (uint8_t y = TOP-1; y > 0; y--) {
        leds[XY(band, y)] = leds[XY(band, y-1)];
    }
    
    // Add new value at top
    uint8_t intensity = constrain(bandValues[band] / params.scaleFactor, 0, TOP);
    leds[XY(band, 0)] = ColorFromPalette(heatPal, intensity * 16);
} 