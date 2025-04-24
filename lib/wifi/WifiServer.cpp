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
#include "type/type.h"         // For setupTypePattern
#include "snake/snake.h"       // For setupSnakePattern
#include "clock/clock.h"       // For setupClockPattern
#include "SPIFFS.h"
#include "tetris/tetris.h"    // Add Tetris setup declaration

#if ENABLE_MICROPHONE
#include "audio/audio.h"      // Add audio pattern header
#endif

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
static int g_PreviewInterval = 100;        // Default 100ms for preview updates

// Provided by patterns.h
extern Pattern g_patternList[];
extern const size_t PATTERN_COUNT;

// -------------------------------------------------------------------
// We'll store Wi-Fi credentials and settings in Preferences
// -------------------------------------------------------------------
static String g_wifi_ssid;
static String g_wifi_password;
static bool g_hasCredentials = false;

// Forward declarations
static void loadCredentials();
static void saveCredentials(const String& ssid, const String& pass);
static void loadSettings();
static void saveBrightness(int brightness);
static void saveSpeed(int speed);
static void startAccessPoint();
static void connectToWiFi();
static void setupMDNS();
static void setupHomePage();
static void setupPatternHandler();
static void setupBrightnessHandler();
static void setupSpeedHandler();
static void setupPixelStatusHandler();
static void setupFaviconHandler();  // Add favicon handler declaration
static void startServer();
static void savePreviewInterval(int interval);

// Forward declarations for pattern setup functions
void setupDrawPattern(AsyncWebServer* server);
void setupVideoPlayer(AsyncWebServer* server);
void setupTypePattern(AsyncWebServer* server);
void setupSnakePattern(AsyncWebServer* server);
void setupTetrisPattern(AsyncWebServer* server);  // Add Tetris setup declaration
void setupClockPattern(AsyncWebServer* server);      // Add Clock setup declaration
void setupAudioPattern(AsyncWebServer* server);  // Add audio pattern setup

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
// Load settings from NVS
// -------------------------------------------------------------------
static void loadSettings() {
  Preferences prefs;
  prefs.begin("settings", true);  // read-only
  g_Brightness = prefs.getInt("brightness", 128);     // Default to 128 if not set
  g_Speed = prefs.getInt("speed", 128);              // Default to 128 if not set
  g_PreviewInterval = prefs.getInt("preview_interval", 100);  // Default to 100ms if not set
  prefs.end();
}

// -------------------------------------------------------------------
// Save brightness to NVS
// -------------------------------------------------------------------
static void saveBrightness(int brightness) {
  Preferences prefs;
  prefs.begin("settings", false); // read/write
  prefs.putInt("brightness", brightness);
  prefs.end();
}

// -------------------------------------------------------------------
// Save speed to NVS
// -------------------------------------------------------------------
static void saveSpeed(int speed) {
  Preferences prefs;
  prefs.begin("settings", false); // read/write
  prefs.putInt("speed", speed);
  prefs.end();
}

