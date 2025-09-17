#include "web_interface.h"
#include "robot_comm.h"
#include "esp_config.h"
#include <ArduinoJson.h>

ESP8266WebServer server(WEB_SERVER_PORT);

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web server started on port " + String(WEB_SERVER_PORT));
}

void handleWebRequests() {
  server.handleClient();
}

void handleRoot() {
  String html = generateControlPage();
  server.send(200, "text/html", html);
}

void handleCommand() {
  if (server.hasArg("cmd")) {
    String command = server.arg("cmd");
    sendCommandToRobot(command);
    
    DynamicJsonDocument doc(200);
    doc["status"] = "success";
    doc["command"] = command;
    doc["robot_connected"] = robotStatus.connected;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No command provided\"}");
  }
}

void handleStatus() {
  DynamicJsonDocument doc(500);
  doc["connected"] = robotStatus.connected;
  doc["last_response"] = robotStatus.lastResponse;
  doc["motors_enabled"] = robotStatus.motorsEnabled;
  doc["current_speed"] = robotStatus.currentSpeed;
  doc["odometry"] = robotStatus.lastOdometry;
  doc["uptime"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "File not found");
}

String generateControlPage() {
  return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Controller</title>
    <style>)" + generateCSS() + R"(</style>
</head>
<body>
    <div class="container">
        <h1>🤖 Robot Controller</h1>
        
        <div class="status-panel">
            <h3>Robot Status</h3>
            <div id="status">
                <span id="connection-status">Connecting...</span>
                <span id="motor-status">Motors: Unknown</span>
            </div>
        </div>
        
        <div class="control-panel">
            <h3>Movement Controls</h3>
            <div class="movement-grid">
                <button class="move-btn" onclick="sendCommand('LEFT 150')">↰ Left</button>
                <button class="move-btn" onclick="sendCommand('FWD 150')">↑ Forward</button>
                <button class="move-btn" onclick="sendCommand('RIGHT 150')">↱ Right</button>
                <div></div>
                <button class="move-btn" onclick="sendCommand('BACK 150')">↓ Backward</button>
                <div></div>
            </div>
            
            <div class="control-row">
                <button class="control-btn stop-btn" onclick="sendCommand('STOP')">🛑 STOP</button>
                <button class="control-btn" onclick="sendCommand('ENABLE')" id="enable-btn">▶️ Enable</button>
                <button class="control-btn" onclick="sendCommand('DISABLE')" id="disable-btn">⏸️ Disable</button>
            </div>
        </div>
        
        <div class="speed-panel">
            <h3>Speed Control</h3>
            <label for="speed-slider">Speed: <span id="speed-value">150</span></label>
            <input type="range" id="speed-slider" min="50" max="255" value="150" oninput="updateSpeed(this.value)">
        </div>
        
        <div class="manual-control">
            <h3>Manual Commands</h3>
            <input type="text" id="manual-command" placeholder="Enter command (e.g., SET_V 100 100)">
            <button onclick="sendManualCommand()">Send</button>
        </div>
        
        <div class="odometry-panel">
            <h3>Odometry Data</h3>
            <pre id="odometry-data">No data received</pre>
        </div>
    </div>
    
    <script>)" + generateJavaScript() + R"(</script>
</body>
</html>
)";
}

String generateCSS() {
  return R"(
body {
    font-family: Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    margin: 0;
    padding: 20px;
    min-height: 100vh;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    background: rgba(255, 255, 255, 0.95);
    border-radius: 15px;
    padding: 30px;
    box-shadow: 0 15px 35px rgba(0, 0, 0, 0.1);
}

h1 {
    text-align: center;
    color: #333;
    margin-bottom: 30px;
    font-size: 2.5em;
}

h3 {
    color: #555;
    border-bottom: 2px solid #667eea;
    padding-bottom: 10px;
    margin-top: 30px;
}

.status-panel, .control-panel, .speed-panel, .manual-control, .odometry-panel {
    background: #f8f9fa;
    border-radius: 10px;
    padding: 20px;
    margin: 20px 0;
}

#status {
    display: flex;
    justify-content: space-between;
    font-weight: bold;
}

.movement-grid {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    gap: 10px;
    max-width: 300px;
    margin: 20px auto;
}

.move-btn {
    padding: 15px;
    font-size: 18px;
    border: none;
    border-radius: 10px;
    background: #667eea;
    color: white;
    cursor: pointer;
    transition: all 0.3s;
}

