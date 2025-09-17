/*
 * ESP8266-01 Web Controller for Robot (Arduino IDE)
 * 
 * Hardware Connections:
 * ESP8266 TX -> Arduino Mega Pin 17 (RX2)
 * ESP8266 RX -> Arduino Mega Pin 16 (TX2)
 * ESP8266 VCC -> 3.3V (REGULATED!)
 * ESP8266 GND -> GND
 * 
 * WiFi Access Point:
 * SSID: RobotController
 * Password: robot123
 * IP: 192.168.4.1
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ================ CONFIGURATION ================
const char* AP_SSID = "RobotController";
const char* AP_PASSWORD = "robot123";
const unsigned long SERIAL_BAUD = 115200;
const unsigned long STATUS_UPDATE_INTERVAL = 800;
const int MAX_COMMAND_LENGTH = 200;
const unsigned long CONNECTION_TIMEOUT = 5000;

// Web server
ESP8266WebServer server(80);

// Globals
String rxBuffer = "";
String lastOdomData = "";
unsigned long lastStatusUpdate = 0;
unsigned long lastRobotResponse = 0;
bool robotConnected = false;
bool motorsEnabled = false;

// ================ WEB INTERFACE HTML ================
const char* getWebPage() {
  static const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Controller</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0; padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white; min-height: 100vh;
        }
        .container {
            max-width: 800px; margin: 0 auto;
            background: rgba(255,255,255,0.1);
            border-radius: 20px; padding: 30px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
        }
        h1 { text-align: center; margin-bottom: 30px; font-size: 2.5em; }
        .status-panel {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px; margin-bottom: 30px;
        }
        .status-item {
            background: rgba(255,255,255,0.2);
            padding: 15px; border-radius: 10px; text-align: center;
            transition: all 0.3s ease;
        }
        .status-value { 
            font-size: 1.5em; font-weight: bold; margin-top: 5px; 
            transition: color 0.3s ease;
        }
        .status-connected { color: #4CAF50; }
        .status-disconnected { color: #f44336; }
        .status-enabled { color: #4CAF50; }
        .status-disabled { color: #ff9800; }
        .controls-grid {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 30px; margin-bottom: 30px;
        }
        .control-section {
            background: rgba(255,255,255,0.1);
            padding: 20px; border-radius: 15px;
        }
        .control-section h3 { margin-top: 0; text-align: center; }
        .direction-pad {
            display: grid; grid-template-areas:
                ". up ."
                "left stop right"
                ". down .";
            gap: 10px; max-width: 200px; margin: 0 auto;
        }
        .dir-btn { 
            padding: 15px; border: none; border-radius: 10px;
            background: rgba(255,255,255,0.3); color: white;
            font-size: 16px; cursor: pointer; transition: all 0.3s;
            user-select: none;
        }
        .dir-btn:hover { background: rgba(255,255,255,0.5); transform: scale(1.05); }
        .dir-btn:active { transform: scale(0.95); }
        .dir-btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .up { grid-area: up; } .down { grid-area: down; }
        .left { grid-area: left; } .right { grid-area: right; }
        .stop { grid-area: stop; background: rgba(255,100,100,0.6); }
        .speed-control { text-align: center; margin: 20px 0; }
        .speed-slider {
            width: 100%; height: 8px; border-radius: 5px;
            background: rgba(255,255,255,0.3); outline: none;
            -webkit-appearance: none;
        }
        .speed-slider::-webkit-slider-thumb {
            -webkit-appearance: none; width: 20px; height: 20px;
            border-radius: 50%; background: white; cursor: pointer;
        }
        .command-section { margin-top: 20px; }
        .command-input {
            width: 70%; padding: 10px; border: none; border-radius: 5px;
            background: rgba(255,255,255,0.2); color: white;
        }
        .command-input::placeholder { color: rgba(255,255,255,0.7); }
        .btn {
            padding: 10px 20px; border: none; border-radius: 5px;
            background: rgba(255,255,255,0.3); color: white;
            cursor: pointer; transition: all 0.3s; margin: 5px;
            user-select: none;
        }
        .btn:hover { background: rgba(255,255,255,0.5); }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .btn-danger { background: rgba(255,100,100,0.6); }
        .btn-success { background: rgba(100,255,100,0.6); }
        .odometry-display {
            background: rgba(0,0,0,0.3); padding: 15px; border-radius: 10px;
            font-family: monospace; font-size: 14px; margin-top: 20px;
            max-height: 200px; overflow-y: auto;
            white-space: pre-wrap;
        }
        .status-indicator {
            display: inline-block;
            width: 12px; height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .indicator-green { background-color: #4CAF50; }
        .indicator-red { background-color: #f44336; }
        .indicator-orange { background-color: #ff9800; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 Robot Controller</h1>
        
        <div class="status-panel">
            <div class="status-item">
                <div>Connection</div>
                <div class="status-value" id="connectionStatus">
                    <span class="status-indicator" id="connIndicator"></span>Checking...
                </div>
            </div>
            <div class="status-item">
                <div>Motors</div>
                <div class="status-value" id="motorStatus">
                    <span class="status-indicator" id="motorIndicator"></span>Unknown
                </div>
            </div>
            <div class="status-item">
                <div>Speed</div>
                <div class="status-value" id="speedDisplay">150</div>
            </div>
        </div>

        <div class="controls-grid">
            <div class="control-section">
                <h3>Movement Control</h3>
                <div class="direction-pad">
                    <button class="dir-btn up" onclick="sendCommand('FWD')">↑</button>
                    <button class="dir-btn left" onclick="sendCommand('LEFT')">←</button>
                    <button class="dir-btn stop" onclick="sendCommand('STOP')">STOP</button>
                    <button class="dir-btn right" onclick="sendCommand('RIGHT')">→</button>
                    <button class="dir-btn down" onclick="sendCommand('BACK')">↓</button>
                </div>
                
                <div class="speed-control">
                    <label for="speedSlider">Speed: <span id="speedValue">150</span></label><br>
                    <input type="range" id="speedSlider" class="speed-slider" 
                           min="50" max="255" value="150" 
                           oninput="updateSpeed(this.value)">
                </div>
            </div>

            <div class="control-section">
                <h3>System Control</h3>
                <button class="btn btn-success" id="enableBtn" onclick="sendCommand('ENABLE')">Enable Motors</button>
                <button class="btn btn-danger" id="disableBtn" onclick="sendCommand('DISABLE')">Disable Motors</button>
                <button class="btn" onclick="sendCommand('REQ_ODOM')">Request Odometry</button>
                
                <div class="command-section">
                    <h4>Manual Command</h4>
                    <input type="text" id="manualCommand" class="command-input" 
                           placeholder="Enter command (e.g., M1 100)" 
                           onkeypress="if(event.key==='Enter') sendManualCommand()">
                    <button class="btn" onclick="sendManualCommand()">Send</button>
                </div>
            </div>
        </div>

        <div class="control-section">
            <h3>Odometry Data</h3>
            <div id="odomDisplay" class="odometry-display">
                Waiting for odometry data...
            </div>
        </div>
    </div>

    <script>
        let currentSpeed = 150;
        // Keyboard controls
        document.addEventListener('keydown', function(e) {
            if (e.target.tagName === 'INPUT') return;
            switch(e.key.toLowerCase()) {
                case 'w': case 'arrowup': sendCommand('FWD'); e.preventDefault(); break;
                case 's': case 'arrowdown': sendCommand('BACK'); e.preventDefault(); break;
                case 'a': case 'arrowleft': sendCommand('LEFT'); e.preventDefault(); break;
                case 'd': case 'arrowright': sendCommand('RIGHT'); e.preventDefault(); break;
                case ' ': case 'escape': sendCommand('STOP'); e.preventDefault(); break;
            }
        });
        function updateSpeed(value) {
            currentSpeed = value;
            document.getElementById('speedValue').textContent = value;
            document.getElementById('speedDisplay').textContent = value;
        }
        function sendCommand(cmd) {
            let fullCmd = cmd;
            if (['FWD', 'BACK', 'LEFT', 'RIGHT'].includes(cmd)) {
                fullCmd = cmd + ' ' + currentSpeed;
            }
            fetch('/command', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'cmd=' + encodeURIComponent(fullCmd)
            }).then(response => response.text())
              .then(data => setTimeout(updateStatus, 100));
        }
        function sendManualCommand() {
            const input = document.getElementById('manualCommand');
            if (input.value.trim()) {
                sendCommand(input.value.trim());
                input.value = '';
            }
        }
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    // Connection
                    const connStatus = document.getElementById('connectionStatus');
                    const connIndicator = document.getElementById('connIndicator');
                    if (data.connected) {
                        connStatus.innerHTML = '<span class="status-indicator indicator-green"></span>Connected';
                        connIndicator.className = 'status-indicator indicator-green';
                    } else {
                        connStatus.innerHTML = '<span class="status-indicator indicator-red"></span>Disconnected';
                        connIndicator.className = 'status-indicator indicator-red';
                    }
                    // Motors
                    const motorStatus = document.getElementById('motorStatus');
                    const motorIndicator = document.getElementById('motorIndicator');
                    const enableBtn = document.getElementById('enableBtn');
                    const disableBtn = document.getElementById('disableBtn');
                    if (data.motorsEnabled) {
                        motorStatus.innerHTML = '<span class="status-indicator indicator-green"></span>Enabled';
                        motorIndicator.className = 'status-indicator indicator-green';
                        enableBtn.style.opacity = '0.5';
                        disableBtn.style.opacity = '1';
                    } else {
                        motorStatus.innerHTML = '<span class="status-indicator indicator-orange"></span>Disabled';
                        motorIndicator.className = 'status-indicator indicator-orange';
                        enableBtn.style.opacity = '1';
                        disableBtn.style.opacity = '0.5';
                    }
                    // Controls
                    const dirButtons = document.querySelectorAll('.dir-btn');
                    dirButtons.forEach(btn => { btn.disabled = !data.connected; });
                    // Odometry
                    if (data.odometry && data.odometry.length > 0) {
                        const odomDisplay = document.getElementById('odomDisplay');
                        const lines = odomDisplay.textContent.split('\n');
                        lines.push(new Date().toLocaleTimeString() + ': ' + data.odometry);
                        if (lines.length > 10) lines.shift();
                        odomDisplay.textContent = lines.join('\n');
                        odomDisplay.scrollTop = odomDisplay.scrollHeight;
                    }
                })
                .catch(err => {
                    document.getElementById('connectionStatus').innerHTML = 
                        '<span class="status-indicator indicator-red"></span>Error';
                });
        }
        setInterval(updateStatus, 800);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";
  return webpage;
}

// ================ ROBOT COMMUNICATION ================
void sendToRobot(const String& command) {
  Serial.println(command);
  Serial.flush();
}
String readFromRobot(unsigned long timeout_ms = 1500) {
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < timeout_ms) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (response.length() > 0) return response;
      } else {
        response += c;
        if (response.length() > MAX_COMMAND_LENGTH) response = "";
      }
    }
    yield();
  }
  return "";
}
void handleRobotMessage(const String& message) {
  lastRobotResponse = millis();
  robotConnected = true;
  if (message.startsWith("ODOM ")) lastOdomData = message;
  else if (message.indexOf("OK ENABLE") >= 0) motorsEnabled = true;
  else if (message.indexOf("OK DISABLE") >= 0) motorsEnabled = false;
}
void processRobotResponse() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (rxBuffer.length() > 0) {
        handleRobotMessage(rxBuffer);
        rxBuffer = "";
      }
    } else {
      rxBuffer += c;
      if (rxBuffer.length() > MAX_COMMAND_LENGTH) rxBuffer = "";
    }
  }
}
bool checkRobotConnection() {
  sendToRobot("REQ_ODOM");
  delay(100);
  String response = readFromRobot(1500);
  if (response.length() > 0) {
    handleRobotMessage(response);
    return true;
  }
  if (millis() - lastRobotResponse > CONNECTION_TIMEOUT) robotConnected = false;
  return robotConnected;
}

// ================ WEB SERVER HANDLERS ================
void handleRoot() { server.send(200, "text/html", getWebPage()); }
void handleCommand() {
  if (server.hasArg("cmd")) {
    String command = server.arg("cmd");
    sendToRobot(command);
    delay(50);
    processRobotResponse();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "ERR");
  }
}
void handleStatus() {
  processRobotResponse();
  String json = "{";
  json += "\"connected\":" + String(robotConnected ? "true" : "false") + ",";
  json += "\"motorsEnabled\":" + String(motorsEnabled ? "true" : "false") + ",";
  json += "\"odometry\":\"" + lastOdomData + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
void handleNotFound() { server.send(404, "text/plain", "Not Found"); }

// ================ MAIN FUNCTIONS ================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  while (Serial.available()) Serial.read();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  server.on("/", handleRoot);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  lastRobotResponse = millis();
  lastStatusUpdate = millis();
  delay(2000);
  checkRobotConnection();
}
void loop() {
  server.handleClient();
  processRobotResponse();
  unsigned long now = millis();
  if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
    checkRobotConnection();
    lastStatusUpdate = now;
  }
  yield();
}