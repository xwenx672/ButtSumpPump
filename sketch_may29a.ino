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
float sum = 0;
float average = 0;
void setup() {
  Serial.begin(115200);
  pinMode(sumpSensorPin, INPUT);
  pinMode(buttSensorPin, INPUT);
  pinMode(pumpRelayPin, OUTPUT);
  pinMode(solenoidRelayPin, OUTPUT);
  digitalWrite(solenoidRelayPin, LOW);
}
void loop() {
  // Measure the HIGH and LOW pulse durations
  sum = 0;
  average = 0;
  BSV = sum/100;
  sumpHighTime = pulseIn(sumpSensorPin, HIGH);
  sumpLowTime = pulseIn(sumpSensorPin, LOW);
  buttHighTime = pulseIn(buttSensorPin, HIGH);
  buttLowTime = pulseIn(buttSensorPin, LOW);
  // Serial.println(highTime);
  // Serial.println(lowTime);
  if (sumpHighTime > 0 && sumpLowTime > 0) {
    sumpPeriod = sumpHighTime + sumpLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    SSV = 1e6 / sumpPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }

  if (buttHighTime > 0 && buttLowTime > 0) {
    buttPeriod = buttHighTime + buttLowTime;  // One cycle is being read here, by measuring the time of the pin being high, and the time of the pin being low, then these times are added together to get the period.
    BSV = 1e6 / buttPeriod;     // Convertion from period to frequency is 1(s)/period(s)=frequency(hz).
  }
    //for (int p=0;p<20;p++){
    //  Serial.println(" ");
    //}
     Serial.print(SSV);
    Serial.print(" ");
    Serial.print(BSV);
    Serial.print(" ");
  if (SSV > 100 && BSV < 50) {
  //sump = 1
  //butt = 0
  //pump on
  //solenoid closed
    if (repeatVal > 100) { 
    digitalWrite(pumpRelayPin, HIGH);
    digitalWrite(solenoidRelayPin, LOW);
    Serial.println("1,0,on,closed");
  }
  if (repeatVal < 101) {
        repeatVal++;
    }
  
  } else if (SSV > 100 && BSV > 50 ) {
  //sump = 1
  //butt = 1
  //pump off
  //solenoid open

      digitalWrite(pumpRelayPin, LOW);
      digitalWrite(solenoidRelayPin, HIGH);
      Serial.println("1,1,off,open");
delay(300000);

        repeatVal = 0;

  } else if (SSV < 100) {
  //sump = 0
  //butt = X
  //pump off
  //solenoid closed
    
      digitalWrite(pumpRelayPin, LOW);
      digitalWrite(solenoidRelayPin, LOW);
      Serial.println("0,X,off,closed");
    
        repeatVal = 0;
  }
   
    Serial.println(repeatVal);
  //delay(1000);  // Adjust to suit your sampling rate
}
