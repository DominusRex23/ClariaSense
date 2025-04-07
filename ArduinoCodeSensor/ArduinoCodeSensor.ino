#include <PinChangeInterrupt.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define FLOW1_PIN 3
#define FLOW2_PIN 4
#define FLOW3_PIN 5
#define TURBIDITY_PIN A0
#define PH_PIN A1
#define TDS_PIN A2
#define TRIG_PIN 12
#define ECHO_PIN 9

#define FLOW1_CAL 604.0
#define FLOW2_CAL 631.0
#define FLOW3_CAL 633.0

#define MAX_DISTANCE 200

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
float VREF = 5.0;              // ADC reference voltage
float ecCalibration = 93.39;   // Based on your 255.5 ppm @ 0.730V
float temperature = 25.0;      // Adjust if you have a sensor

// TDS sensor read function
int readTDSRaw(int samples = 10) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(TDS_PIN);
    delay(5);
  }
  return total / samples;
}

float computeTDS() {
  int raw = readTDSRaw();
  float voltage = raw * (5.0 / 1023.0);

  // Convert voltage to raw EC (non-calibrated)
  float rawEC = (voltage * 1000) / 133.42;
  float ecValue = rawEC * ecCalibration;

  // Temperature compensation (optional)
  float compensatedEC = ecValue / (1.0 + 0.02 * (temperature - 25.0));

  // TDS = EC × 0.5
  float tdsValue = compensatedEC * 0.5;
  return tdsValue;
}

long microsecondsToInches(long microseconds) {
  // Convert the pulse duration to inches
  return (microseconds / 74) / 2;
}

long microsecondsToCentimeters(long microseconds) {
  // Convert the pulse duration to centimeters
  return (microseconds / 29) / 2;
}

void ISR_Flow1() { flow1_pulses++; }
void ISR_Flow2() { flow2_pulses++; }
void ISR_Flow3() { flow3_pulses++; }

void setup() {
  Serial.begin(9600);
  delay(500);

  pinMode(FLOW1_PIN, INPUT_PULLUP);
  pinMode(FLOW2_PIN, INPUT_PULLUP);
  pinMode(FLOW3_PIN, INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(FLOW1_PIN), ISR_Flow1, RISING);
  attachPCINT(digitalPinToPCINT(FLOW2_PIN), ISR_Flow2, RISING);
  attachPCINT(digitalPinToPCINT(FLOW3_PIN), ISR_Flow3, RISING);

  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) {
    Serial.println("Unable to find temperature sensor");
  } else {
    sensors.setResolution(insideThermometer, 9);
  }

  Serial.println("Arduino ready. Sending data...");
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

    pHVol = (float)avgValue * 5.0 / 1024/ 6;
    phValue = 1.5385 * pHVol + 1.4614;

    // TDS Calculation
    float tds = computeTDS();

    // Ultrasonic Distance Measurement
    long duration, inches, cm;

    // Trigger the sensor to send a pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Read the pulse duration from echoPin
    duration = pulseIn(ECHO_PIN, HIGH);

    // Convert microseconds to inches and centimeters
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);

    // JSON output
    String json = "{";
    json += "\"l1\":" + String(f1, 2) + ",";
    json += "\"l2\":" + String(f2, 2) + ",";
    json += "\"l3\":" + String(f3, 2) + ",";
    json += "\"temp\":" + String(tempC, 2) + ",";
    json += "\"ntu\":" + String(ntu, 2) + ",";
    json += "\"ph\":" + String(phValue, 2) + ",";
    json += "\"tds\":" + String(tds, 2) + ",";
    json += "\"ph_voltage\":" + String(pHVol, 4) + ","; 
    json += "\"distance_in\": " + String(inches); // Add distance in inches
    json += "}\n";

    Serial.print(json);
    Serial.println();
  }
}