#include <OneWire.h>
#include <DallasTemperature.h>

// -------------------- Pin assignments (UNCHANGED) --------------------
constexpr int ONE_WIRE_BUS1 = 19;
constexpr int ONE_WIRE_BUS2 = 18;

constexpr int level1    = 16;
constexpr int level2    = 17;

constexpr int AC_PIN    = 34;   // SEN0211
constexpr int TURB_PIN  = 35;   // (unused here, kept for future)
constexpr int PH_PIN    = 32;   // SEN0161

constexpr int relay1    = 26;
constexpr int relay2    = 25;
constexpr int relayPump = 33;

// -------------------- OneWire / Temperature -------------------------
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

// -------------------- ADC / Sensors config --------------------------
constexpr float VREF = 3.3f;            // ADC full-scale used for conversion (approx)
constexpr int   ADC_MAX = 4095;         // 12-bit ADC

// SEN0211: 20A model, but you have 6 turns through the clamp.
// 6 turns multiplies sensed current by 6, so divide reading by 6 to get real current.
constexpr float ACTectionRange = 20.0f; // 20A model
constexpr int   CT_TURNS = 6;           // <--- your number of loops
constexpr float CT_SCALE = 1.0f / CT_TURNS;

// pH calibration (replace with your measured voltages at pH7 and pH4)
constexpr float PH7_VOLT = 1.50f;
constexpr float PH4_VOLT = 2.00f;

// -------------------- Helpers --------------------
static inline float adcToVolts(float adcCounts) {
  return (adcCounts / float(ADC_MAX)) * VREF;
}

float readAdcAverage(int pin, int samples, int delayMsPerSample) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    if (delayMsPerSample > 0) delay(delayMsPerSample);
  }
  return sum / float(samples);
}

// -------------------- AC current (SEN0211) ---------------------------
float readACCurrentAmps(int pin) {
  // Average a few readings (as you had)
  float adcAvg = readAdcAverage(pin, 5, 1);

  // Convert ADC to "peak" voltage at ADC pin (matches your original approach)
  float vPeak = adcToVolts(adcAvg);

  // Peak -> RMS
  float vRms = vPeak * 0.707f;

  // Board note: divide by 2 (kept exactly as your working method)
  vRms *= 0.5f;

  // Map 0–1Vrms (from probe method) to 0–ACTectionRange amps
  float amps = vRms * ACTectionRange;

  // Correct for multiple turns through the CT
  amps *= CT_SCALE;

  return amps;
}

// -------------------- pH conversion --------------------
float voltageToPH(float v) {
  // line through (PH7_VOLT,7) and (PH4_VOLT,4)
  const float slope = (4.0f - 7.0f) / (PH4_VOLT - PH7_VOLT);
  const float intercept = 7.0f - slope * PH7_VOLT;
  return slope * v + intercept;
}

float readPH(int pin) {
  // pH probes are noisy/high impedance: average more samples
  float adcAvg = readAdcAverage(pin, 50, 10);
  float v = adcToVolts(adcAvg);
  return voltageToPH(v);
}

// -------------------- Setup / Loop --------------------
void setup() {
  delay(5000);
  Serial.begin(115200);

  sensors1.begin();
  sensors2.begin();

  pinMode(level1, INPUT_PULLUP);
  pinMode(level2, INPUT_PULLUP);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relayPump, OUTPUT);

  // Start relays OFF
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relayPump, LOW);

  // ESP32 ADC setup
  analogReadResolution(12);       // 0..4095
  analogSetAttenuation(ADC_11db); // best for near-3.3V range
}

void loop() {
  // Floats: with INPUT_PULLUP, LOW means switch closed (to GND)
  const bool float1Closed = (digitalRead(level1) == LOW);
  const bool float2Closed = (digitalRead(level2) == LOW);

  Serial.println(float1Closed ? "FLOAT1: CLOSED" : "FLOAT1: OPEN");
  Serial.println(float2Closed ? "FLOAT2: CLOSED" : "FLOAT2: OPEN");

  // Relay actions (kept same behaviour, just simplified)
  digitalWrite(relayPump, float1Closed ? HIGH : LOW);
  digitalWrite(relay2,    float2Closed ? HIGH : LOW);

  // Temperatures
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();

  const float t1 = sensors1.getTempCByIndex(0);
  const float t2 = sensors2.getTempCByIndex(0);

  if (t1 == DEVICE_DISCONNECTED_C || t2 == DEVICE_DISCONNECTED_C) {
    Serial.println("Temp sensor missing!");
  } else {
    Serial.printf("Temp 1 (GPIO19): %.0f C\n", t1);
    Serial.printf("Temp 2 (GPIO18): %.0f C\n", t2);
  }

  // AC current (corrected for 6 turns)
  const float acA = readACCurrentAmps(AC_PIN);
  Serial.printf("AC Current (turns=%d): %.2f A\n", CT_TURNS, acA);

  // pH
  const float ph = readPH(PH_PIN);
  Serial.printf("pH: %.2f\n", ph);

  Serial.println("----");
  delay(1000);
}
