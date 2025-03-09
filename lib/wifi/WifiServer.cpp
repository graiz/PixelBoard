//
// WiFiServer.cpp — Example code for an ESP32 that either:
//    1) Starts as an Access Point for onboarding (if no credentials are saved).
//    2) Connects to known Wi-Fi and serves pattern selection pages.
//
// NOTE: For clarity, all logic is in this single file. 
//       Adjust or split into .h/.cpp as your project requires.
//

#include "WifiServer.h"        // Your header if needed
#include <WiFi.h>
#include <Preferences.h>       // For storing Wi-Fi credentials in NVS
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "patterns.h"          // For g_patternList, PATTERN_COUNT, etc.
#include "draw/draw.h"         // For setupDrawHandler
#include "video/video.h"       // For setupVideoPlayer
#include "SPIFFS.h"

// -------------------------------------------------------------------
// Global AsyncWebServer for "normal" operation on port 80
// -------------------------------------------------------------------
AsyncWebServer server(80);

// -------------------------------------------------------------------
// We'll create another server for AP onboarding, if needed
// (You can combine them, but this keeps logic simpler.)
// -------------------------------------------------------------------
static AsyncWebServer onboardServer(80);
// Onboarding happens once and is usually at 192.168.4.1

// -------------------------------------------------------------------
// External references from main/patterns
// -------------------------------------------------------------------
extern uint8_t g_current_pattern_number;   // Which pattern is selected
extern int g_Brightness;                   // From main code
extern int g_Speed;                        // From main code

// Provided by patterns.h
extern Pattern g_patternList[];
extern const size_t PATTERN_COUNT;

// -------------------------------------------------------------------
// We'll store Wi-Fi credentials in Preferences
// -------------------------------------------------------------------
static String g_wifi_ssid;
static String g_wifi_password;
static bool g_hasCredentials = false;

// Forward declarations
static void loadCredentials();
static void saveCredentials(const String& ssid, const String& pass);
static void startAccessPoint();
static void connectToWiFi();
static void setupMDNS();
static void setupHomePage();
static void setupPatternHandler();
static void setupBrightnessHandler();
static void setupSpeedHandler();
static void setupPixelStatusHandler();
static void startServer();

// -------------------------------------------------------------------
// Load Wi-Fi credentials from NVS
// -------------------------------------------------------------------
static void loadCredentials() {
  Preferences prefs;
  prefs.begin("wifi", true);  // read-only
  g_wifi_ssid     = prefs.getString("ssid", "");
  g_wifi_password = prefs.getString("pass", "");
  prefs.end();

  if (!g_wifi_ssid.isEmpty() && !g_wifi_password.isEmpty()) {
    g_hasCredentials = true;
  } else {
    g_hasCredentials = false;
  }
}

// -------------------------------------------------------------------
// Save Wi-Fi credentials to NVS
// -------------------------------------------------------------------
static void saveCredentials(const String& ssid, const String& pass) {
  Preferences prefs;
  prefs.begin("wifi", false); // read/write
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

// -------------------------------------------------------------------
// Start Access Point for onboarding if no credentials
// -------------------------------------------------------------------
static void startAccessPoint() {
  Serial.println("[WiFi] No credentials found. Starting AP for onboarding...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("PixelBoardSetup", ""); // AP name + pass - note blank password for now
  IPAddress AP_IP = WiFi.softAPIP();
  Serial.print("[WiFi] AP IP address: ");
  Serial.println(AP_IP);

  // Provide a simple HTML form so user can enter SSID & password
  onboardServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>PixelBoard Setup</title></head>
      <body style="font-family:sans-serif;background:#f5f5f5;margin:0;padding:20px;">
        <h2>Enter Wi-Fi Credentials</h2>
        <form action="/save" method="get">
          <label for="ssid">SSID:</label><br>
          <input type="text" id="ssid" name="ssid" required><br><br>
          <label for="pass">Password:</label><br>
          <input type="password" id="pass" name="pass" required><br><br>
          <input type="submit" value="Save">
        </form>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  // Handle the form submission
  onboardServer.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("ssid") && request->hasArg("pass")) {
      String newSsid = request->arg("ssid");
      String newPass = request->arg("pass");
      saveCredentials(newSsid, newPass);

      // Send a quick response
      request->send(200, "text/html", 
          "Credentials saved! Device will reboot and try to connect.<br>"
          "SSID: " + newSsid + "<br>"
          "Password: " + newPass + "<br>"
          "<p>Please reconnect to the new network after reboot.</p>");

      // Reboot after short delay so user sees the page
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/html", 
                    "Missing SSID or Password in parameters. Try again.");
    }
  });

  // Start the onboarding server on port 80
  onboardServer.begin();
  Serial.println("[WiFi] Onboarding WebServer started (AP mode).");
}

