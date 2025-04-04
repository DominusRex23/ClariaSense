#include <PinChangeInterrupt.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define FLOW1_PIN 3
#define FLOW2_PIN 4
#define FLOW3_PIN 5
#define TURBIDITY_PIN A0
#define PH_PIN A1
#define TDS_PIN A2

#define FLOW1_CAL 604.0
#define FLOW2_CAL 631.0
#define FLOW3_CAL 633.0

volatile unsigned int flow1_pulses = 0;
volatile unsigned int flow2_pulses = 0;
volatile unsigned int flow3_pulses = 0;

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

// pH variables
int buf[10], temp;
unsigned long int avgValue;
float pHVol, phValue;

// TDS variables
float V_1000 = 1.53;
float V_1413 = 2.05;
float V_12880 = 2.28;
float filteredTDSVoltage = 0.0;

int readTDSRaw(int samples = 10) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(TDS_PIN);
    delay(5);
  }
  return total / samples;
}

float smoothVoltage(float current, float previous, float alpha = 0.2) {
  return previous + alpha * (current - previous);
}

float computeTDS() {
  int raw = readTDSRaw();
  float voltage = raw * (5.0 / 1023.0);
  filteredTDSVoltage = smoothVoltage(voltage, filteredTDSVoltage);

  float EC;
  if (filteredTDSVoltage <= V_1413) {
    EC = ((filteredTDSVoltage - V_1000) / (V_1413 - V_1000)) * (1413 - 1000) + 1000;
  } else {
    EC = ((filteredTDSVoltage - V_1413) / (V_12880 - V_1413)) * (12880 - 1413) + 1413;
  }

  return EC * 0.5;
}

void setup() {
  Serial.begin(9600);
  delay(500);

  pinMode(FLOW1_PIN, INPUT_PULLUP);
  pinMode(FLOW2_PIN, INPUT_PULLUP);
  pinMode(FLOW3_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FLOW1_PIN), ISR_Flow1, RISING);
  attachPCINT(digitalPinToPCINT(FLOW2_PIN), ISR_Flow2, RISING);
  attachPCINT(digitalPinToPCINT(FLOW3_PIN), ISR_Flow3, RISING);

  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) {
    Serial.println("Unable to find temperature sensor");
  } else {
    sensors.setResolution(insideThermometer, 9);
  }

  Serial.println("Arduino ready. Sending data to ESP32...");
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long now = millis();

  if (now - lastTime >= 1000) {
    lastTime = now;

    // Flow pulses
    noInterrupts();
    unsigned int p1 = flow1_pulses;
    unsigned int p2 = flow2_pulses;
    unsigned int p3 = flow3_pulses;
    flow1_pulses = flow2_pulses = flow3_pulses = 0;
    interrupts();

    float f1 = (p1 / FLOW1_CAL);
    float f2 = (p2 / FLOW2_CAL);
    float f3 = (p3 / FLOW3_CAL);

    // Temperature
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(insideThermometer);

    // Turbidity
    int turbRaw = analogRead(TURBIDITY_PIN);
    float turbVolt = turbRaw * (5.0 / 1023.0);
    float ntu = (4.5 - turbVolt) * 30;

    // pH
    for (int i = 0; i < 10; i++) {
      buf[i] = analogRead(PH_PIN);
      delay(30);
    }

    for (int i = 0; i < 9; i++) {
      for (int j = i + 1; j < 10; j++) {
        if (buf[i] > buf[j]) {
          temp = buf[i];
          buf[i] = buf[j];
          buf[j] = temp;
        }
      }
    }

    avgValue = 0;
    for (int i = 2; i < 8; i++)
      avgValue += buf[i];

    pHVol = (float)avgValue * 5.0 / 1024 / 6;
    phValue = -2.2556 * pHVol + 11.841;

    // TDS
    float tds = computeTDS();

    // JSON output
    String json = "{";
    json += "\"l1\":" + String(f1, 2) + ",";
    json += "\"l2\":" + String(f2, 2) + ",";
    json += "\"l3\":" + String(f3, 2) + ",";
    json += "\"temp\":" + String(tempC, 2) + ",";
    json += "\"ntu\":" + String(ntu, 2) + ",";
    json += "\"ph\":" + String(phValue, 2) + ",";
    json += "\"tds\":" + String(tds, 0);
    json += "}\n";

    Serial.print(json);
  }
}

void ISR_Flow1() { flow1_pulses++; }
void ISR_Flow2() { flow2_pulses++; }
void ISR_Flow3() { flow3_pulses++; }