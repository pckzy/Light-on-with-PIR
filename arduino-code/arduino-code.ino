#include <WiFiS3.h>
#include <R4HttpClient.h>

int pirPin = 2;
int ledPin = 8;
unsigned long lastMotionTime = 0;
unsigned long motionDelay = 5000;
int dimLevel = 50;
bool pirMode = true;
bool motionDetected = false;
int currentBrightness = 0;

const char* ssid = "Pckzy_2.4G";
const char* password = "03032548pk";
WiFiServer server(80);

void connectToWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to WiFi.");
    }
}

void sendResponse(WiFiClient client, String content) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.print(content);
    client.stop();
}

void setLEDBrightness(int brightness) {
    currentBrightness = brightness;
    analogWrite(ledPin, brightness);
}

void sendStatusJSON(WiFiClient client) {
    String json = "{";
    json += "\"pirMode\":" + String(pirMode ? "true" : "false") + ",";
    json += "\"motionDetected\":" + String(motionDetected ? "true" : "false") + ",";
    json += "\"brightness\":" + String(currentBrightness);
    json += "}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(json);
    client.stop();
}

void handleRoot(WiFiClient client) {
    String html = "<!DOCTYPE html><html lang='en'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>LED Control</title>";
    html += "<style>";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; font-family: Arial, sans-serif; }";
    html += "body { background: #f0f2f5; color: #333; line-height: 1.6; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #1a73e8; text-align: center; margin-bottom: 30px; font-size: 2em; }";
    html += ".status-card { background: #f8f9fa; border-radius: 8px; padding: 20px; margin: 20px 0; text-align: center; }";
    html += ".btn { display: inline-block; padding: 12px 24px; background: #1a73e8; color: white; text-decoration: none; border-radius: 5px; margin: 10px; transition: background 0.3s; }";
    html += ".btn:hover { background: #1557b0; }";
    html += ".btn.mode { background: " + String(pirMode ? "#4caf50" : "#ff9800") + "; }";
    html += ".btn.mode:hover { background: " + String(pirMode ? "#3d8b40" : "#f57c00") + "; }";
    html += ".status { font-size: 1.2em; color: #666; margin: 20px 0; }";
    html += ".led-indicator { width: 20px; height: 20px; border-radius: 50%; display: inline-block; margin-right: 10px; vertical-align: middle; background: " + 
            String((pirMode && motionDetected) || (!pirMode && currentBrightness > 0) ? "#4caf50" : "#ccc") + "; }";
    html += "@media (max-width: 480px) { .container { padding: 15px; } .btn { display: block; margin: 10px 0; } }";
    html += "</style>";
    
    html += "<script>";
    html += "function updateStatus() {";
    html += "    fetch('/status')";
    html += "        .then(response => response.json())";
    html += "        .then(data => {";
    html += "            const ledIndicator = document.querySelector('.led-indicator');";
    html += "            const ledStatus = document.getElementById('ledStatus');";
    html += "            if (data.pirMode) {";
    html += "                ledIndicator.style.background = data.motionDetected ? '#4caf50' : '#ccc';";
    html += "                ledStatus.textContent = data.motionDetected ? 'DETECT' : 'STANDBY';";
    html += "            } else {";
    html += "                ledIndicator.style.background = data.brightness > 0 ? '#4caf50' : '#ccc';";
    html += "                ledStatus.textContent = data.brightness > 0 ? 'ON' : 'OFF';";
    html += "            }";
    html += "        });";
    html += "}";
    html += "setInterval(updateStatus, 500);";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>LED Control</h1>";
    
    html += "<div class='status-card'>";
    html += "<div class='status'><span class='led-indicator'></span>LED Status: <span id='ledStatus'>" + 
            String(pirMode ? (motionDetected ? "DETECT" : "STANDBY") : (currentBrightness > 0 ? "ON" : "OFF")) + "</span></div>";
    
    html += "<div class='status'>Mode: " + String(pirMode ? "Motion Detection (PIR)" : "Manual Control") + "</div>";
    html += "</div>";
    
    html += "<div style='text-align: center;'>";
    html += "<a href='/toggle' class='btn'>Turn on/off LED</a>";
    html += "<a href='/mode' class='btn mode'>Switch to " + String(pirMode ? "Manual" : "PIR") + " Mode</a>";
    html += "</div>";
    
    html += "</div></body></html>";
    
    sendResponse(client, html);
}

void handleToggleLED(WiFiClient client) {
    if (!pirMode) {
        setLEDBrightness(currentBrightness == 0 ? 255 : 0);
    }
    handleRoot(client);
}

void handleModeSwitch(WiFiClient client) {
    pirMode = !pirMode;
    if (pirMode) {
        setLEDBrightness(dimLevel);
        motionDetected = false;
    } else {
        setLEDBrightness(0);
    }
    handleRoot(client);
}

void setup() {
    pinMode(pirPin, INPUT);
    pinMode(ledPin, OUTPUT);
    Serial.begin(9600);
    connectToWiFi();
    server.begin();
    setLEDBrightness(pirMode ? dimLevel : 0);
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        String request = client.readStringUntil('\r');
        client.flush();
        
        if (request.indexOf("GET /toggle") != -1) {
            handleToggleLED(client);
        } else if (request.indexOf("GET /mode") != -1) {
            handleModeSwitch(client);
        } else if (request.indexOf("GET /status") != -1) {
            sendStatusJSON(client);
        } else {
            handleRoot(client);
        }
    }
    
    if (pirMode) {
        int pirState = digitalRead(pirPin);
        unsigned long currentTime = millis();
        
        if (pirState == HIGH) {
            motionDetected = true;
            setLEDBrightness(255);
            lastMotionTime = currentTime;
        } else if (currentTime - lastMotionTime > motionDelay) {
            motionDetected = false;
            setLEDBrightness(dimLevel);
        }
    }
}