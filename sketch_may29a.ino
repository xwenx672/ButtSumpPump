#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "BT-P7CT59";
const char* password = "cLqXHguH3xcuDt";

WebServer server(80);
String logBuffer = "";

// GPIO Pins
const int sumpSensorPin = 33;
const int buttSensorPin = 25;
const int valveRelayPin = 4;
const int pumpRelayPin = 2;

// Global variables
int sumpHighTime; // The highest part of the frequency.
int buttHighTime; // The highest part of the frequency.
int sumpLowTime; // The lowest part of the frequency.
int buttLowTime; // The lowest part of the frequency.
int currentLoop = 0; // The current loop.
float BSV = 1, SSV = 1;
float buttPeriod = 1, sumpPeriod = 1;
int defnPV = 10, defnVV = 50; // the values for nPV and nVV when they are reset in 'setValue()'.
int nPV = 10, nVV = 10, loopDelayms = 5000;
// 20hz = empty, 50hz = 25%, 100hz = 50%, 200hz = 70%, 400hz = 100%
int ssvLower = 50; // if water in sump is less than this, turn off pump.
int ssvUpper = 50; // if water in sump is more than this, turn on pump.
int bsvLower = 50; // if water in butt is more than this, turn off pump.
int bsvUpper = 50; // if water in butt is less than this, turn on pump.

// Logging function
void webLog(String message) {
  Serial.println(message);
  logBuffer = message + "<br>" + logBuffer;  // Prepend, so newest is first

  // Limit to ~100 lines
  int lineCount = 0;
  int index = 0;
  while ((index = logBuffer.indexOf("<br>", index)) != -1) {
    lineCount++;
    index += 4;
    if (lineCount > 1000) {
      int trimIndex = logBuffer.lastIndexOf("<br>");
      logBuffer = logBuffer.substring(0, trimIndex);
      break;
    }
  }
}
// Webpage handler
void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Gray Water System</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += "h2 { color: #333; }";
  html += "pre { background: #f4f4f4; padding: 10px; border: 1px solid #ccc; overflow-x: auto; }";
  html += "dl { margin-top: 10px; }";
  html += "dt { font-weight: bold; margin-top: 10px; }";
  html += "dd { margin: 0 0 10px 20px; }";
  html += "p { margin-bottom: 10px; }";
  html += "</style></head><body>";
  html += "<h2>Gray Water System</h2>";
  html += "<p><strong>SSV</strong> is the Sump Sensor Value.</p>";
  html += "<p><strong>BSV</strong> is the Butt Sensor Value.</p>";
  html += "<dl>";
  html += "<dt>Sensor Frequency Levels</dt>";
  html += "<dd>20Hz = 0%</dd>";
  html += "<dd>50Hz = 0% to 25%</dd>";
  html += "<dd>100Hz = 25% to 50%</dd>";
  html += "<dd>200Hz = 50% to 75%</dd>";
  html += "<dd>400Hz = 75% and above </dd>";
  html += "<dd>410Hz+ = Sensor may require cleaning</dd>";
  html += "</dl>";
  html += "<p><strong>nPV</strong> is used for debugging.</p>";
  html += "<p><strong>currentLoop</strong> counts how many loops the device has completed.</p>";
  html += "<p><strong>Be aware</strong> The firmware written for this has pauses of up to 5 minutes, so do not be surprised when nothing changes for a large period of time in some cases.</p>";
  html += "<h2>Log</h2>";
  html += "<pre>" + logBuffer + "</pre>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