// -------------------------------------------------------------------
// Connect to Wi-Fi in STA mode if credentials exist
// -------------------------------------------------------------------
static void connectToWiFi() {
  loadCredentials(); // Attempt to load from NVS

  if (!g_hasCredentials) {
    // No credentials => run AP mode for onboarding
    startAccessPoint();
    return;
  }

  // We have credentials, proceed in STA mode
  Serial.printf("[WiFi] Attempting to connect to SSID: %s\n", g_wifi_ssid.c_str());
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("PixelBoard");

  WiFi.begin(g_wifi_ssid.c_str(), g_wifi_password.c_str());

  const int maxAttempts = 5;
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(2000);
    attempts++;
    Serial.printf("[WiFi] Attempt %d of %d\n", attempts, maxAttempts);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Still not connected. Retrying...");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Failed to connect after max attempts. Rebooting...");
    ESP.restart();
  }
}

// -------------------------------------------------------------------
// Setup mDNS so we can use e.g. http://pixelboard.local
// -------------------------------------------------------------------
static void setupMDNS() {
  if (MDNS.begin("pixelboard")) {
    Serial.println("[mDNS] Responder started: http://pixelboard.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("[mDNS] Error setting up mDNS responder");
  }
}

// -------------------------------------------------------------------
// Home page route ("/") with improved styling for patterns
// -------------------------------------------------------------------
static void setupHomePage() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>PixelBoard Control</title>
        <script type="text/javascript" src="/static/libgif.js"></script>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
          body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #282c34;
            color: #ffffff;
            min-height: 100vh;
          }
          .page-layout {
            display: flex;
            min-height: 100vh;
          }
          .controls-panel {
            width: 33%;
            padding: 20px;
            background: #3b3f47;
            border-right: 1px solid #61dafb;
            box-shadow: 2px 0 5px rgba(0, 0, 0, 0.1);
            display: flex;
            flex-direction: column;
          }
          .main-controls {
            flex: 1;
          }
          .preview-panel {
            width: 67%;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
          }
          .preview-panel iframe {
            width: 100%;
            height: 100%;
            border: none;
            background: #3b3f47;
            border-radius: 10px;
          }
          .preview-grid {
            display: grid;
            grid-template-columns: repeat(16, 1fr);
            gap: 2px;
            padding: 20px;
            background: #3b3f47;
            border-radius: 10px;
            aspect-ratio: 1;
            width: min(80%, 600px);
          }
          .preview-pixel {
            aspect-ratio: 1;
            background: #61dafb;
            border-radius: 2px;
            transition: background-color 0.3s ease;
          }
          h1 {
            margin-bottom: 20px;
            font-size: 24px;
            color: #61dafb;
          }
          label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
          }
          select {
            width: 100%;
            padding: 8px;
            margin-bottom: 15px;
            border: 1px solid #61dafb;
            border-radius: 4px;
            background: #282c34;
            color: white;
          }
          .slider-container {
            margin-bottom: 15px;
          }
          .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 10px;
            border-radius: 5px;
            background: #282c34;
            outline: none;
            margin: 10px 0;
          }
          .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #61dafb;
            cursor: pointer;
          }
          .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #61dafb;
            cursor: pointer;
          }
          .value-display {
            text-align: right;
            color: #61dafb;
            font-size: 14px;
            margin-top: 5px;
          }
          .preview-controls {
            margin-top: auto;
            padding: 15px;
            background: #282c34;
            border-radius: 8px;
            border: 1px solid #61dafb;
          }
          .preview-controls h2 {
            font-size: 18px;
            color: #61dafb;
            margin: 0 0 15px 0;
          }
          .button-group {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
          }
          .control-button {
            flex: 1;
            padding: 8px;
            background: #61dafb;
            color: #282c34;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            transition: background-color 0.2s;
          }
          .control-button:hover {
            background: #4fa8d3;
          }
          .preview-status {
            font-size: 14px;
            color: #bbb;
            margin-top: 15px;
          }
          .preview-status span {
            color: #61dafb;
          }
          .value-display {
            font-size: 12px;
            color: #bbb;
            margin-top: 5px;
            width: 100%;
            overflow: hidden;
          }
        </style>
      </head>
      <body>
        <div class="page-layout">
          <div class="controls-panel">
            <div class="main-controls">
              <h1>PixelBoard Control</h1>
              
              <label for="pattern">Pattern:</label>
              <select id="pattern" onchange="updatePattern(this.value)">
    )rawliteral";

    // Add pattern options
    for (size_t i = 0; i < PATTERN_COUNT; i++) {
              html += "<option value=\"" + String(i) + "\"" +
                     (i == g_current_pattern_number ? " selected" : "") +
                     ">" + g_patternList[i].name + "</option>";
            }

            html += "</select>";

            // Add brightness slider
            html += "<div class=\"slider-container\">";
            html += "<label for=\"brightness\">Brightness: <span id=\"brightnessValue\">" + String(g_Brightness) + "</span></label>";
            html += "<input type=\"range\" min=\"0\" max=\"255\" value=\"" + String(g_Brightness) + "\" class=\"slider\" id=\"brightness\" oninput=\"updateBrightness(this.value)\">";
            html += "</div>";

            // Add speed slider
            html += "<div class=\"slider-container\">";
            html += "<label for=\"speed\">Speed: <span id=\"speedValue\">" + String(g_Speed) + "</span></label>";
            html += "<input type=\"range\" min=\"0\" max=\"255\" value=\"" + String(g_Speed) + "\" class=\"slider\" id=\"speed\" oninput=\"updateSpeed(this.value)\">";
            html += "</div>";

            // Add preview controls
    html += R"rawliteral(
            </div>
            <div class="preview-controls">
              <h2>Preview Controls</h2>
              <div class="slider-container">
                <label for="previewSpeed">Update Interval: <span id="previewSpeedValue">20s</span></label>
                <input type="range" min="10" max="10000" value="500" class="slider" id="previewSpeed" oninput="updatePreviewSpeed(this.value)">
                <div class="value-display">
                  <span style="float: left">10ms</span>
                  <span style="float: right">10s</span>
                </div>
              </div>
              <div class="preview-status">
                Status: <span id="previewStatus">Running</span><br>
                Last Update: <span id="lastUpdate">Never</span>
              </div>
            </div>
          </div>
          <div class="preview-panel" id="previewPanel">
            <!-- Preview content will be dynamically inserted here -->
          </div>
        </div>

        <script>
          let previewUpdateInterval;
          let isPaused = false;
          let currentUpdateInterval = 20000; // Default 20 seconds
          let isUpdating = false; // Flag to prevent concurrent requests

          function formatTime(date) {
            return date.toLocaleTimeString();
          }

          function formatInterval(ms) {
            return ms >= 1000 ? (ms / 1000).toFixed(1) + 's' : ms + 'ms';
          }

          function updateLastUpdateTime() {
            document.getElementById('lastUpdate').textContent = formatTime(new Date());
          }

          function updatePreviewSpeed(value) {
            currentUpdateInterval = parseInt(value);
            document.getElementById('previewSpeedValue').textContent = formatInterval(currentUpdateInterval);
            
            // Restart interval with new timing if running
            if (!isPaused) {
              startPreviewInterval();
            }
          }

          function refreshPreview() {
            // If already updating, skip this update
            if (isUpdating) {
              console.log('Skipping update - previous request still in progress');
              return;
            }

            isUpdating = true;
            document.getElementById('previewStatus').textContent = 'Updating...';

            fetch('/pixelStatus')
              .then(response => response.arrayBuffer())  // Get as binary data
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
                updateLastUpdateTime();
                document.getElementById('previewStatus').textContent = 'Running';
              })
              .catch(error => {
                console.error('Error updating preview:', error);
                document.getElementById('previewStatus').textContent = 'Error';
              })
              .finally(() => {
                isUpdating = false;
              });
          }

          function togglePreview() {
            const pauseButton = document.getElementById('pauseButton');
            const statusElement = document.getElementById('previewStatus');
            
            isPaused = !isPaused;
            
            if (isPaused) {
              clearInterval(previewUpdateInterval);
              pauseButton.textContent = 'Resume';
              statusElement.textContent = 'Paused';
            } else {
              startPreviewInterval();
              pauseButton.textContent = 'Pause';
              statusElement.textContent = 'Running';
              // Only refresh immediately if we're not already updating
              if (!isUpdating) {
                refreshPreview();
              }
            }
          }

          function startPreviewInterval() {
            if (previewUpdateInterval) {
              clearInterval(previewUpdateInterval);
            }
            previewUpdateInterval = setInterval(refreshPreview, currentUpdateInterval);
          }

          function startPreviewUpdates() {
            // Create the initial grid
            const previewPanel = document.getElementById('previewPanel');
            const grid = document.createElement('div');
            grid.className = 'preview-grid';
            
            // Create grid using row/column layout
            for (let i = 0; i < 256; i++) {
              const pixel = document.createElement('div');
              pixel.className = 'preview-pixel';
              pixel.id = 'pixel-' + i;
              grid.appendChild(pixel);
            }
            previewPanel.appendChild(grid);

            // Initial update
            refreshPreview();

            // Start interval if not paused
            if (!isPaused) {
              startPreviewInterval();
            }
          }

          function updateBrightness(value) {
            document.getElementById('brightnessValue').textContent = value;
            fetch('/brightness?value=' + value)
              .then(response => response.text())
              .then(data => console.log('Brightness updated:', data))
              .catch(error => console.error('Error:', error));
          }

          function updateSpeed(value) {
            document.getElementById('speedValue').textContent = value;
            fetch('/speed?value=' + value)
              .then(response => response.text())
              .then(data => console.log('Speed updated:', data))
              .catch(error => console.error('Error:', error));
          }

          function updatePattern(value) {
            const pattern = document.getElementById('pattern');
            const selectedPattern = pattern.options[pattern.selectedIndex].text;
            const previewPanel = document.getElementById('previewPanel');
            
            // Clear existing content
            previewPanel.innerHTML = '';
            
            // Clear any existing interval
            if (previewUpdateInterval) {
                clearInterval(previewUpdateInterval);
            }
            
            // Handle special patterns
            if (selectedPattern.toLowerCase().includes('draw')) {
                // Load draw interface
                const iframe = document.createElement('iframe');
                iframe.src = '/draw';
                previewPanel.appendChild(iframe);
            } else if (selectedPattern.toLowerCase().includes('video')) {
                // Load video interface
                const iframe = document.createElement('iframe');
                iframe.src = '/video';
                previewPanel.appendChild(iframe);
            } else {
                // Start preview updates for regular patterns
                startPreviewUpdates();
            }

            fetch('/pattern?value=' + value)
                .then(response => response.text())
                .then(data => {
                    console.log('Pattern updated:', data);
                    if (!selectedPattern.toLowerCase().includes('draw') && 
                        !selectedPattern.toLowerCase().includes('video')) {
                        // Wait 1 second before refreshing preview to allow pattern to initialize
                        setTimeout(() => {
                            refreshPreview();
                        }, 1000);
                    }
                })
                .catch(error => console.error('Error:', error));
          }

          // Initialize preview on page load
          document.addEventListener('DOMContentLoaded', function() {
            updatePattern(document.getElementById('pattern').value);
          });
        </script>
      </body>
      </html>
    )rawliteral";

    // Send response
    request->send(200, "text/html", html);
  });
}

