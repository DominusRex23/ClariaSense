// === Analog Pins ===
#define PH_PIN 33
#define TDS_PIN 34

// === Relay Pins (Normally Closed) ===
#define RELAY1_PIN 21
#define RELAY2_PIN 19
#define RELAY3_PIN 18
#define RELAY4_PIN 5

// === Calibration ===
const float V4 = 3.20, V7 = 2.38, V10 = 2.14;
const float V_1000 = 1.53, V_1413 = 2.05, V_12880 = 2.28;

// === Safe Ranges & Volumes ===
const float PH_MIN = 6.0, PH_MAX = 8.5;
const float TEMP_MIN = 20.0, TEMP_MAX = 32.0;
const float TDS_TRIGGER = 300.0;
const float NTU_MIN = 20.0, NTU_MAX = 80.0;
const float THREE_GALLONS = 11.35;
const float TWO_GALLONS = 7.57;

// === Sampling Settings ===
const int NUM_MEDIAN_SAMPLES = 15;
const int BUFFER_SIZE = 60;
const float SMOOTHING_ALPHA = 0.2;
const unsigned long SAMPLE_INTERVAL_MS = 1000;
const unsigned long DUMP_DURATION_MS = 10000;

// === Flowless Test Timers ===
bool testUseTimers = false;
unsigned long drainStartTime = 0;
unsigned long fillStartTime = 0;
const unsigned long DRAIN_TEST_DURATION_MS = 8000;
const unsigned long FILL_TEST_DURATION_MS  = 8000;

// === Buffers ===
float pH_voltageBuffer[BUFFER_SIZE];
float tempBuffer[BUFFER_SIZE];
float turbBuffer[BUFFER_SIZE];
float tdsVoltageBuffer[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastSampleTime = 0;
float filteredTDSVoltage = 0;

// === Mode Tracking ===
enum SystemMode { CONTINUOUS, DRAIN, DUMP, FILL };
SystemMode currentMode = CONTINUOUS;
float slope1, slope2, avgSlope;
float drainedLiters = 0.0, filledLiters = 0.0;
float targetDrainLiters = 0.0, targetFillLiters = 0.0;
unsigned long dumpStartTime = 0;

// === Arduino Data ===
String unoData = "";
float latestL1 = 0.0, latestL3 = 0.0;
float latestTemp = 0.0, latestTurbidity = 0.0;

// === Manual Test Mode ===
bool manualTestMode = true;
float testPH = 7.0, testTDS = 250.0, testTemp = 25.0, testNTU = 50.0;

// === Relays ===
void setModeContinuous() {
  Serial.println("🔁 Continuous Mode");
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);
}

void setModeDrain() {
  Serial.println("💧 Drain Mode");
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);
  drainStartTime = millis();
}

void setModeDump() {
  Serial.println("🗑️ Dump Mode");
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, HIGH);
  dumpStartTime = millis();
}

void setModeFill() {
  Serial.println("🚿 Fill Mode");
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, LOW);
  fillStartTime = millis();
}

// === Helpers ===
void sortArray(int arr[], int size) {
  for (int i = 0; i < size - 1; i++)
    for (int j = i + 1; j < size; j++)
      if (arr[j] < arr[i]) {
        int temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
      }
}

float getFilteredPHVoltage() {
  int samples[NUM_MEDIAN_SAMPLES];
  for (int i = 0; i < NUM_MEDIAN_SAMPLES; i++) {
    samples[i] = analogRead(PH_PIN);
    delayMicroseconds(100);
  }
  sortArray(samples, NUM_MEDIAN_SAMPLES);
  return samples[NUM_MEDIAN_SAMPLES / 2] * (3.3 / 4095.0);
}

int readTDSRaw(int pin, int samples = 10) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(10);
  }
  return total / samples;
}

float smoothVoltage(float current, float previous, float alpha = SMOOTHING_ALPHA) {
  return previous + alpha * (current - previous);
}

int evaluateTrigger(float ph, float tds, float temp, float ntu) {
  if (tds > TDS_TRIGGER || ntu < NTU_MIN || ntu > NTU_MAX) return 3;
  if (ph < PH_MIN || ph > PH_MAX || temp < TEMP_MIN || temp > TEMP_MAX) return 2;
  return 0;
}

void simulateManualTestValues(float ph, float tds, float temp, float ntu) {
  manualTestMode = true;
  testPH = ph;
  testTDS = tds;
  testTemp = temp;
  testNTU = ntu;
  Serial.printf("🧪 Manual test mode ON: pH=%.2f, TDS=%.1f, Temp=%.1f, NTU=%.1f\n", ph, tds, temp, ntu);
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);

  slope1 = (4.00 - 7.00) / (V4 - V7);
  slope2 = (7.00 - 10.00) / (V7 - V10);
  avgSlope = (slope1 + slope2) / 2.0;

  Serial.println("✅ ESP32 READY");
  setModeContinuous();  // Start in Continuous Mode
}

