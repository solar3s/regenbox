#define PIN_CHARGE    4        // output pin (charge)
#define PIN_DISCHARGE 3        // output pin (discharge)
#define PIN_LED       13       // output pin (arduino led)
#define PIN_ANALOG    A0       // analog pin on battery-0 voltage

// config parameters for getVoltage()
#define REF_VOLTAGE   2460     // tension de reference, tension sur la pin AREF
#define SOME_NUMBER   1023     // ?
#define NB_ANALOG_RD  10       // how many analog read to average on

#define INTERVAL      3600000  //nombre de seconde dans une heure


void setCharge(boolean b) {
  Serial.print("set charge: ");
  Serial.println(b);
  digitalWrite(PIN_CHARGE, !b);
}

void setDischarge(boolean b) {
  Serial.print("set discharge: ");
  Serial.println(b);
  digitalWrite(PIN_DISCHARGE, b);
}

void setLed(boolean b) {
  Serial.print("set led: ");
  Serial.println(b);
  digitalWrite(PIN_LED, b);
}

unsigned long getAnalog() {
  return analogRead(PIN_ANALOG);
}

unsigned long getVoltage() {
  unsigned long tmp, sum;
  for(byte i=0; i < NB_ANALOG_RD; i++){
    tmp = getAnalog();
    sum = sum + tmp;
  }
  sum = sum / NB_ANALOG_RD;
  sum = (sum * REF_VOLTAGE) / SOME_NUMBER; // todo - comment on values used here
  return sum;
}

void setup() {
  Serial.begin(9600);
  //analogReference(EXTERNAL);      //reference de tension pour les mesures
  pinMode(PIN_CHARGE, OUTPUT);
  pinMode(PIN_DISCHARGE, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  setCharge(0);
  setDischarge(0);
  setLed(1);
}

boolean hi = false;

void loop() {
  if(!hi && Serial.available()){
    hi = true;
    Serial.read();
    Serial.println("hi :)");
    Serial.println("just doing some A0 reads, don't mind me");
  }

  if (hi) {
    delay(2000);
    Serial.print(getVoltage());
    Serial.print("mV - A0: ");
    Serial.println(getAnalog());
  }
}