// -------------------------------------------------------------------
// Handler for /pattern?value=X
// -------------------------------------------------------------------
static void setupPatternHandler() {
  server.on("/pattern", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String patternStr = request->getParam("value")->value();
      int pattern = patternStr.toInt();
      
      if (pattern >= 0 && pattern < PATTERN_COUNT) {
        g_current_pattern_number = pattern;
        request->send(200, "text/plain", "Pattern updated");
      } else {
        request->send(400, "text/plain", "Invalid pattern number");
      }
    } else {
      request->send(400, "text/plain", "Missing value parameter");
    }
  });
}

// -------------------------------------------------------------------
// Handler for /brightness?value=X
// -------------------------------------------------------------------
static void setupBrightnessHandler() {
  server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String brightnessStr = request->getParam("value")->value();
      int brightness = brightnessStr.toInt();
      
      if (brightness >= 0 && brightness <= 255) {
        g_Brightness = brightness;
        request->send(200, "text/plain", "Brightness updated");
      } else {
        request->send(400, "text/plain", "Invalid brightness value");
      }
    } else {
      request->send(400, "text/plain", "Missing value parameter");
    }
  });
}

// -------------------------------------------------------------------
// Handler for /speed?value=X
// -------------------------------------------------------------------
static void setupSpeedHandler() {
  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String speedStr = request->getParam("value")->value();
      int speed = speedStr.toInt();
      
      if (speed >= 0 && speed <= 255) {
        g_Speed = speed;
        request->send(200, "text/plain", "Speed updated");
      } else {
        request->send(400, "text/plain", "Invalid speed value");
      }
    } else {
      request->send(400, "text/plain", "Missing value parameter");
    }
  });
}

