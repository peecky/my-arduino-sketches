#include <DHT.h>
#include <IRremote.h>
#include "common.h" // link (or simply copy) ../common.h

enum AirConIROperation { noop, turnOn, turnOff };

unsigned long now;

const int LED1 = LED_BUILTIN;
int led1State, led1DefaultState = HIGH;

DHT dht1(2, DHT22);
float temperature;
float maxTemperature = 31.4f;
float minTemperature = 29.9f;

unsigned long sleepModeBeginAt = 0L;
const unsigned long sleepModeDuration = 21600000L; // 6 hour

const unsigned long turnOnLGAirCon = 0x8800E0EL; // 0x8800E0E: 29C. 0x8800F0F: 30C. See http://blog.iolate.kr/235
const unsigned long turnOffLGAirCon = 0x88C0051L;
IRsend irsend;
bool irSendingInactivated = false;

const unsigned long minimumSendingIRInterval = 60000L; // 1 min
const unsigned long minimumSendingSameIRInterval = 300000L; // 5 min
unsigned long irSentAt = 0L;
AirConIROperation lastIROperation = noop;

inline void sendLG(unsigned long code) {
  irsend.sendLG(code, 28);
  irSentAt = now;
}

inline float getSleepModeProgress() {
  return static_cast<float>(now - sleepModeBeginAt) / sleepModeDuration;
}

inline void printStat() {
  SerialPrintValue("Temperature", temperature);
  SerialPrintValue("MAX (4/5)", maxTemperature);
  SerialPrintValue("MIN (6/7)", minTemperature);
  SerialPrintValue("irSentAt", irSentAt);
  SerialPrintValue("irSendingInactivated (0)", irSendingInactivated);
  SerialPrintValue("sleepModeBeginAt (2)", sleepModeBeginAt);
  if (sleepModeBeginAt) SerialPrintValue("sleepModeProgress", getSleepModeProgress());
  SerialPrintValue("now", now);
  Serial.println("");
}

inline void getTemperature() {
  float t = dht1.readTemperature();
  if (isnan(t)) {
    digitalWrite(LED1, toggleDigitalValue(led1State));
    Serial.println("Fail to read temperature");
  } else {
    temperature = t;
    if (led1State != led1DefaultState) digitalWrite(LED1, led1State = led1DefaultState);
  }
}

inline void controlAirCon() {
  if (irSendingInactivated) return;
  const unsigned long irSendingInterval = now - irSentAt;
  if (irSentAt != 0 && irSendingInterval < minimumSendingIRInterval) return;

  const float outboundTemperatureDiff = 10.0f;
  const float sleepModeTemperatureDiff = 2.0f;

  const int maxEdgeTemperatureHitCount = 5;
  static int edgeTemperatureHitCount = 0;
  AirConIROperation irOperation = noop;

  float minTemperature = ::minTemperature;
  float maxTemperature = ::maxTemperature;
  if (sleepModeBeginAt) {
    float sleepModeProgress = getSleepModeProgress();
    if (sleepModeProgress > 1.0f) sleepModeProgress = 1.0f;
    const float temperatureDiff = sleepModeTemperatureDiff * sleepModeProgress;
    minTemperature += temperatureDiff;
    maxTemperature += temperatureDiff;
  }

  if (temperature <= minTemperature) {
    if (minTemperature - temperature < outboundTemperatureDiff) {
      irOperation = turnOff;
    }
  } else if (temperature >= maxTemperature) {
    if (temperature - maxTemperature < outboundTemperatureDiff) {
      irOperation = turnOn;
    }
  }

  if (irOperation != noop) {
    if (irOperation == lastIROperation && irSendingInterval < minimumSendingSameIRInterval) return;

    edgeTemperatureHitCount += 1;
    if (edgeTemperatureHitCount >= maxEdgeTemperatureHitCount) {
      edgeTemperatureHitCount = 0;
      lastIROperation = irOperation;
      if (irOperation == turnOn) {
        SerialPrintln("Turn On");
        sendLG(turnOnLGAirCon);
      } else if (irOperation == turnOff) {
        SerialPrintln("Turn Off");
        sendLG(turnOffLGAirCon);
      }
    }
  } else {
    edgeTemperatureHitCount = 0;
  }
}

inline void toggleSleepMode() {
  if (sleepModeBeginAt) {
    sleepModeBeginAt = 0L; // turn off sleep mode
    led1DefaultState = HIGH;
  } else {
    sleepModeBeginAt = now;
    if (!sleepModeBeginAt) sleepModeBeginAt = 1; // prevent unexpected turning off
    led1DefaultState = LOW;
  }
  digitalWrite(LED1, led1State = led1DefaultState);
}

inline void handleSerialInput() {
  while (Serial.available() > 0) {
    const float temperatureDiffUnit = 0.1f;

    int c = Serial.read();
    switch (c) {
      case '1':
      printStat();
      break;

      case '2':
      toggleSleepMode();
      break;

      case '3':
      sleepModeBeginAt -= 3600000L; // 1 hour
      break;

      case '4':
      maxTemperature -= temperatureDiffUnit;
      break;

      case '5':
      maxTemperature += temperatureDiffUnit;
      break;

      case '6':
      minTemperature -= temperatureDiffUnit;
      break;

      case '7':
      minTemperature += temperatureDiffUnit;
      break;

      case '8':
      sendLG(turnOffLGAirCon);
      break;

      case '9':
      sendLG(turnOnLGAirCon);
      break;

      case '0':
      irSendingInactivated = !irSendingInactivated;
      break;
    }
  }
}

void setup() {
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, led1State = led1DefaultState);
  Serial.begin(9600);
  dht1.begin();
}

void loop() {
  delay(2000);

  now = millis();

  getTemperature();
  controlAirCon();
  handleSerialInput();
}
