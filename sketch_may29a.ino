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
int sumpHighTime, buttHighTime, sumpLowTime, buttLowTime, currentLoop = 0;
float voltRead, BSV = 1, SSV = 1;
float buttPeriod = 1, sumpPeriod = 1;
int repeatVal = 0;

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
  html += "<dd>50Hz = 0%–25%</dd>";
  html += "<dd>100Hz = 25%–50%</dd>";
  html += "<dd>200Hz = 50%–75%</dd>";
  html += "<dd>400Hz = 75%–∞%</dd>";
  html += "<dd>401Hz+ = Sensor may require cleaning</dd>";
  html += "</dl>";
  html += "<p><strong>repeatVal</strong> is used for debugging.</p>";
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

  //delay(10000);  // Initial delay
}

float sumpRead() {
  sumpHighTime = pulseIn(sumpSensorPin, HIGH, 1000000);
  sumpLowTime = pulseIn(sumpSensorPin, LOW, 1000000);
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;
    SSV = 1e6 / sumpPeriod;
  }
  return SSV;
}

void nPCV() {
  digitalWrite(pumpRelayPin, HIGH);
  digitalWrite(valveRelayPin, LOW);
}
void fPOV() {
  digitalWrite(pumpRelayPin, LOW);
  digitalWrite(valveRelayPin, HIGH);
}
float buttRead() {
  buttHighTime = pulseIn(buttSensorPin, HIGH, 1000000);
  buttLowTime = pulseIn(buttSensorPin, LOW, 1000000);
  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;
    BSV = 1e6 / buttPeriod;
  }
  return BSV;
}

void loop() {
  server.handleClient();
  currentLoop++;
  SSV = sumpRead();
  BSV = buttRead();

  while (SSV > 500) {
    SSV = sumpRead();
    webLog("SSV needs cleaning");
    fPOV();
    delay(10000);
    server.handleClient();
  }

  while (BSV > 500) {
    BSV = buttRead();
    webLog("BSV needs cleaning");
    fPOV();
    delay(10000);
    server.handleClient();
  }

  webLog("SSV: " + String(SSV));
  webLog("BSV: " + String(BSV));
  webLog("currentLoop: " + String(currentLoop));

  if (SSV > 00) {
    if (BSV < 50) {
      nPCV();
      webLog("nPCV");
      repeatVal = 0;
    } else if (BSV > 50) {
      fPOV();
      webLog("fPOV");
      //delay(3000);
      repeatVal = 0;
    }
  } else {
    if (repeatVal != -1) {
      fPOV();
    }
    webLog("fPOV");
    repeatVal = -1;
  }

  webLog("repeatVal: " + String(repeatVal));
  webLog(" ");
  delay(5000);
}
