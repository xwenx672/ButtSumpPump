const int sumpSensorPin = 33;  // Change to the GPIO your sensor is connected to
const int buttSensorPin = 25;
const int valveRelayPin = 4;
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
  pinMode(valveRelayPin, OUTPUT);
  digitalWrite(valveRelayPin, HIGH);
  delay(10000);
}
float sumpRead() {
  sumpHighTime = pulseIn(sumpSensorPin, HIGH, 1000000);
  sumpLowTime = pulseIn(sumpSensorPin, LOW, 1000000);
  
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    SSV = 1e6 / sumpPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }
  return SSV;
}

float buttRead() {
  buttHighTime = pulseIn(buttSensorPin, HIGH, 1000000);
  buttLowTime = pulseIn(buttSensorPin, LOW, 1000000);
  
  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    BSV = 1e6 / buttPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }
  return BSV;
}


void loop() {
  SSV = sumpRead();
  BSV = buttRead();
  Serial.print("SSV: ");
  Serial.println(SSV);
  Serial.print("BSV: ");
  Serial.println(BSV);
  Serial.print("repeatVal: ");
  Serial.println(repeatVal);
  
  if (SSV >= 400) {
    if (BSV < 49) {
      //s1,b0,pump on,valve closed
      if (repeatVal > 100) { 
      digitalWrite(pumpRelayPin, HIGH);
      digitalWrite(valveRelayPin, LOW);
      Serial.println("1,0,on,closed");
      }
      if (repeatVal <= 100) {
          repeatVal++;
      }
    } else if (BSV >= 100) {
      //s1,b1,pump off,valve open
      digitalWrite(pumpRelayPin, LOW);
      digitalWrite(valveRelayPin, HIGH);
      Serial.println("1,1,off,open");
      delay(300000);
      repeatVal = 0;
    }
  } else {
    if (repeatVal != -1) {
      //s0,bX,pump off,valve closed
      digitalWrite(pumpRelayPin, LOW);
      digitalWrite(valveRelayPin, HIGH);
      Serial.println("0,X,off,open(temp)");
      delay(300000);
      digitalWrite(valveRelayPin, LOW);
    }
    Serial.println("0,X,off,closed");
    repeatVal = -1;
  }
  Serial.println("repeatVal: ");
  Serial.print(repeatVal);
  Serial.println(" ");
  delay(1000);
}
