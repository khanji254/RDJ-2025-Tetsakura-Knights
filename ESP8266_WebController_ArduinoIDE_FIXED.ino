/*
 * ESP8266-01 Web Controller for Robot - FIXED VERSION
 * Arduino IDE Version
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
 * 
 * FIXES:
 * - Motor enable/disable status now properly tracked from robot responses
 * - Better command acknowledgment handling
 * - Improved status updates
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ================ CONFIGURATION ================
const char* AP_SSID = "RobotController";
const char* AP_PASSWORD = "robot123";
const unsigned long SERIAL_BAUD = 115200;
const unsigned long COMMAND_TIMEOUT_MS = 5000;
const unsigned long STATUS_UPDATE_INTERVAL = 1000;
const int MAX_COMMAND_LENGTH = 200;

// Web server
ESP8266WebServer server(80);

// Global variables
String rxBuffer = "";
String lastOdomData = "";
unsigned long lastStatusUpdate = 0;
unsigned long lastRobotResponse = 0;
bool robotConnected = false;
bool motorsEnabled = false;  // Will be updated from robot responses

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
        }
        .status-value { font-size: 1.5em; font-weight: bold; margin-top: 5px; }
        .connected { color: #4ade80; }
        .disconnected { color: #f87171; }
        .enabled { color: #4ade80; }
        .disabled { color: #f87171; }
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
        .dir-btn { padding: 15px; border: none; border-radius: 10px;
            background: rgba(255,255,255,0.3); color: white;
            font-size: 16px; cursor: pointer; transition: all 0.3s;
        }
        .dir-btn:hover { background: rgba(255,255,255,0.5); transform: scale(1.05); }
        .dir-btn:active { transform: scale(0.95); }
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
        .system-controls { text-align: center; margin: 20px 0; }
        .btn {
            padding: 12px 24px; border: none; border-radius: 8px;
            cursor: pointer; transition: all 0.3s; margin: 5px;
            font-size: 16px; font-weight: bold;
        }
        .btn-enable { background: #10b981; color: white; }
        .btn-enable:hover { background: #059669; }
        .btn-disable { background: #ef4444; color: white; }
        .btn-disable:hover { background: #dc2626; }
        .btn-secondary { background: rgba(255,255,255,0.3); color: white; }
        .btn-secondary:hover { background: rgba(255,255,255,0.5); }
        .command-section { margin-top: 20px; }
        .command-input {
            width: 70%; padding: 10px; border: none; border-radius: 5px;
            background: rgba(255,255,255,0.2); color: white;
        }
        .command-input::placeholder { color: rgba(255,255,255,0.7); }
        .odometry-display {
            background: rgba(0,0,0,0.3); padding: 15px; border-radius: 10px;
            font-family: monospace; font-size: 14px; margin-top: 20px;
            max-height: 200px; overflow-y: auto;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 Robot Controller</h1>
        
        <div class="status-panel">
            <div class="status-item">
                <div>Connection</div>
                <div class="status-value" id="connectionStatus">Checking...</div>
            </div>
            <div class="status-item">
                <div>Motors</div>
                <div class="status-value" id="motorStatus">Unknown</div>
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
                <div class="system-controls">
                    <button class="btn btn-enable" onclick="sendCommand('ENABLE')">
                        ⚡ Enable Motors
                    </button>
                    <button class="btn btn-disable" onclick="sendCommand('DISABLE')">
                        ⏹️ Disable Motors
                    </button>
                    <button class="btn btn-secondary" onclick="sendCommand('REQ_ODOM')">
                        📊 Get Status
                    </button>
                </div>
                
                <div class="command-section">
                    <h4>Manual Command</h4>
                    <input type="text" id="manualCommand" class="command-input" 
                           placeholder="Enter command (e.g., M1 100)" 
                           onkeypress="if(event.key==='Enter') sendManualCommand()">
                    <button class="btn btn-secondary" onclick="sendManualCommand()">Send</button>
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
            if (e.target.tagName === 'INPUT') return; // Don't interfere with input fields
            
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
            
            console.log('Sending command:', fullCmd);
            
            fetch('/command', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'cmd=' + encodeURIComponent(fullCmd)
            })
            .then(response => response.text())
            .then(data => {
                console.log('Command response:', data);
                // Force immediate status update
                setTimeout(updateStatus, 100);
            })
            .catch(error => {
                console.error('Command error:', error);
            });
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
                    console.log('Status update:', data);
                    
                    // Update connection status
                    const connStatus = document.getElementById('connectionStatus');
                    if (data.connected) {
                        connStatus.textContent = '🟢 Connected';
                        connStatus.className = 'status-value connected';
                    } else {
                        connStatus.textContent = '🔴 Disconnected';
                        connStatus.className = 'status-value disconnected';
                    }
                    
                    // Update motor status
                    const motorStatus = document.getElementById('motorStatus');
                    if (data.motorsEnabled) {
                        motorStatus.textContent = '⚡ Enabled';
                        motorStatus.className = 'status-value enabled';
                    } else {
                        motorStatus.textContent = '⏹️ Disabled';
                        motorStatus.className = 'status-value disabled';
                    }
                    
                    // Update odometry
                    if (data.odometry && data.odometry.length > 0) {
                        document.getElementById('odomDisplay').textContent = data.odometry;
                    }
                })
                .catch(err => {
                    console.error('Status error:', err);
                    document.getElementById('connectionStatus').textContent = '❌ Error';
                    document.getElementById('connectionStatus').className = 'status-value disconnected';
                });
        }
        
        // Update status every 800ms for more responsive feedback
        setInterval(updateStatus, 800);
        
        // Initial status update
        setTimeout(updateStatus, 500);
    </script>
</body>
</html>
)rawliteral";
  return webpage;
}

// ================ ROBOT COMMUNICATION ================
void sendToRobot(const String& command) {
  Serial.println("TX: " + command);
  Serial.println(command);
  Serial.flush();
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
      if (rxBuffer.length() > MAX_COMMAND_LENGTH) {
        rxBuffer = "";
      }
    }
  }
}

void handleRobotMessage(const String& message) {
  lastRobotResponse = millis();
  robotConnected = true;
  
  Serial.println("RX: " + message);
  
  if (message.startsWith("ODOM ")) {
    lastOdomData = message;
  } else if (message.startsWith("OK")) {
    // Process acknowledgments to update motor status
    if (message.indexOf("ENABLE") >= 0) {
      motorsEnabled = true;
      Serial.println("✅ Motors ENABLED confirmed");
    } else if (message.indexOf("DISABLE") >= 0) {
      motorsEnabled = false;
      Serial.println("⛔ Motors DISABLED confirmed");
    }
  } else if (message.startsWith("ERR")) {
    Serial.println("❌ Robot error: " + message);
  }
}

bool checkRobotConnection() {
  // Check if robot is still responding
  unsigned long now = millis();
  bool wasConnected = robotConnected;
  robotConnected = (now - lastRobotResponse < COMMAND_TIMEOUT_MS);
  
  if (wasConnected && !robotConnected) {
    Serial.println("❌ Robot connection lost");
  } else if (!wasConnected && robotConnected) {
    Serial.println("✅ Robot connection established");
  }
  
  return robotConnected;
}

// ================ WEB SERVER HANDLERS ================
void handleRoot() {
  server.send(200, "text/html", getWebPage());
}

void handleCommand() {
  if (server.hasArg("cmd")) {
    String command = server.arg("cmd");
    Serial.println("Web command received: " + command);
    sendToRobot(command);
    server.send(200, "text/plain", "Command sent: " + command);
  } else {
    server.send(400, "text/plain", "Missing command parameter");
  }
}

void handleStatus() {
  checkRobotConnection();
  
  String json = "{";
  json += "\"connected\":" + String(robotConnected ? "true" : "false") + ",";
  json += "\"motorsEnabled\":" + String(motorsEnabled ? "true" : "false") + ",";
  json += "\"odometry\":\"" + lastOdomData + "\",";
  json += "\"lastResponse\":" + String(lastRobotResponse) + ",";
  json += "\"uptime\":" + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page not found");
}

// ================ MAIN FUNCTIONS ================
void setup() {
  // Initialize serial communication with robot
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println("=== ESP8266 Robot Controller Starting ===");
  
  // Configure WiFi Access Point
  WiFi.mode(WIFI_AP);
  bool apResult = WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  if (apResult) {
    IPAddress myIP = WiFi.softAPIP();
    Serial.println("✅ Access Point started successfully");
    Serial.println("📶 SSID: " + String(AP_SSID));
    Serial.println("🔑 Password: " + String(AP_PASSWORD));
    Serial.println("🌐 IP Address: " + myIP.toString());
    Serial.println("👉 Open browser to: http://" + myIP.toString());
  } else {
    Serial.println("❌ Failed to start Access Point!");
  }
  
  // Configure web server routes
  server.on("/", handleRoot);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  
  // Start web server
  server.begin();
  Serial.println("🚀 Web server started");
  
  // Wait for Mega to boot, then send initial commands
  delay(3000);
  Serial.println("🔄 Initializing robot connection...");
  sendToRobot("ENABLE");  // Try to enable motors
  delay(500);
  sendToRobot("REQ_ODOM"); // Request initial status
  
  Serial.println("✅ ESP8266 Robot Controller Ready!");
  Serial.println("==========================================");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Process incoming data from robot
  processRobotResponse();
  
  // Periodic connection check and status request
  unsigned long now = millis();
  if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
    checkRobotConnection();
    
    // Request periodic odometry
    if (robotConnected) {
      sendToRobot("REQ_ODOM");
    }
    
    lastStatusUpdate = now;
  }
  
  yield(); // Allow ESP8266 to handle WiFi tasks
}