// -------------------------------------------------------------------
// Handler for /pixelStatus - returns current LED states
// -------------------------------------------------------------------
static void setupPixelStatusHandler() {
  server.on("/pixelStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern CRGB leds[];
    
    // Pre-allocate fixed size buffer (256 pixels * 3 bytes per pixel)
    static uint8_t response[768];  // RGB values only, no terminator needed
    uint8_t* ptr = response;
    
    // Process 16x16 grid with serpentine pattern
    for (int y = 0; y < 16; y++) {
      const bool isEvenRow = (y & 1) == 0;
      const int rowStart = y << 4;  // y * 16
      
      for (int x = 0; x < 16; x++) {
        // Calculate LED index - even rows reversed
        const int ledIndex = rowStart + (isEvenRow ? 15 - x : x);
        
        // Direct binary copy - no conversion needed
        *ptr++ = leds[ledIndex].r;
        *ptr++ = leds[ledIndex].g;
        *ptr++ = leds[ledIndex].b;
      }
    }
    
    AsyncWebServerResponse *response_obj = request->beginResponse_P(200, "application/octet-stream", response, 768);
    response_obj->addHeader("Cache-Control", "no-store");
    request->send(response_obj);
  });
}

// -------------------------------------------------------------------
// Start the "normal" server (station mode only)
// -------------------------------------------------------------------
static void startServer() {
  setupHomePage();
  setupPatternHandler();
  setupBrightnessHandler();
  setupSpeedHandler();
  setupPixelStatusHandler();  // Add the new handler
  
  // Serve the libgif.js file from SPIFFS
  server.begin();
  Serial.println("[WiFi] Normal Web server started on port 80");
}