// -------------------------------------------------------------------
// Save preview interval to NVS
// -------------------------------------------------------------------
static void savePreviewInterval(int interval) {
  Preferences prefs;
  prefs.begin("settings", false); // read/write
  prefs.putInt("preview_interval", interval);
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
        <meta charset="UTF-8">
        <title>PixelBoard Control</title>
        <script type="text/javascript" src="/static/libgif.js"></script>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <link rel="stylesheet" href="/style.css">
        <link rel="icon" type="image/x-icon" href="/favicon.ico">
      </head>
      <body>
        <div class="mobile-tabs">
          <div class="tab-buttons">
            <button class="tab-button active" data-tab="pattern">Pattern</button>
            <button class="tab-button" data-tab="preview">Preview</button>
          </div>
        </div>
        <div class="page-layout">
          <div class="controls-panel">
            <div class="resize-handle"></div>
            <div class="main-controls">
              <h1>PixelBoard Control</h1>
              <div class="pattern-grid">
    )rawliteral";

    // Add pattern options as grid items
    for (size_t i = 0; i < PATTERN_COUNT; i++) {
      html += "<div class=\"pattern-item" + String(i == g_current_pattern_number ? " selected" : "") + 
              "\" onclick=\"updatePattern(" + String(i) + ")\">" +
              "<div class=\"pattern-icon\">" + String(g_patternList[i].icon) + "</div>" +
              "<div class=\"pattern-name\">" + String(g_patternList[i].name) + "</div>" +
              "</div>";
    }

    html += R"rawliteral(
              </div>
            </div>
          </div>
          <div class="preview-panel" id="previewPanel">
            <!-- Preview content will be dynamically inserted here -->
          </div>
        </div>

        <!-- Settings Modal -->
        <button class="settings-button" onclick="openModal()">⚙️</button>
        <div class="modal" id="settingsModal">
          <div class="modal-content">
            <div class="modal-header">
              <h2>Settings & Preview Controls</h2>
              <button class="close-modal" onclick="closeModal()">&times;</button>
            </div>
            
            <div class="modal-section">
              <h3>Pattern Controls</h3>
              <div class="slider-container">
                <label for="brightness">Brightness: <span id="brightnessValue">)rawliteral";
    
    html += String(g_Brightness);
    
    html += R"rawliteral(</span></label>
                <input type="range" min="0" max="255" value=")rawliteral";
    
    html += String(g_Brightness);
    
    html += R"rawliteral(" class="slider" id="brightness" oninput="updateBrightness(this.value)">
              </div>

              <div class="slider-container">
                <label for="speed">Speed: <span id="speedValue">)rawliteral";
    
    html += String(g_Speed);
    
    html += R"rawliteral(</span></label>
                <input type="range" min="0" max="255" value=")rawliteral";
    
    html += String(g_Speed);
    
    html += R"rawliteral(" class="slider" id="speed" oninput="updateSpeed(this.value)">
              </div>
            </div>

            <div class="modal-section">
              <h3>Preview Settings</h3>
              <div class="slider-container">
                <label for="previewSpeed">Update Interval: <span id="previewSpeedValue">)rawliteral";

    html += String(g_PreviewInterval) + "ms";

    html += R"rawliteral(</span></label>
                <input type="range" min="10" max="10000" value=")rawliteral";

    html += String(g_PreviewInterval);

    html += R"rawliteral(" class="slider" id="previewSpeed" oninput="updatePreviewSpeed(this.value)">
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
        </div>

        <script>
          let previewUpdateInterval;
          let isPaused = false;
          let currentUpdateInterval = )rawliteral";

    html += String(g_PreviewInterval);

    html += R"rawliteral(; // Use server-side interval
          let isUpdating = false; // Flag to prevent concurrent requests
          let currentTab = 'pattern';

          // Mobile tab handling
          document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', () => {
              // Update active tab button
              document.querySelectorAll('.tab-button').forEach(btn => btn.classList.remove('active'));
              button.classList.add('active');
              
              // Update current tab
              currentTab = button.dataset.tab;
              
              // Show/hide panels based on selected tab
              const controlsPanel = document.querySelector('.controls-panel');
              const previewPanel = document.querySelector('.preview-panel');
              
              if (currentTab === 'pattern') {
                controlsPanel.style.display = 'flex';
                previewPanel.style.display = 'none';
              } else {
                controlsPanel.style.display = 'none';
                previewPanel.style.display = 'flex';
              }
            });
          });

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
            
            // Save the new interval to the server
            fetch('/previewInterval?value=' + value)
              .then(response => response.text())
              .then(data => console.log('Preview interval updated:', data))
              .catch(error => console.error('Error:', error));
            
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
            // Update selected pattern in grid
            document.querySelectorAll('.pattern-item').forEach(item => {
              item.classList.remove('selected');
            });
            document.querySelector(`.pattern-item[onclick*="updatePattern(${value})"]`).classList.add('selected');
            
            const previewPanel = document.getElementById('previewPanel');
            
            // Clear existing content
            previewPanel.innerHTML = '';
            
            // Clear any existing interval
            if (previewUpdateInterval) {
                clearInterval(previewUpdateInterval);
            }
            
            // Get pattern name from selected item
            const selectedName = document.querySelector(`.pattern-item[onclick*="updatePattern(${value})"] .pattern-name`).textContent;
            
            // Handle special patterns
            if (selectedName.toLowerCase().includes('draw')) {
                // Load draw interface
                const iframe = document.createElement('iframe');
                iframe.src = '/draw';
                previewPanel.appendChild(iframe);
            } else if (selectedName.toLowerCase().includes('video')) {
                // Load video interface
                const iframe = document.createElement('iframe');
                iframe.src = '/video';
                previewPanel.appendChild(iframe);
            } else if (selectedName.toLowerCase().includes('text') || selectedName.toLowerCase().includes('type')) {
                // Load text/type interface
                const iframe = document.createElement('iframe');
                iframe.src = '/type';
                previewPanel.appendChild(iframe);
            } else if (selectedName.toLowerCase().includes('snake')) {
                // Load snake game interface
                const iframe = document.createElement('iframe');
                iframe.src = '/snake';
                previewPanel.appendChild(iframe);
            } else if (selectedName.toLowerCase().includes('tetris')) {
                // Load tetris game interface
                const iframe = document.createElement('iframe');
                iframe.src = '/tetris';
                previewPanel.appendChild(iframe);
            } else if (selectedName.toLowerCase().includes('clock')) {
                // Load clock game interface
                const iframe = document.createElement('iframe');
                iframe.src = '/clock';
                previewPanel.appendChild(iframe);
            } else {
                // Start preview updates for regular patterns
                startPreviewUpdates();
            }

            fetch('/pattern?value=' + value)
                .then(response => response.text())
                .then(data => {
                    console.log('Pattern updated:', data);
                    if (!selectedName.toLowerCase().includes('draw') && 
                        !selectedName.toLowerCase().includes('video') &&
                        !selectedName.toLowerCase().includes('text') &&
                        !selectedName.toLowerCase().includes('type') &&
                        !selectedName.toLowerCase().includes('snake') &&
                        !selectedName.toLowerCase().includes('tetris') &&
                        !selectedName.toLowerCase().includes('clock')) {
                        // Wait 1 second before refreshing preview to allow pattern to initialize
                        setTimeout(() => {
                            refreshPreview();
                        }, 1000);
                    }
                })
                .catch(error => console.error('Error:', error));
          }

          function openModal() {
            document.getElementById('settingsModal').classList.add('show');
          }

          function closeModal() {
            document.getElementById('settingsModal').classList.remove('show');
          }

          // Close modal when clicking outside
          document.addEventListener('click', function(event) {
            const modal = document.getElementById('settingsModal');
            const modalContent = modal.querySelector('.modal-content');
            const settingsButton = document.querySelector('.settings-button');
            
            if (event.target === modal && !modalContent.contains(event.target) && !settingsButton.contains(event.target)) {
              closeModal();
            }
          });

          // Initialize preview on page load
          document.addEventListener('DOMContentLoaded', function() {
            // Initialize resizable controls panel
            const controlsPanel = document.querySelector('.controls-panel');
            const resizeHandle = document.querySelector('.resize-handle');
            let isResizing = false;
            let startX;
            let startWidth;

            resizeHandle.addEventListener('mousedown', function(e) {
              isResizing = true;
              startX = e.pageX;
              startWidth = parseInt(document.defaultView.getComputedStyle(controlsPanel).width, 10);
              resizeHandle.classList.add('active');
              e.preventDefault(); // Prevent default selection behavior
            });

            document.addEventListener('mousemove', function(e) {
              if (!isResizing) return;
              
              const width = startWidth + (e.pageX - startX);
              // Ensure minimum width of 150px
              if (width >= 150) {
                controlsPanel.style.width = width + 'px';
              }
              e.preventDefault(); // Prevent default selection behavior
            });

            document.addEventListener('mouseup', function(e) {
              isResizing = false;
              resizeHandle.classList.remove('active');
              document.body.style.cursor = 'default';
            });

            // Initialize pattern selection with current pattern number
            updatePattern()rawliteral";

    html += String(g_current_pattern_number);

    html += R"rawliteral();
          });
        </script>
      </body>
      </html>
    )rawliteral";

    // Send response
    request->send(200, "text/html; charset=utf-8", html);
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
        saveBrightness(brightness);  // Save to NVS
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
        saveSpeed(speed);  // Save to NVS
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
    
    AsyncWebServerResponse *response_obj = request->beginResponse(200, "application/octet-stream", response, 768);
    response_obj->addHeader("Cache-Control", "no-store");
    request->send(response_obj);
  });
}

