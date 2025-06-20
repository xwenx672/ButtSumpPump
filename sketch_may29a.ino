const int sumpSensorPin = 33;  // Change to the GPIO your sensor is connected to
const int buttSensorPin = 25;
const int solenoidRelayPin = 4;
const int pumpRelayPin = 2;
int sumpHighTime;
int buttHighTime;
int sumpLowTime;
int buttLowTime;
int tripLvl = 578;
float voltRead;
int repeatVal = 0;
float BSV = 1;
float SSV = 1;
float buttPeriod = 1;
float sumpPeriod = 1;

void setup() {
  Serial.begin(115200);
  pinMode(sumpSensorPin, INPUT);
  pinMode(buttSensorPin, INPUT);
  pinMode(pumpRelayPin, OUTPUT);
  pinMode(solenoidRelayPin, OUTPUT);
  digitalWrite(solenoidRelayPin, LOW);
}
float sumpRead() {
  sumpHighTime = pulseIn(sumpSensorPin, HIGH);
  sumpLowTime = pulseIn(sumpSensorPin, LOW);
  
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    SSV = 1e6 / sumpPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }
  return SSV;
}

float buttRead() {
  buttHighTime = pulseIn(buttSensorPin, HIGH);
  buttLowTime = pulseIn(buttSensorPin, LOW);
  
  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    BSV = 1e6 / buttPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }
  return BSV;
}


void loop() {
  // Measure the HIGH and LOW pulse durations
  SSV = sumpRead();
  BSV = buttRead();
  Serial.print(SSV);
  Serial.print(" ");
  Serial.print(BSV);
  Serial.print(" ");

  if (SSV > 100 && BSV < 50) {
  //s1,b0,pump on,solenoid closed
    if (repeatVal > 100) { 
      digitalWrite(pumpRelayPin, HIGH);
      digitalWrite(solenoidRelayPin, LOW);
      Serial.println("1,0,on,closed");
    }
    if (repeatVal <= 100) {
        repeatVal++;
    }
  
  } else if (SSV > 100 && BSV > 50 ) {
  //s1,b1,pump off,solenoid open
    digitalWrite(pumpRelayPin, LOW);
    digitalWrite(solenoidRelayPin, HIGH);
    Serial.println("1,1,off,open");
    delay(300000);
    repeatVal = 0;
  } else if (SSV < 100) {
  //s0,bX,pump off,solenoid closed
    digitalWrite(pumpRelayPin, LOW);
    digitalWrite(solenoidRelayPin, LOW);
    Serial.println("0,X,off,closed");
    repeatVal = 0;
  }
  Serial.println(repeatVal);
  //delay(1000);  // Adjust to suit your sampling rate
}
