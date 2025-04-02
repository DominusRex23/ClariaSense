#include <PinChangeInterrupt.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define FLOW1_PIN 3
#define FLOW2_PIN 4
#define FLOW3_PIN 5
#define TURBIDITY_PIN A0

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

    // Capture pulses safely
    noInterrupts();
    unsigned int p1 = flow1_pulses;
    unsigned int p2 = flow2_pulses;
    unsigned int p3 = flow3_pulses;
    flow1_pulses = flow2_pulses = flow3_pulses = 0;
    interrupts();

    // Convert to flow (L/min)
    float f1 = (p1 / FLOW1_CAL);
    float f2 = (p2 / FLOW2_CAL);
    float f3 = (p3 / FLOW3_CAL);

    // Read temp
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(insideThermometer);

    // Read turbidity
    int turbRaw = analogRead(TURBIDITY_PIN);
    float turbVolt = turbRaw * (5.0 / 1023.0);
    float ntu = (4.5 - turbVolt) * 30;

    // Send JSON
    String json = "{";
    json += "\"l1\":" + String(f1, 2) + ",";
    json += "\"l2\":" + String(f2, 2) + ",";
    json += "\"l3\":" + String(f3, 2) + ",";
    json += "\"temp\":" + String(tempC, 2) + ",";
    json += "\"ntu\":" + String(ntu, 2);
    json += "}\n";

    Serial.print(json);  // Send to ESP32

    // Debug
    Serial.print("Sent: "); Serial.print(json);
  }
}

void ISR_Flow1() { flow1_pulses++; }
void ISR_Flow2() { flow2_pulses++; }
void ISR_Flow3() { flow3_pulses++; }