// -------------------------------------------------------------------
// Handler for /previewInterval?value=X
// -------------------------------------------------------------------
static void setupPreviewIntervalHandler() {
  server.on("/previewInterval", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String intervalStr = request->getParam("value")->value();
      int interval = intervalStr.toInt();
      
      if (interval >= 10 && interval <= 10000) {
        g_PreviewInterval = interval;
        savePreviewInterval(interval);  // Save to NVS
        request->send(200, "text/plain", "Preview interval updated");
      } else {
        request->send(400, "text/plain", "Invalid interval value");
      }
    } else {
      request->send(400, "text/plain", "Missing value parameter");
    }
  });
}

// -------------------------------------------------------------------
// Handler for /style.css - returns shared CSS styles
// -------------------------------------------------------------------
static void setupStyleHandler() {
  server.serveStatic("/style.css", SPIFFS, "/style.css")
        .setCacheControl("public, max-age=31536000");
}

// -------------------------------------------------------------------
// Handler for /favicon.ico - returns current LED state as favicon
// -------------------------------------------------------------------
static void setupFaviconHandler() {
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern CRGB leds[];
    
    // ICO file format requires:
    // - 6 bytes for ICO header
    // - 16 bytes for ICO directory entry
    // - Bitmap header (40 bytes)
    // - Color data (16x16x4 bytes)
    static uint8_t icoFile[6 + 16 + 40 + (16 * 16 * 4)];
    uint8_t* ptr = icoFile;
    
    // ICO Header (6 bytes)
    *ptr++ = 0x00;  // Reserved, must be 0
    *ptr++ = 0x00;  // Reserved, must be 0
    *ptr++ = 0x01;  // Type (1 for ICO)
    *ptr++ = 0x00;  // Type
    *ptr++ = 0x01;  // Number of images
    *ptr++ = 0x00;  // Number of images
    
    // ICO Directory Entry (16 bytes)
    *ptr++ = 16;    // Width
    *ptr++ = 16;    // Height
    *ptr++ = 0;     // Color palette size (0 for no palette)
    *ptr++ = 0;     // Reserved, must be 0
    *ptr++ = 1;     // Color planes
    *ptr++ = 0;     // Color planes
    *ptr++ = 32;    // Bits per pixel
    *ptr++ = 0;     // Bits per pixel
    uint32_t size = 40 + (16 * 16 * 4);  // Size of bitmap data + header
    *ptr++ = size & 0xFF;          // Image size in bytes
    *ptr++ = (size >> 8) & 0xFF;
    *ptr++ = (size >> 16) & 0xFF;
    *ptr++ = (size >> 24) & 0xFF;
    *ptr++ = 22;    // Offset to start of image data (6 + 16)
    *ptr++ = 0;     // Offset
    *ptr++ = 0;     // Offset
    *ptr++ = 0;     // Offset
    
    // Bitmap Header (40 bytes)
    *ptr++ = 40;    // Header size
    *ptr++ = 0;     // Header size
    *ptr++ = 0;     // Header size
    *ptr++ = 0;     // Header size
    *ptr++ = 16;    // Width
    *ptr++ = 0;     // Width
    *ptr++ = 0;     // Width
    *ptr++ = 0;     // Width
    *ptr++ = 32;    // Height (16 * 2 because ICO format requires doubled height)
    *ptr++ = 0;     // Height
    *ptr++ = 0;     // Height
    *ptr++ = 0;     // Height
    *ptr++ = 1;     // Planes
    *ptr++ = 0;     // Planes
    *ptr++ = 32;    // Bits per pixel
    *ptr++ = 0;     // Bits per pixel
    for (int i = 0; i < 24; i++) {  // Rest of bitmap header (compression, etc.)
      *ptr++ = 0;
    }
    
    // Image data (bottom-up, BGRA format)
    for (int y = 15; y >= 0; y--) {  // ICO format requires bottom-up pixel data
      const bool isEvenRow = (y & 1) == 0;
      const int rowStart = y << 4;  // y * 16
      
      for (int x = 0; x < 16; x++) {
        // Calculate LED index - even rows reversed
        const int ledIndex = rowStart + (isEvenRow ? 15 - x : x);
        
        // Write in BGRA format (ICO/BMP expects BGR)
        *ptr++ = leds[ledIndex].b;  // Blue
        *ptr++ = leds[ledIndex].g;  // Green
        *ptr++ = leds[ledIndex].r;  // Red
        *ptr++ = 255;               // Alpha
      }
    }
    
    AsyncWebServerResponse *response = request->beginResponse(200, "image/x-icon", icoFile, sizeof(icoFile));
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });
}

// -------------------------------------------------------------------
// Start the "normal" server (station mode only)
// -------------------------------------------------------------------
static void startServer() {
  // Setup all the web handlers
  setupHomePage();
  setupPatternHandler();
  setupBrightnessHandler();
  setupSpeedHandler();
  setupPixelStatusHandler();
  setupPreviewIntervalHandler();
  setupStyleHandler();
  setupFaviconHandler();

  // Setup pattern-specific handlers
  setupDrawPattern(&server);
  setupVideoPlayer(&server);
  setupTypePattern(&server);
  setupSnakePattern(&server);
  setupTetrisPattern(&server);
  setupClockPattern(&server);
#if ENABLE_MICROPHONE
  setupAudioPattern(&server);
#endif

  // Start server
  server.begin();
  Serial.println("[Server] HTTP server started");
}

// -------------------------------------------------------------------
// wifiServerSetup - main entry point to do WiFi + server setup
// -------------------------------------------------------------------
void wifiServerSetup() {
  // Load settings before starting server
  loadSettings();
  
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
    setupTypePattern(&server);
    setupSnakePattern(&server);
    setupTetrisPattern(&server);
    setupClockPattern(&server);
#if ENABLE_MICROPHONE
    setupAudioPattern(&server);
#endif
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