.move-btn:hover {
    background: #5a6fd8;
    transform: translateY(-2px);
}

.move-btn:active {
    transform: translateY(0);
}

.control-row {
    display: flex;
    justify-content: center;
    gap: 15px;
    margin-top: 20px;
}

.control-btn {
    padding: 12px 20px;
    border: none;
    border-radius: 8px;
    cursor: pointer;
    font-size: 16px;
    transition: all 0.3s;
}

.stop-btn {
    background: #dc3545;
    color: white;
}

.stop-btn:hover {
    background: #c82333;
}

#enable-btn {
    background: #28a745;
    color: white;
}

#enable-btn:hover {
    background: #218838;
}

#disable-btn {
    background: #6c757d;
    color: white;
}

#disable-btn:hover {
    background: #5a6268;
}

#speed-slider {
    width: 100%;
    margin: 10px 0;
}

.manual-control input {
    width: 70%;
    padding: 10px;
    border: 1px solid #ddd;
    border-radius: 5px;
    margin-right: 10px;
}

.manual-control button {
    padding: 10px 20px;
    background: #667eea;
    color: white;
    border: none;
    border-radius: 5px;
    cursor: pointer;
}

#odometry-data {
    background: #2d3748;
    color: #68d391;
    padding: 15px;
    border-radius: 5px;
    font-family: 'Courier New', monospace;
    overflow-x: auto;
    min-height: 100px;
}

.connected {
    color: #28a745;
}

.disconnected {
    color: #dc3545;
}
)";
}

String generateJavaScript() {
  return R"(
let currentSpeed = 150;
let isConnected = false;

function updateSpeed(value) {
    currentSpeed = value;
    document.getElementById('speed-value').textContent = value;
}

function sendCommand(baseCmd) {
    let command = baseCmd;
    
    // Replace speed in movement commands
    if (baseCmd.includes('150')) {
        command = baseCmd.replace('150', currentSpeed.toString());
    }
    
    fetch('/command', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: 'cmd=' + encodeURIComponent(command)
    })
    .then(response => response.json())
    .then(data => {
        console.log('Command sent:', data);
    })
    .catch(error => {
        console.error('Error:', error);
        alert('Failed to send command');
    });
}

function sendManualCommand() {
    const input = document.getElementById('manual-command');
    const command = input.value.trim();
    
    if (command) {
        sendCommand(command);
        input.value = '';
    }
}

function updateStatus() {
    fetch('/status')
    .then(response => response.json())
    .then(data => {
        const connectionStatus = document.getElementById('connection-status');
        const motorStatus = document.getElementById('motor-status');
        const odometryData = document.getElementById('odometry-data');
        
        isConnected = data.connected;
        
        if (data.connected) {
            connectionStatus.textContent = '🟢 Connected';
            connectionStatus.className = 'connected';
        } else {
            connectionStatus.textContent = '🔴 Disconnected';
            connectionStatus.className = 'disconnected';
        }
        
        motorStatus.textContent = 'Motors: ' + (data.motors_enabled ? 'Enabled' : 'Disabled');
        
        if (data.odometry) {
            odometryData.textContent = data.odometry;
        }
    })
    .catch(error => {
        console.error('Status update error:', error);
        document.getElementById('connection-status').textContent = '❌ Error';
    });
}

// Handle keyboard controls
document.addEventListener('keydown', function(event) {
    if (event.target.tagName.toLowerCase() === 'input') return;
    
    switch(event.key.toLowerCase()) {
        case 'w':
        case 'arrowup':
            sendCommand('FWD ' + currentSpeed);
            event.preventDefault();
            break;
        case 's':
        case 'arrowdown':
            sendCommand('BACK ' + currentSpeed);
            event.preventDefault();
            break;
        case 'a':
        case 'arrowleft':
            sendCommand('LEFT ' + currentSpeed);
            event.preventDefault();
            break;
        case 'd':
        case 'arrowright':
            sendCommand('RIGHT ' + currentSpeed);
            event.preventDefault();
            break;
        case ' ':
        case 'spacebar':
            sendCommand('STOP');
            event.preventDefault();
            break;
    }
});

// Allow Enter key to send manual commands
document.getElementById('manual-command').addEventListener('keypress', function(event) {
    if (event.key === 'Enter') {
        sendManualCommand();
    }
});

// Update status every 500ms
setInterval(updateStatus, 500);

// Initial status update
updateStatus();
)";
}
