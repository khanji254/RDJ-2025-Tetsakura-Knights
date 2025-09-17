/*
 * ESP8266-01 Web Controller for Robot - Millis-Compatible Version
 * Communicates with Arduino Mega via UART (TX/RX)
 * Updated for robust status, command, and debug handling
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// ================ CONFIGURATION ================
const char* AP_SSID = "RobotController";
const char* AP_PASSWORD = "robot123";

const unsigned long SERIAL_BAUD = 115200;
const unsigned long STATUS_UPDATE_INTERVAL = 2000;
const unsigned long COMMAND_TIMEOUT = 1200;
const int MAX_COMMAND_LENGTH = 500;
const unsigned long CONNECTION_TIMEOUT = 8000;
const unsigned long RESPONSE_TIMEOUT = 2000;

ESP8266WebServer server(80);

// Global state variables
String rxBuffer = "";
String lastStatusData = "System Starting";
String lastSensorData = "No Data";
String debugBuffer = "";
unsigned long lastStatusUpdate = 0;
unsigned long lastRobotResponse = 0;
bool robotConnected = false;
bool motorsEnabled = false;
bool debugMode = true;
bool systemInitialized = false;
int commandCounter = 0;

// Individual motor PWM values
int motor1PWM = 0;
int motor2PWM = 0;
int motor3PWM = 0;
int motor4PWM = 0;

// ================ ROBOT COMMUNICATION ================
// Send raw JSON command to Arduino (no processing)
void sendToRobot(const String& jsonCommand) {
  // Just send the JSON command directly without any processing
  //Serial.print("[ESP TX] Sending to Arduino: ");
  //Serial.println(jsonCommand);
  
  Serial.println(jsonCommand);
  Serial.flush();
}


// Helper function to build JSON commands for non-web usage
void sendCommandToRobot(const String& cmd, const String& args = "") {
  if (args.length() == 0) {
    // Simple command without args
    String json = "{\"cmd\":\"" + cmd + "\"}";
    sendToRobot(json);
  } else {
    // Command with args
    String json = "{\"cmd\":\"" + cmd + "\",\"args\":" + args + "}";
    sendToRobot(json);
  }
}

void handleRobotMessage(const String& message) {
  lastRobotResponse = millis();
  robotConnected = true;

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, message);

if (error) {
    StaticJsonDocument<128> errDoc;
    errDoc["resp"] = "BAD_JSON";
    errDoc["raw"] = message;
    String out;
    serializeJson(errDoc, out);
    lastStatusData = out;
    return;
}

  const char* resp = doc["resp"] | "";
  if (strcmp(resp, "PONG") == 0) {
    lastStatusData = "PONG";
  }
  else if (strcmp(resp, "STATUS") == 0) {
    lastStatusData = "STATUS";
    motorsEnabled = doc["motors"] | 0;
    systemInitialized = strcmp(doc["system"] | "init", "ready") == 0;
  }
  else if (strcmp(resp, "SENSORS") == 0) {
    serializeJson(doc, lastSensorData);
  }
  else if (strcmp(resp, "ERROR") == 0) {
    lastStatusData = "ERROR:" + String((const char*)doc["msg"]);
  }
  else if (strcmp(resp, "ACK") == 0) {
    lastStatusData = "ACK:" + String((const char*)doc["type"]);
  }
  else if (strcmp(resp, "ODOM") == 0) {
    serializeJson(doc, lastSensorData);
    return;
  }
  else if (strcmp(resp, "DEBUG") == 0) {
    // Append debug messages to debugBuffer (keep last N lines)
    String dbg = String((const char*)doc["msg"]);
    debugBuffer += dbg + "\n";
    const int maxDebugLines = 20;
    int lines = 0, idx = debugBuffer.length();
    while (idx > 0 && lines < maxDebugLines) {
      idx = debugBuffer.lastIndexOf('\n', idx - 1);
      lines++;
    }
    if (idx > 0) debugBuffer = debugBuffer.substring(idx + 1);
    return; // Do NOT set lastStatusData for debug messages!
  }
  else {
    // Wrap unhandled messages as a JSON error object
    StaticJsonDocument<256> errDoc;
    errDoc["resp"] = "UNHANDLED";
    errDoc["raw"] = message;
    String out;
    serializeJson(errDoc, out);
    lastStatusData = out;
  }
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

bool requestStatus() {
  sendCommandToRobot("STATUS"); // Remove the duplicate sendToRobot("STATUS");
  delay(200);
  unsigned long startTime = millis();
  while (millis() - startTime < RESPONSE_TIMEOUT) {
    processRobotResponse();
    yield();
    delay(5);
  }
  return robotConnected;
}

// ================ WEB INTERFACE ================
const char* getWebPage() {
  static const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Controller - Millis System</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0; padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white; min-height: 100vh;
        }
        .container {
            max-width: 1200px; margin: 0 auto;
            background: rgba(255,255,255,0.1);
            border-radius: 20px; padding: 25px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
        }
        h1 { 
            text-align: center; margin-bottom: 25px; font-size: 2.2em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
        }
        .status-panel {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 12px; margin-bottom: 25px;
        }
        .status-item {
            background: rgba(255,255,255,0.15);
            padding: 12px; border-radius: 10px; text-align: center;
            transition: all 0.3s ease; font-size: 14px;
        }
        .status-value { 
            font-size: 1.1em; font-weight: bold; margin-top: 5px; 
            transition: color 0.3s ease;
        }
        .controls-grid {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 18px; margin-bottom: 25px;
        }
        .control-section {
            background: rgba(255,255,255,0.1);
            padding: 18px; border-radius: 15px;
        }
        .control-section h3 { 
            margin-top: 0; text-align: center; font-size: 1.3em;
            margin-bottom: 15px;
        }
        .direction-pad {
            display: grid; grid-template-areas:
                ". up ."
                "left stop right"
                ". down .";
            gap: 8px; max-width: 180px; margin: 0 auto;
        }
        .dir-btn { 
            padding: 12px; border: none; border-radius: 8px;
            background: rgba(255,255,255,0.25); color: white;
            font-size: 14px; cursor: pointer; transition: all 0.3s;
            user-select: none; font-weight: bold;
        }
        .dir-btn:hover { background: rgba(255,255,255,0.4); transform: scale(1.05); }
        .dir-btn:active { transform: scale(0.95); }
        .dir-btn:disabled { opacity: 0.4; cursor: not-allowed; }
        .up { grid-area: up; } .down { grid-area: down; }
        .left { grid-area: left; } .right { grid-area: right; }
        .stop { grid-area: stop; background: rgba(255,80,80,0.7); }
        .speed-control { text-align: center; margin: 15px 0; }
        .speed-slider, .motor-slider {
            width: 100%; height: 6px; border-radius: 3px;
            background: rgba(255,255,255,0.3); outline: none;
            -webkit-appearance: none;
        }
        .speed-slider::-webkit-slider-thumb, .motor-slider::-webkit-slider-thumb {
            -webkit-appearance: none; width: 18px; height: 18px;
            border-radius: 50%; background: white; cursor: pointer;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
        .motor-control {
            display: grid; grid-template-columns: 1fr 1fr;
            gap: 12px; margin: 12px 0;
        }
        .motor-item {
            background: rgba(255,255,255,0.1);
            padding: 8px; border-radius: 6px;
        }
        .motor-item label {
            display: block; font-size: 12px; margin-bottom: 4px;
            font-weight: bold;
        }
        .btn {
            padding: 8px 16px; border: none; border-radius: 5px;
            background: rgba(255,255,255,0.25); color: white;
            cursor: pointer; transition: all 0.3s; margin: 2px;
            font-size: 13px; font-weight: bold;
        }
        .btn:hover { background: rgba(255,255,255,0.4); }
        .btn:disabled { opacity: 0.4; cursor: not-allowed; }
        .btn-danger { background: rgba(255,80,80,0.6); }
        .btn-success { background: rgba(80,200,80,0.6); }
        .btn-warning { background: rgba(255,160,0,0.6); }
        .btn-info { background: rgba(0,140,255,0.6); }
        .btn-small { padding: 4px 8px; font-size: 11px; }
        .command-input {
            width: 65%; padding: 8px; border: none; border-radius: 4px;
            background: rgba(255,255,255,0.2); color: white;
            font-size: 13px;
        }
        .command-input::placeholder { color: rgba(255,255,255,0.6); }
        .log-display {
            background: rgba(0,0,0,0.4); padding: 12px; border-radius: 8px;
            font-family: 'Courier New', monospace; font-size: 11px; margin-top: 15px;
            max-height: 200px; overflow-y: auto;
            white-space: pre-wrap; line-height: 1.3;
        }
        .status-indicator {
            display: inline-block;
            width: 10px; height: 10px;
            border-radius: 50%; margin-right: 6px;
            box-shadow: 0 0 4px rgba(255,255,255,0.3);
        }
        .indicator-green { background-color: #4CAF50; }
        .indicator-red { background-color: #f44336; }
        .indicator-orange { background-color: #ff9800; }
        .indicator-blue { background-color: #2196F3; }
        .connection-info {
            text-align: center; 
            background: rgba(0,0,0,0.25);
            padding: 8px; border-radius: 8px;
            margin-bottom: 15px; font-size: 13px;
        }
        .debug-info {
            background: rgba(0,0,0,0.25);
            padding: 8px; border-radius: 5px;
            font-size: 11px; margin-top: 8px; line-height: 1.4;
        }
        .system-info {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 8px; margin-top: 10px; font-size: 11px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 Robot Controller v2.0</h1>
        
        <div class="connection-info">
            <strong>ESP8266 ↔ Arduino Mega (Millis System)</strong><br>
            <small>TB6612 Motor Control • Serial2 Communication • Real-time Debug</small>
        </div>
        
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
                <div>System</div>
                <div class="status-value" id="systemStatus">
                    <span class="status-indicator" id="systemIndicator"></span>Starting
                </div>
            </div>
            <div class="status-item">
                <div>Commands</div>
                <div class="status-value" id="cmdCount">0</div>
            </div>
            <div class="status-item">
                <div>Speed</div>
                <div class="status-value" id="speedDisplay">150</div>
            </div>
            <div class="status-item">
                <div>Response</div>
                <div class="status-value" id="respAge">0s</div>
            </div>
        </div>

        <div class="controls-grid">
            <div class="control-section">
                <h3>🎮 Movement Control</h3>
                <div class="direction-pad">
                    <button class="dir-btn up" onclick="sendCommand('FWD')">↑ FWD</button>
                    <button class="dir-btn left" onclick="sendCommand('LEFT')">← LEFT</button>
                    <button class="dir-btn stop" onclick="sendCommand('STOP')">⬛ STOP</button>
                    <button class="dir-btn right" onclick="sendCommand('RIGHT')">RIGHT →</button>
                    <button class="dir-btn down" onclick="sendCommand('BACK')">↓ BACK</button>
                </div>
                
                <div class="speed-control">
                    <label for="speedSlider">Speed: <span id="speedValue">150</span></label><br>
                    <input type="range" id="speedSlider" class="speed-slider" 
                           min="50" max="255" value="150" 
                           oninput="updateSpeed(this.value)">
                </div>
            </div>

            <div class="control-section">
                <h3>⚙️ System Control</h3>
                <button class="btn btn-warning" onclick="initializeSystem()">🚀 Initialize</button>
                <button class="btn btn-success" onclick="sendCommand('ENABLE')">✅ Enable Motors</button>
                <button class="btn btn-danger" onclick="sendCommand('DISABLE')">❌ Disable Motors</button><br>
                <button class="btn btn-info" onclick="sendCommand('TEST')">🔧 Motor Test</button>
                <button class="btn" onclick="sendCommand('STATUS')">📊 Get Status</button>
                <button class="btn" onclick="sendCommand('SENSORS')">🔍 Get Sensors</button>
                <button class="btn" onclick="sendCommand('PING')">📡 Ping</button>
            </div>

            <div class="control-section">
                <h3>🔧 Individual Motor Control</h3>
                <div class="motor-control">
                    <div class="motor-item">
                        <label for="m1Slider">M1-FL: <span id="m1Value">0</span></label>
                        <input type="range" id="m1Slider" class="motor-slider" 
                               min="-255" max="255" value="0" 
                               oninput="updateMotor(1, this.value)">
                    </div>
                    <div class="motor-item">
                        <label for="m2Slider">M2-FR: <span id="m2Value">0</span></label>
                        <input type="range" id="m2Slider" class="motor-slider" 
                               min="-255" max="255" value="0" 
                               oninput="updateMotor(2, this.value)">
                    </div>
                    <div class="motor-item">
                        <label for="m3Slider">M3-RL: <span id="m3Value">0</span></label>
                        <input type="range" id="m3Slider" class="motor-slider" 
                               min="-255" max="255" value="0" 
                               oninput="updateMotor(3, this.value)">
                    </div>
                    <div class="motor-item">
                        <label for="m4Slider">M4-RR: <span id="m4Value">0</span></label>
                        <input type="range" id="m4Slider" class="motor-slider" 
                               min="-255" max="255" value="0" 
                               oninput="updateMotor(4, this.value)">
                    </div>
                </div>
                <div style="text-align: center; margin-top: 8px;">
                    <button class="btn btn-small btn-success" onclick="applyMotorSettings()">▶️ Apply</button>
                    <button class="btn btn-small btn-danger" onclick="resetMotors()">🔄 Reset</button>
                    <button class="btn btn-small btn-warning" onclick="sendCommand('ENABLE')">⚡ Quick Enable</button>
                </div>
            </div>

            <div class="control-section">
                <h3>💬 Manual Command</h3>
                <input type="text" id="manualCommand" class="command-input" 
                       placeholder='e.g. {"cmd":"PING"} or {"cmd":"M","args":[150,150,150,150,1]}' 
                       onkeypress="if(event.key==='Enter') sendManualCommand()">
                <button class="btn btn-info" onclick="sendManualCommand()">📤 Send</button>
            </div>
        </div>

        <div class="control-section">
            <h3>📋 Robot Communication Log</h3>
            <div id="logDisplay" class="log-display">
                [ESP] ESP8266 Millis-Compatible Controller Loading...
            </div>
            <div class="debug-info" id="debugInfo">
                <div class="system-info">
                    <div>Commands: <span id="cmdCountBottom">0</span></div>
                    <div>Last Response: <span id="lastResp">Starting</span></div>
                    <div>Age: <span id="respAgeBottom">0</span>s</div>
                    <div>Connected: <span id="connStatus">No</span></div>
                </div>
                <div style="margin-top: 6px;">
                    Motor States → M1:<span id="m1Display">0</span> M2:<span id="m2Display">0</span> 
                    M3:<span id="m3Display">0</span> M4:<span id="m4Display">0</span>
                </div>
            </div>
        </div>
    </div>

    <script>
        let currentSpeed = 150;
        let commandCount = 0;
        let isConnected = false;
        let lastStatusData = "";
        let systemInitialized = false;
        
        // Motor PWM values
        let motor1PWM = 0, motor2PWM = 0, motor3PWM = 0, motor4PWM = 0;
        
        document.addEventListener('keydown', function(e) {
            if (e.target.tagName === 'INPUT') return;
            switch(e.key.toLowerCase()) {
                case 'w': case 'arrowup': sendCommand('FWD'); e.preventDefault(); break;
                case 's': case 'arrowdown': sendCommand('BACK'); e.preventDefault(); break;
                case 'a': case 'arrowleft': sendCommand('LEFT'); e.preventDefault(); break;
                case 'd': case 'arrowright': sendCommand('RIGHT'); e.preventDefault(); break;
                case ' ': case 'escape': sendCommand('STOP'); e.preventDefault(); break;
                case 'i': initializeSystem(); e.preventDefault(); break;
                case 't': sendCommand('TEST'); e.preventDefault(); break;
                case 'p': sendCommand('PING'); e.preventDefault(); break;
                case 'e': sendCommand('ENABLE'); e.preventDefault(); break;
                case 'q': sendCommand('DISABLE'); e.preventDefault(); break;
            }
        });
        
        function updateSpeed(value) {
            currentSpeed = parseInt(value);
            document.getElementById('speedValue').textContent = value;
            document.getElementById('speedDisplay').textContent = value;
        }
function updateElementText(elementId, text) {
    const element = document.getElementById(elementId);
    if (element) element.textContent = text;
}        
        function updateMotor(motorNum, value) {
            const intValue = parseInt(value);
            switch(motorNum) {
                case 1: 
                    motor1PWM = intValue;
                    document.getElementById('m1Value').textContent = value;
                    document.getElementById('m1Display').textContent = value;
                    break;
                case 2:
                    motor2PWM = intValue;
                    document.getElementById('m2Value').textContent = value;
                    document.getElementById('m2Display').textContent = value;
                    break;
                case 3:
                    motor3PWM = intValue;
                    document.getElementById('m3Value').textContent = value;
                    document.getElementById('m3Display').textContent = value;
                    break;
                case 4:
                    motor4PWM = intValue;
                    document.getElementById('m4Value').textContent = value;
                    document.getElementById('m4Display').textContent = value;
                    break;
            }
        }

        function applyMotorSettings() {
            // Send M command using JSON args array
            const args = [motor1PWM, motor2PWM, motor3PWM, motor4PWM, 1];
            const payload = { cmd: "M", args: args };
            postJsonCommand(payload, (resp) => {
                addToLog("Motor cmd -> " + JSON.stringify(resp), 'success');
                setTimeout(updateStatus, 250);
            });
            commandCount++;
            updateCommandCounters();
        }
        
        function resetMotors() {
            motor1PWM = motor2PWM = motor3PWM = motor4PWM = 0;
            ['m1Slider', 'm2Slider', 'm3Slider', 'm4Slider'].forEach(id => {
                const el = document.getElementById(id);
                if (el) el.value = 0;
            });
            [1,2,3,4].forEach(n => updateMotor(n, 0));
            // Send disable motor command
            postJsonCommand({ cmd: "M", args: [0,0,0,0,0] }, (resp) => {
                addToLog("Reset motors -> " + JSON.stringify(resp), 'success');
                setTimeout(updateStatus, 250);
            });
            commandCount++;
            updateCommandCounters();
        }
        
        function initializeSystem() {
            addToLog('🚀 Initializing Arduino Millis System...', 'info');
            systemInitialized = false;
            updateSystemStatus('Initializing...', 'orange');
            // Send INIT commands as JSON (arg as string)
            postJsonCommand({ cmd: "INIT", args: "SYSTEM" }, () => {});
            setTimeout(() => postJsonCommand({ cmd: "INIT", args: "MOTORS" }, () => {}), 800);
            setTimeout(() => postJsonCommand({ cmd: "INIT", args: "SENSORS" }, () => {}), 1600);
            setTimeout(() => postJsonCommand({ cmd: "STATUS" }, () => updateStatus()), 2500);
            commandCount += 3;
            updateCommandCounters();
        }
        
        function updateSystemStatus(status, color) {
            const indicator = color === 'green' ? 'indicator-green' : 
                             color === 'red' ? 'indicator-red' : 'indicator-orange';
            document.getElementById('systemStatus').innerHTML = 
                `<span class="status-indicator ${indicator}"></span>${status}`;
        }
        
        function addToLog(message, type = 'info') {
            const logDisplay = document.getElementById('logDisplay');
            const timestamp = new Date().toLocaleTimeString();
            const prefixMap = {
                'tx': '📤 TX: ', 'rx': '📥 RX: ', 'error': '❌ ERR: ',
                'success': '✅ OK: ', 'info': 'ℹ️  '
            };
            const prefix = prefixMap[type] || 'ℹ️  ';
            const lines = logDisplay.textContent.split('\n');
            lines.push(`${timestamp} ${prefix}${message}`);
            if (lines.length > 35) lines.shift();
            logDisplay.textContent = lines.join('\n');
            logDisplay.scrollTop = logDisplay.scrollHeight;
        }

        function updateCommandCounters() {
            document.getElementById('cmdCount').textContent = commandCount;
            document.getElementById('cmdCountBottom').textContent = commandCount;
        }

        // Generic function to POST JSON command to /command and handle JSON response
/// Generic function to POST JSON command to /command and handle JSON response
function postJsonCommand(payload, onSuccess) {
    addToLog(JSON.stringify(payload), 'tx');
    fetch('/command', {
        method: 'POST',
        headers: { 
            'Content-Type': 'application/json; charset=UTF-8',
            'Accept': 'application/json'
        },
        body: JSON.stringify(payload)
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok: ' + response.status);
        }
        return response.json();
    })
    .then(data => {
        addToLog(JSON.stringify(data), 'rx');
        if (onSuccess) onSuccess(data);
        if (data.robot) {
            lastStatusData = data.robot;
            // Safe element update
            const lastRespElement = document.getElementById('lastResp');
            if (lastRespElement) {
                lastRespElement.textContent = String(data.robot).substring(0,20);
            }
            isConnected = true;
        }
    })
    .catch(err => {
        addToLog("POST error: " + err.message, 'error');
        isConnected = false;
        
        // If it's a 400 error, try to get more details
        if (err.message.includes('400')) {
            addToLog("Server rejected request - check JSON format", 'error');
        }
    });
}

        // The central sendCommand() wrapper — builds JSON payloads
        function sendCommand(cmd) {
            let payload = null;
            switch (cmd) {
                case 'FWD':
                    payload = { cmd: "M", args: [currentSpeed, currentSpeed, currentSpeed, currentSpeed, 1] };
                    break;
                case 'BACK':
                    payload = { cmd: "M", args: [-currentSpeed, -currentSpeed, -currentSpeed, -currentSpeed, 1] };
                    break;
                case 'LEFT':
                    payload = { cmd: "M", args: [-currentSpeed, currentSpeed, -currentSpeed, currentSpeed, 1] };
                    break;
                case 'RIGHT':
                    payload = { cmd: "M", args: [currentSpeed, -currentSpeed, currentSpeed, -currentSpeed, 1] };
                    break;
                case 'STOP':
                    payload = { cmd: "M", args: [0,0,0,0,1] };
                    break;
                case 'ENABLE':
                    payload = { cmd: "ENABLE" };
                    break;
                case 'DISABLE':
                    payload = { cmd: "DISABLE" };
                    break;
                case 'STATUS':
                    payload = { cmd: "STATUS" };
                    break;
                case 'SENSORS':
                    payload = { cmd: "SENSORS" };
                    break;
                case 'PING':
                    payload = { cmd: "PING" };
                    break;
                case 'TEST':
                    payload = { cmd: "TEST" };
                    break;
                default:
                    // If caller provided a full legacy string like "M:..", convert:
                    if (typeof cmd === 'string' && cmd.indexOf(':') !== -1 && cmd[0] === 'M') {
                        // convert "M:m1,m2,m3,m4,en" to JSON
                        const params = cmd.substring(2).split(',').map(x => parseInt(x.trim()));
                        if (params.length >= 5) payload = { cmd: "M", args: params };
                        else payload = { cmd: cmd };
                    } else {
                        payload = { cmd: cmd };
                    }
            }

            // send
            if (payload) {
                postJsonCommand(payload, (resp) => {
                    // On motor/pwm commands, refresh status after short delay
                    if (String(payload.cmd) === "M" || String(payload.cmd) === "PWM") {
                        setTimeout(updateStatus, 300);
                    }
                });
                commandCount++;
                updateCommandCounters();
            }
        }
        
        function sendManualCommand() {
            const input = document.getElementById('manualCommand');
            if (!input.value.trim()) return;
            let raw = input.value.trim();
            // Try to parse input as JSON. If valid, send it directly.
            try {
                const parsed = JSON.parse(raw);
                postJsonCommand(parsed, (resp) => {});
            } catch (e) {
                // Not JSON — wrap as { cmd: "<raw>" } or try to detect legacy M: format
                if (raw.startsWith("M:") || raw.startsWith("PWM:") || raw.startsWith("INIT:") || raw.startsWith("DEBUG:")) {
                    // Let sendCommand convert or post directly
                    sendCommand(raw);
                } else {
                    postJsonCommand({ cmd: raw }, (resp) => {});
                }
            }
            commandCount++;
            updateCommandCounters();
            input.value = '';
        }
        
        function updateStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                // connection status
                const connIndicator = data.connected ? 'indicator-green' : 'indicator-red';
                const connText = data.connected ? 'Connected' : 'Disconnected';
                document.getElementById('connectionStatus').innerHTML = 
                    `<span class="status-indicator ${connIndicator}"></span>${connText}`;
                document.getElementById('connStatus').textContent = data.connected ? 'Yes' : 'No';

                // motors
                const motorIndicator = data.motorsEnabled ? 'indicator-green' : 'indicator-orange';
                const motorText = data.motorsEnabled ? 'Enabled' : 'Disabled';
                document.getElementById('motorStatus').innerHTML = 
                    `<span class="status-indicator ${motorIndicator}"></span>${motorText}`;

                // system
                if (data.systemInitialized) {
                    updateSystemStatus('Ready', 'green');
                    systemInitialized = true;
                } else {
                    updateSystemStatus('Not Ready', 'orange');
                }

                // response age
                const ageSeconds = Math.round((data.lastResponse || 0) / 1000);
                document.getElementById('respAge').textContent = ageSeconds + 's';
                document.getElementById('respAgeBottom').textContent = ageSeconds;

                // command counters, motor displays
                document.getElementById('cmdCount').textContent = data.commandCount || commandCount;
                document.getElementById('cmdCountBottom').textContent = data.commandCount || commandCount;
                document.getElementById('m1Display').textContent = data.motor1PWM || 0;
                document.getElementById('m2Display').textContent = data.motor2PWM || 0;
                document.getElementById('m3Display').textContent = data.motor3PWM || 0;
                document.getElementById('m4Display').textContent = data.motor4PWM || 0;

                // log robotData or sensorData if meaningful
                if (data.robotData && data.robotData !== lastStatusData) {
                    addToLog(String(data.robotData), 'rx');
                    lastStatusData = data.robotData;
                    document.getElementById('lastResp').textContent = String(data.robotData).substring(0,20);
                }
                if (data.sensorData) {
                    // optionally show sensor details
                }

                isConnected = !!data.connected;

                // enable/disable movement buttons
                const dirButtons = document.querySelectorAll('.dir-btn');
                dirButtons.forEach(btn => {
                    btn.disabled = !isConnected;
                    btn.style.opacity = isConnected ? '1' : '0.5';
                });
            })
            .catch(err => {
                document.getElementById('connectionStatus').innerHTML = 
                    '<span class="status-indicator indicator-red"></span>Error';
                isConnected = false;
            });
        }
        
        // Boot sequence
        setTimeout(() => {
            addToLog('ESP8266 Millis-Compatible Controller Ready', 'success');
            addToLog('🎮 Keyboard: WASD/Arrows=Move, Space=Stop, T=Test, P=Ping', 'info');
            addToLog('📡 Attempting Arduino connection...', 'info');
            sendCommand('PING');
            setTimeout(() => {
                sendCommand('STATUS');
                setTimeout(updateStatus, 500);
            }, 1000);
        }, 1500);

        setInterval(updateStatus, 3000);
        setInterval(() => {
            if (!isConnected) sendCommand('PING');
        }, 5000);
    </script>
</body>
</html>
)rawliteral";
  return webpage;
}


// ================ WEB SERVER HANDLERS ================
void handleRoot() {
  server.send(200, "text/html", getWebPage());
}

// In your ESP8266 code (handleCommand function)
// This version forwards the Arduino's JSON response directly to the browser,
// instead of wrapping it as a string, which fixes the JSON parse errors.

// In your ESP8266 code (handleCommand function)
// This version forwards the Arduino's JSON response directly to the browser,
// instead of wrapping it as a string, which fixes the JSON parse errors.

void handleCommand() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"METHOD_NOT_ALLOWED\"}");
    return;
  }

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"EMPTY_BODY\"}");
    return;
  }

  // Validate JSON
  StaticJsonDocument<256> testDoc;
  DeserializationError error = deserializeJson(testDoc, body);
  if (error) {
    server.send(400, "application/json", "{\"error\":\"INVALID_JSON\"}");
    return;
  }

  // Send to Arduino
  sendToRobot(body);
  commandCounter++;

  // Clear previous response and wait for new one
  String previousStatus = lastStatusData;
  lastStatusData = "TIMEOUT"; // Reset
  
  unsigned long startTime = millis();
  while (millis() - startTime < COMMAND_TIMEOUT) {
    processRobotResponse();
    if (lastStatusData != "TIMEOUT" && lastStatusData != previousStatus) {
      break; // Got new response
    }
    delay(10);
    yield();
  }

  // Send the response (make sure it's valid JSON)
  if (lastStatusData == "TIMEOUT" || lastStatusData == previousStatus) {
    server.send(200, "application/json", "{\"resp\":\"TIMEOUT\"}");
  } else {
    server.send(200, "application/json", lastStatusData);
  }
}



void handleStatus() {
  processRobotResponse();
  if (millis() - lastRobotResponse > CONNECTION_TIMEOUT) robotConnected = false;

  StaticJsonDocument<512> doc;
  doc["connected"] = robotConnected;
  doc["motorsEnabled"] = motorsEnabled;
  doc["systemInitialized"] = systemInitialized;
  doc["robotData"] = lastStatusData;
  doc["sensorData"] = lastSensorData;
  doc["debugData"] = debugBuffer;
  doc["motor1PWM"] = motor1PWM;
  doc["motor2PWM"] = motor2PWM;
  doc["motor3PWM"] = motor3PWM;
  doc["motor4PWM"] = motor4PWM;
  doc["commandCount"] = commandCounter;
  doc["lastResponse"] = millis() - lastRobotResponse;
  doc["uptime"] = millis();
  doc["debugMode"] = debugMode;

  String jsonOut;
  serializeJson(doc, jsonOut);
  server.send(200, "application/json", jsonOut);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Page Not Found");
}

// ================ MAIN FUNCTIONS ================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  while (Serial.available()) Serial.read();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  delay(2000);
  server.on("/", handleRoot);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  lastRobotResponse = millis();
  lastStatusUpdate = millis();

  delay(3000);
  sendCommandToRobot("ESP8266", "\"READY\"");
  delay(500);
  sendCommandToRobot("PING");
  sendCommandToRobot("HELLO", "\"ESP8266_READY\"");
  delay(500);
  sendCommandToRobot("PING");
}


void loop() {
  server.handleClient();
  processRobotResponse();
  unsigned long now = millis();
  if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
    if (robotConnected) {
      sendCommandToRobot("STATUS");
    } else {
      sendCommandToRobot("PING");
    }
    lastStatusUpdate = now;
  }
  yield();
  delay(10);
}