// -------------------------------------------------------------------
// wifiServerSetup - main entry point to do WiFi + server setup
// -------------------------------------------------------------------
void wifiServerSetup() {
  // Attempt connecting or start AP if no creds
  connectToWiFi();

  // If we ended up in AP mode, we're serving `onboardServer`.
  // If we have a successful STA connection, serve normal pages:
  if (WiFi.status() == WL_CONNECTED) {
    setupMDNS();
    setupHomePage();
    setupPatternHandler();
    setupBrightnessHandler();
    setupSpeedHandler();
    setupVideoPlayer(&server);
    setupDrawPattern(&server);
    startServer();

    // Debug: List files in SPIFFS
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        String output = "";
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file) {
            output += "File: ";
            output += file.name();
            output += " Size: ";
            output += file.size();
            output += "\n";
            file = root.openNextFile();
        }
        request->send(200, "text/plain", output);
    });

    // Serve static files with error handling
    server.serveStatic("/static/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=600");
  } else {
    Serial.println("[WiFi] Running in AP mode for onboarding.");
  }
}

// -------------------------------------------------------------------
// wifiLoop - rarely needed if using AsyncWebServer. 
// Typically empty, but you can place background tasks here if needed.
// -------------------------------------------------------------------
void wifiLoop() {
  // No-op for now
}