void setup() {
  Serial.begin(115200);
  pinMode(sumpSensorPin, INPUT);
  pinMode(buttSensorPin, INPUT);
  pinMode(pumpRelayPin, OUTPUT);
  pinMode(valveRelayPin, OUTPUT);
  IPAddress local_IP(192, 168, 1, 184);     // Desired static IP
  IPAddress gateway(192, 168, 1, 254);        // Your router’s IP
  IPAddress subnet(255, 255, 255, 0);       // Usually this
  WiFi.config(local_IP, gateway, subnet);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.begin();
}
void setValue(String text) {
  if (text == "pump") {
    nPV = defnPV;
  } else if (text == "valve") {
    nVV = defnVV;
  } else if (text == "both") {
    nPV = defnPV;
    nVV = defnVV;
  } else {
    webLog("Passed wrong decrease value");
  }
}
int askDecreaseValve() {
  if (nVV > 0) {
    nVV--;
    return 1;
  } else {
    return 0;
  }
}
int askDecreasePump() {
  if (nPV > 0) {
    nPV--;
    return 1;
  } else {
    return 0;
  }
}
void onPumpCloseValve() {
    
  if (!digitalRead(pumpRelayPin)) {
    digitalWrite(pumpRelayPin, HIGH);
    setValue("pump");
    webLog("Pump: ON");
  }
    
  if ((!askDecreaseValve()) && (!digitalRead(valveRelayPin))) {
    digitalWrite(valveRelayPin, HIGH);
    setValue("valve");
    webLog("Valve: CLOSED");
  }
}
void offPumpOpenValve() {
  if ((!askDecreasePump()) && (digitalRead(pumpRelayPin))) {
    digitalWrite(pumpRelayPin, LOW);
    webLog("Pump: OFF");
  }
  
  if ((!askDecreaseValve()) && (digitalRead(valveRelayPin))) {
    digitalWrite(valveRelayPin, LOW);
    setValue("valve");
    webLog("Valve: OPEN");
  }
}
void sumpRead() {
  SSV = 0;
  sumpHighTime = pulseIn(sumpSensorPin, HIGH, 1000000);
  sumpLowTime = pulseIn(sumpSensorPin, LOW, 1000000);
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;
    SSV = 1e6 / sumpPeriod;
  }
}
void buttRead() {
  BSV = 0;
  buttHighTime = pulseIn(buttSensorPin, HIGH, 1000000);
  buttLowTime = pulseIn(buttSensorPin, LOW, 1000000);
  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;
    BSV = 1e6 / buttPeriod;
  }
}
void errorChecking() {
  while (SSV > 500) {
    sumpRead();
    webLog("SSV: " + String(SSV) + " > 500");
    nPV = 0;
    nVV = 0;
    offPumpOpenValve();
    delay(1000);
    server.handleClient();
  }

  while (SSV < 2) {
    sumpRead();
    webLog("SSV: " + String(SSV) + " < 20");
    nPV = 0;
    nVV = 0;
    offPumpOpenValve();
    delay(1000);
    server.handleClient();
  }

  while (BSV > 500) {
    buttRead();
    webLog("BSV: " + String(BSV) + " > 500");
    nPV = 0;
    nVV = 0;
    offPumpOpenValve();
    delay(1000);
    server.handleClient();
  }

  while (BSV < 2) {
    buttRead();
    webLog("BSV: " + String(BSV) + " < 20");
    nPV = 0;
    nVV = 0;
    offPumpOpenValve();
    delay(1000);
    server.handleClient();
  }
}
void displayValues() {
  webLog("SSV: " + String(SSV));
  webLog("BSV: " + String(BSV));
  webLog("currentLoop: " + String(currentLoop));
  webLog("nPV: " + String(nPV));
  webLog("nVV: " + String(nVV));
  webLog(" ");
  server.handleClient();
}
void loop() {
  currentLoop++;
  sumpRead();
  buttRead();
  errorChecking();

  if (SSV > ssvLower) {
    if (BSV < bsvUpper) {
      onPumpCloseValve();
    } else if (BSV > bsvLower) {
      offPumpOpenValve();
    }
  } else if (SSV < ssvUpper) {
    offPumpOpenValve();
  }
  
  displayValues();
  
  delay(loopDelayms);
}
