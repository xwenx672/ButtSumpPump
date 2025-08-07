#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>

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
float butt = 1, sump = 1;
float buttPeriod = 1, sumpPeriod = 1;
int defnPV = 10, defnVV = 200, subtract = 1; // the values for nPV and nVV when they are reset in 'setValue()'.
int nPV = 10, nVV = 10;
const unsigned long loopDelayms = 2500;
const unsigned int maxLineCount = 70; // How many lines the webpage shows.
// 20hz = empty, 50hz = 25%, 100hz = 50%, 200hz = 75%, 400hz = 100%

int sumpUpper = 40; // 1 (sump < sumpUpper) if water in sump is less than this, turn off pump.
int sumpLower = 40; // 2 (sump > sumpLower) if water in sump is more than this, turn on pump. Needs to be Lower than Upper.
int buttUpper = 40; // A (butt > buttLower) if water in butt is more than this, turn off pump.
int buttLower = 40; // B (butt < buttUpper) if water in butt is less than this, turn on pump. Needs to be Lower than Upper.

void webLog(String message) {
  Serial.println(message);
  logBuffer = message + "<br>" + logBuffer;
  int lineCount = 0, index = 0;
  while ((index = logBuffer.indexOf("<br>", index)) != -1) {
    lineCount++; index += 4;
    if (lineCount > maxLineCount) {
      int trimIndex = logBuffer.lastIndexOf("<br>");
      logBuffer = logBuffer.substring(0, trimIndex);
      break;
    }
  }
}

void setupOTA() {
  ArduinoOTA
    .setPassword("Blueeyeswhitedragon10!")
    .onStart([]() {
      String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
      webLog("Start updating " + type);
    })
    .onEnd([]() {
      webLog("Update complete. Rebooting...");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      webLog("Progress: " + String((progress * 100) / total) + "%");
    })
    .onError([](ota_error_t error) {
      String errorMsg = "Error[" + String(error) + "]: ";
      if (error == OTA_AUTH_ERROR)       errorMsg += "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) errorMsg += "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) errorMsg += "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) errorMsg += "Receive Failed";
      else if (error == OTA_END_ERROR)     errorMsg += "End Failed";
      else errorMsg += "Unknown Error";
      webLog(errorMsg);
    });
  ArduinoOTA.begin();
}


void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Greywater Pump Monitor v3.2.250807</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f8f8f8; }";
  html += "h1, h2 { color: #2a2a2a; }";
  html += "pre { background: #ffffff; padding: 10px; border: 1px solid #ccc; overflow-x: auto; }";
  html += "dl { margin-top: 10px; background: #fff; padding: 10px; border: 1px solid #ccc; }";
  html += "dt { font-weight: bold; margin-top: 10px; }";
  html += "dd { margin: 0 0 10px 20px; }";
  html += "p { margin-bottom: 10px; }";
  html += "</style></head><body>";
  html += "<h1>Greywater Pump Monitor v3.2.250807</h1>";

  html += "<p>This page shows real-time status of the sump and water butt sensors, and whether the pump/valve is allowed to operate.</p>";

  html += "<h2>System Information</h2>";
  html += "<p><strong>sump (Sump Sensor Value)</strong>: Measures how full the sump is based on signal frequency. Higher frequency = more water.</p>";
  html += "<p><strong>butt (Butt Sensor Value)</strong>: Measures how full the water butt is. Lower frequency = less water.</p>";
  html += "<p><strong>nPV (Pump Timeout)</strong>: Countdown before the pump is allowed to turn off. 0 = ready.</p>";
  html += "<p><strong>nVV (Valve Timeout)</strong>: Countdown before the valve is allowed to change position. 0 = ready.</p>";
  html += "<p><strong>Time On (minutes)</strong>: " + String((currentLoop * loopDelayms) / 60000.0, 1) + "</p>";
  html += "<dl>";
  html += "<dt>Sensor Frequency to Fill Level Guide</dt>";
  html += "<dd>20Hz or less is empty</dd>";
  html += "<dd>50Hz is ~25% full</dd>";
  html += "<dd>100Hz is ~50% full</dd>";
  html += "<dd>200Hz is ~75% full</dd>";
  html += "<dd>400Hz or higher is full</dd>";
  html += "<dd>More than 410Hz or less than 2hz and the sensor in question may require cleaning</dd>";
  html += "</dl>";
  //html += "<p><strong>Time On (minutes)</strong>: " + String((currentLoop * 2.5) / 60.0, 1) + "</p>";
  html += "<h2>Event Log</h2>";
  html += "<pre>" + logBuffer + "</pre>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleUploadPage() {
  String html = "<html><head><title>Firmware Upload</title></head><body>";
  html += "<h2>Upload Firmware</h2>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  html += "<input type='file' name='update'>";
  html += "<input type='submit' value='Upload'>";
  html += "</form></body></html>";
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

  setupOTA();
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
    nVV = nVV - subtract;
    if (nVV < 0) {
      nVV = 0;
    }
    return 1;
  } else {
    return 0;
  }
}
int askDecreasePump() {
  if (nPV > 0) {
    nPV = nPV - subtract;
    if (nPV < 0) {
      nPV = 0;
    }
    return 1;
  } else {
    return 0;
  }
}