// === Main Loop ===
void loop() {
  unsigned long now = millis();

  // === Serial Command Input ===
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("test:")) {
      cmd.remove(0, 5);
      int i1 = cmd.indexOf(',');
      int i2 = cmd.indexOf(',', i1 + 1);
      int i3 = cmd.indexOf(',', i2 + 1);
      if (i1 != -1 && i2 != -1 && i3 != -1) {
        float ph = cmd.substring(0, i1).toFloat();
        float tds = cmd.substring(i1 + 1, i2).toFloat();
        float temp = cmd.substring(i2 + 1, i3).toFloat();
        float ntu = cmd.substring(i3 + 1).toFloat();
        simulateManualTestValues(ph, tds, temp, ntu);
      }
    } else if (cmd == "reset") {
      manualTestMode = false;
      Serial.println("✅ Manual test OFF.");
    } else if (cmd == "flowless") {
      testUseTimers = true;
      Serial.println("🧪 Flowless test mode ENABLED.");
    } else if (cmd == "flowon") {
      testUseTimers = false;
      Serial.println("✅ Flow meter logic RE-ENABLED.");
    }
  }

  // === Arduino JSON Input Parsing ===
  while (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    if (!data.startsWith("{")) continue;

    unoData = data;
    int l1 = unoData.indexOf("\"l1\":");
    int l3 = unoData.indexOf("\"l3\":");
    int temp = unoData.indexOf("\"temp\":");
    int ntu = unoData.indexOf("\"ntu\":");

    if (l1 != -1 && l3 != -1 && temp != -1 && ntu != -1) {
      latestL1 = unoData.substring(l1 + 5, unoData.indexOf(',', l1)).toFloat();
      latestL3 = unoData.substring(l3 + 5, unoData.indexOf(',', l3)).toFloat();
      latestTemp = unoData.substring(temp + 7, unoData.indexOf(',', temp)).toFloat();
      latestTurbidity = unoData.substring(ntu + 6, unoData.indexOf('}', ntu)).toFloat();

      if (!testUseTimers) {
        if (currentMode == DRAIN) {
          drainedLiters += latestL1;
          Serial.printf("💧 Draining... %.2f L total\n", drainedLiters);
        }
        if (currentMode == FILL) {
          filledLiters += latestL3;
          Serial.printf("🚿 Filling... %.2f L total\n", filledLiters);
        }
      }
    }
  }

  // === Simulated Counters for Timer Mode ===
  if (testUseTimers) {
    if (currentMode == DRAIN) {
      Serial.printf("💧 Draining (Timer)... %lus elapsed\n", (millis() - drainStartTime) / 1000);
    }
    if (currentMode == FILL) {
      Serial.printf("🚿 Filling (Timer)... %lus elapsed\n", (millis() - fillStartTime) / 1000);
    }
  }

  // === Sensor Averaging & Logic ===
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = millis();

    float pH_V = getFilteredPHVoltage();
    pH_voltageBuffer[bufferIndex] = pH_V;
    tempBuffer[bufferIndex] = latestTemp;
    turbBuffer[bufferIndex] = latestTurbidity;

    float tds_V = readTDSRaw(TDS_PIN) * (3.3 / 4095.0);
    filteredTDSVoltage = smoothVoltage(tds_V, filteredTDSVoltage);
    tdsVoltageBuffer[bufferIndex] = filteredTDSVoltage;

    bufferIndex++;

    if (bufferIndex >= BUFFER_SIZE) {
      float avgPH_V = 0, avgTemp = 0, avgNTU = 0, avgTDS_V = 0;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        avgPH_V += pH_voltageBuffer[i];
        avgTemp += tempBuffer[i];
        avgNTU += turbBuffer[i];
        avgTDS_V += tdsVoltageBuffer[i];
      }

      avgPH_V /= BUFFER_SIZE;
      avgTemp /= BUFFER_SIZE;
      avgNTU /= BUFFER_SIZE;
      avgTDS_V /= BUFFER_SIZE;

      float avgPH = 7.00 - ((avgPH_V - V7) * avgSlope);
      float EC = (avgTDS_V <= V_1413)
        ? ((avgTDS_V - V_1000) / (V_1413 - V_1000)) * (1413 - 1000) + 1000
        : ((avgTDS_V - V_1413) / (V_12880 - V_1413)) * (12880 - 1413) + 1413;
      float TDS = EC * 0.5;

      if (manualTestMode) {
        avgPH = testPH;
        avgTemp = testTemp;
        avgNTU = testNTU;
        TDS = testTDS;
      }

      Serial.printf("\n[AVG] pH: %.2f | Temp: %.2f°C | NTU: %.2f | TDS: %.1f ppm\n", avgPH, avgTemp, avgNTU, TDS);

      int trigger = evaluateTrigger(avgPH, TDS, avgTemp, avgNTU);

      if (currentMode == CONTINUOUS && trigger > 0) {
        currentMode = DRAIN;
        targetDrainLiters = (trigger == 3) ? THREE_GALLONS : TWO_GALLONS;
        targetFillLiters = targetDrainLiters;
        drainedLiters = 0;
        filledLiters = 0;
        setModeDrain();
      }
      else if (currentMode == DRAIN &&
        ((testUseTimers && millis() - drainStartTime >= DRAIN_TEST_DURATION_MS) ||
         (!testUseTimers && drainedLiters >= targetDrainLiters))) {
        currentMode = DUMP;
        setModeDump();
      }
      else if (currentMode == DUMP &&
               (millis() - dumpStartTime >= DUMP_DURATION_MS)) {
        currentMode = FILL;
        setModeFill();
      }
      else if (currentMode == FILL &&
               ((testUseTimers && millis() - fillStartTime >= FILL_TEST_DURATION_MS) ||
                (!testUseTimers && filledLiters >= targetFillLiters))) {
        currentMode = CONTINUOUS;
        setModeContinuous();
      }

      bufferIndex = 0;
    }
  }
}
