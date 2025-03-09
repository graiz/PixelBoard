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
        <title>PixelBoard GIF</title>
        <script type="text/javascript" src="/static/libgif.js"></script>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
          body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
            background-color: #282c34;
            color: #ffffff;
          }
          .container {
            max-width: 600px;
            width: 100%;
            padding: 20px;
            margin: 20px auto;
            background: #3b3f47;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
          }
          h1 {
            margin-bottom: 20px;
            text-align: center;
            font-size: 24px;
            color: #61dafb;
          }
          label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
          }
          select, button {
            width: 100%;
            padding: 10px;
            margin-bottom: 20px;
            border: none;
            border-radius: 5px;
            background: #61dafb;
            color: #000;
            font-size: 16px;
            cursor: pointer;
          }
          button:hover {
            background: #21a1f1;
          }
          .note {
            font-size: 12px;
            text-align: center;
            margin-top: -10px;
            color: #bbb;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>PixelBoard Control</h1>
          <label for="pattern_select">Select Pattern:</label>
          <select id="pattern_select" onchange="changePattern()">
    )rawliteral";

    // Add pattern options
    for (size_t i = 0; i < PATTERN_COUNT; i++) {
      html += "<option value=\"" + String(i) + "\">" + String(g_patternList[i].name) + "</option>";
    }

    // Brightness
    html += R"rawliteral(
          </select>
          <label for="brightness_select">Select Brightness:</label>
          <select id="brightness_select" onchange="changeBrightness()">
    )rawliteral";

    int brightnessOptions[] = {0, 1, 3, 5, 10, 25, 50, 100, 150, 250, 255};
    for (int val : brightnessOptions) {
      html += "<option value=\"" + String(val) + "\">" + String(val) + "</option>";
    }

    // Speed
    html += R"rawliteral(
          </select>
          <label for="speed_select">Select Speed:</label>
          <select id="speed_select" onchange="changeSpeed()">
    )rawliteral";

    int speedOptions[] = {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2000};
    for (int val : speedOptions) {
      html += "<option value=\"" + String(val) + "\">" + String(val) + "</option>";
    }

    html += R"rawliteral(
          </select>
          <button onclick="changePattern()">Apply Changes</button>
          <p class="note">Use the controls above to update the PixelBoard.</p>
        </div>
        <script>
          function changePattern() {
            var e = document.getElementById('pattern_select');
            fetch('/set_pattern?pattern=' + e.value);
          }
          function changeBrightness() {
            var e = document.getElementById('brightness_select');
            fetch('/setBrightness?brightness=' + e.value);
          }
          function changeSpeed() {
            var e = document.getElementById('speed_select');
            fetch('/setSpeed?speed=' + e.value);
          }
        </script>
      </body>
      </html>
    )rawliteral";

    // Send response
    request->send(200, "text/html", html);
  });
}

// -------------------------------------------------------------------
// Handler for /set_pattern?pattern=X
// pattern = -1 => next pattern, -2 => random
// -------------------------------------------------------------------
static void setupPatternHandler() {
  server.on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("pattern")) {
      int new_pattern = request->arg("pattern").toInt();
      if (new_pattern >= 0 && (size_t)new_pattern < PATTERN_COUNT) {
        g_current_pattern_number = new_pattern;
      } else if (new_pattern == -1) {
        g_current_pattern_number = (g_current_pattern_number + 1) % PATTERN_COUNT;
      } else if (new_pattern == -2) {
        g_current_pattern_number = random(PATTERN_COUNT);
      }
    }
    request->send(200, "text/plain", "OK");
  });
}

// -------------------------------------------------------------------
// Handler for /setBrightness?brightness=X
// -------------------------------------------------------------------
static void setupBrightnessHandler() {
  server.on("/setBrightness", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->hasParam("brightness")) {
      String brightnessVal = request->getParam("brightness")->value();
      g_Brightness = brightnessVal.toInt();
    }
    request->redirect("/");
  });
}

// -------------------------------------------------------------------
// Handler for /setSpeed?speed=X
// -------------------------------------------------------------------
static void setupSpeedHandler() {
  server.on("/setSpeed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->hasParam("speed")) {
      String speedVal = request->getParam("speed")->value();
      g_Speed = speedVal.toInt();
    }
    request->redirect("/");
  });
}

// -------------------------------------------------------------------
// Start the "normal" server (station mode only)
// -------------------------------------------------------------------
static void startServer() {
  // Serve the libgif.js file from SPIFFS
  server.serveStatic("/libgif.js", SPIFFS, "/libgif.js");
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