void readPins() {
  if (digitalRead(pumpRelayPin)) {
    webLog("Pump: ON");
  } else { 
    webLog("Pump: OFF");
  }
  if (digitalRead(valveRelayPin)) {
    webLog("Valve: CLOSED");
  } else {
    webLog("Valve: OPEN");
  }
}

void onPumpCloseValve() {
  if (!digitalRead(pumpRelayPin)) {
    digitalWrite(pumpRelayPin, HIGH);
    setValue("pump");
  } 
  if (!digitalRead(valveRelayPin)) {
    digitalWrite(valveRelayPin, HIGH);
  }
  setValue("valve");
}

void offPumpOpenValve() {
  if ((!askDecreasePump()) && (digitalRead(pumpRelayPin))) {
    digitalWrite(pumpRelayPin, LOW);
  }  
  if ((!askDecreaseValve()) && (digitalRead(valveRelayPin))) {
    digitalWrite(valveRelayPin, LOW);
  }
}

void sumpRead() {
  sump = 0;
  sumpHighTime = pulseIn(sumpSensorPin, HIGH, 1000000);
  sumpLowTime = pulseIn(sumpSensorPin, LOW, 1000000);
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;
    sump = 1e6 / sumpPeriod;
  }
}
void buttRead() {
  butt = 0;
  buttHighTime = pulseIn(buttSensorPin, HIGH, 1000000);
  buttLowTime = pulseIn(buttSensorPin, LOW, 1000000);
  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;
    butt = 1e6 / buttPeriod;
  }
}
void errorChecking() {
  while (sump > 500 || sump < 2 || butt > 500 || butt < 2) {
    if (sump > 500) webLog("sump: " + String(sump) + " > 500");
    if (sump < 2) webLog("sump: " + String(sump) + " < 2");
    if (butt > 500) webLog("butt: " + String(butt) + " > 500");
    if (butt < 2) webLog("butt: " + String(butt) + " < 2");
    nPV = 0; nVV = 0; offPumpOpenValve();
    delay(20);
    server.handleClient(); ArduinoOTA.handle();
    sumpRead(); buttRead();
  }
}

void displayValues() {
  webLog("sump: " + String(sump));
  webLog("butt: " + String(butt));
  // webLog("currentLoop: " + String(currentLoop));
  webLog("nPV: " + String(nPV));
  webLog("nVV: " + String(nVV));
  webLog(" ");
  server.handleClient();
}
void loop() {
  unsigned long start = millis();
  while (millis() - start < loopDelayms) {
    ArduinoOTA.handle();
    server.handleClient();
    delay(20); // Prevent watchdog timeout
  }
  currentLoop++;
  sumpRead();
  buttRead();
  errorChecking();
  readPins();

  if (sump > sumpLower) {
    subtract = 1;
    if (butt < buttUpper) {
      onPumpCloseValve();
    } else if (butt > buttLower) {
      offPumpOpenValve();
    }
  } else if (sump < sumpUpper) {
    subtract = 5;
    offPumpOpenValve();
  }
  
  displayValues();
  
  //delay(loopDelayms);
}
