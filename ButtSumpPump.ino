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
int defnPV = 10, defnVV = 100; // the values for nPV and nVV when they are reset in 'setValue()'.
int nPV = 10, nVV = 10;
const unsigned long loopDelayms = 2500;
// 20hz = empty, 50hz = 25%, 100hz = 50%, 200hz = 75%, 400hz = 100%
int ssvLower = 50; // 1 (SSV > ssvLower) if water in sump is more than this, turn on pump.
int ssvUpper = 50; // 2 (SSV < ssvUpper) if water in sump is less than this, turn off pump.
int bsvUpper = 100; // A (BSV > bsvLower) if water in butt is more than this, turn off pump.
int bsvLower = 50; // B (BSV < bsvUpper) if water in butt is less than this, turn on pump.

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
void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Greywater Pump Monitor v2.1.250718</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f8f8f8; }";
  html += "h1, h2 { color: #2a2a2a; }";
  html += "pre { background: #ffffff; padding: 10px; border: 1px solid #ccc; overflow-x: auto; }";
  html += "dl { margin-top: 10px; background: #fff; padding: 10px; border: 1px solid #ccc; }";
  html += "dt { font-weight: bold; margin-top: 10px; }";
  html += "dd { margin: 0 0 10px 20px; }";
  html += "p { margin-bottom: 10px; }";
  html += "</style></head><body>";
  html += "<h1>Greywater System Status</h1>";

  html += "<p>This page updates every 5 seconds to show real-time status of the sump and water butt sensors, and whether the pump/valve is allowed to operate.</p>";

  html += "<h2>Sensor Readings</h2>";
  html += "<p><strong>SSV (Sump Sensor Value)</strong>: Measures how full the sump is based on signal frequency. Higher frequency = more water.</p>";
  html += "<p><strong>BSV (Butt Sensor Value)</strong>: Measures how full the water butt is. Lower frequency = less water.</p>";

  html += "<dl>";
  html += "<dt>Sensor Frequency to Fill Level Guide</dt>";
  html += "<dd>20Hz or less is empty</dd>";
  html += "<dd>50Hz is ~25% full</dd>";
  html += "<dd>100Hz is ~50% full</dd>";
  html += "<dd>200Hz is ~75% full</dd>";
  html += "<dd>400Hz or higher is full</dd>";
  html += "<dd>More than 410Hz or less than 2hz and the sensor in question may require cleaning</dd>";
  html += "</dl>";

  html += "<h2>System Information</h2>";
  html += "<p><strong>nPV (Pump Timeout)</strong>: Countdown before the pump is allowed to turn off. 0 = ready.</p>";
  html += "<p><strong>nVV (Valve Timeout)</strong>: Countdown before the valve is allowed to change position. 0 = ready.</p>";
  html += "<p><strong>Time On (minutes)</strong>: " + String((currentLoop * loopDelayms) / 60000.0, 1) + "</p>";
  //html += "<p><strong>Time On (minutes)</strong>: " + String((currentLoop * 2.5) / 60.0, 1) + "</p>";

  html += "<h2>Event Log</h2>";
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
    webLog("SSV: " + String(SSV) + " < 2");
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
    webLog("BSV: " + String(BSV) + " < 2");
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
  // webLog("currentLoop: " + String(currentLoop));
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
