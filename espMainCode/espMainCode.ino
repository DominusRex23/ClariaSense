#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"
#include <ArduinoJson.h>

#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "password"
#define API_KEY "api-key"
#define FIREBASE_PROJECT_ID "project-id"
#define DATABASE_URL "databaseurl"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// 🔹 NTP Configuration (Manila Time GMT+8)
const char* ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // GMT+8 Offset
const int daylightOffset_sec = 0; // No daylight saving in Manila

int lastLoggedHour = -1; // 🔹 Stores the last logged hour to prevent duplicates
bool signupOK = false;  // Declare globally

unsigned long sendDataPrevMillis = 0; // Declare to avoid scope issues

#define RELAY_1 21
#define RELAY_2 19
#define RELAY_3 18
#define RELAY_4 5

float latestTemp = 0.0, latestTDS = 0.0, latestPH = 0.0;
float latestL1 = 0.0, latestL2 = 0.0, latestL3 = 0.0;

int mode = 1;
float drainLiters = 0.0;
float fillLiters = 0.0;
unsigned long fillStartTime = 0;
unsigned long fillDuration = 0;
unsigned long dumpStartTime = 0;

unsigned long lastLoggedTime = 0;  // Variable to track last log time for hourly logs
unsigned long lastParamLogTime = 0;  // Variable to track last log time for parameter out-of-range logs

void stopAllRelays() {
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
  digitalWrite(RELAY_3, HIGH);
  digitalWrite(RELAY_4, HIGH);
}

void checkMode() {
  unsigned long now = millis();

  switch (mode) {
    case 1: { // Continuous Mode
      if (latestTDS < 300 &&
          latestPH < 8.9 &&
          latestTemp < 33 /*&&
          latestTurbidity < 80*/) {
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        Serial.println("✅ Mode 1: Continuous Mode ACTIVE (All parameters OK)");
      } else {
        stopAllRelays();
        mode = 2; // Switch to Drain/Fill Mode
        drainLiters = 0.0;
        fillLiters = 0.0;
        Serial.println("⚠️ Parameters out of range! Switching to Mode 2 (Drain/Fill Mode)");
      }
      break;
    }

    case 2: { // Drain/Fill Mode
      drainLiters += latestL1; // Drain amount (L)
      fillLiters += latestL3;  // Fill amount (L)
      digitalWrite(RELAY_1, LOW); // Activate drain relay
      digitalWrite(RELAY_4, LOW); // Activate fill relay
      
      Serial.print("🚰 Mode 2: Draining/Filling... Drained: ");
      Serial.print(drainLiters, 2);
      Serial.print(" L | Filled: ");
      Serial.print(fillLiters, 2);
      Serial.println(" L");

      // If drainage reaches 5L, switch to dump mode
      if (drainLiters >= 5.0) {
        stopAllRelays();
        mode = 3; // Go to Dump Mode (was previously setting to mode 4)
        dumpStartTime = millis(); // Start time for dump mode
        Serial.println("✅ Drain complete. Switching to Mode 3 (Dump Mode)");
      }
      break;
    }

    case 3: { // Dump Mode (Continuous, 60 seconds only)
      unsigned long dumpElapsed = (millis() - dumpStartTime) / 1000;
      digitalWrite(RELAY_3, LOW); // Activate dump relay
      
      Serial.print("🗑️ Mode 3: Dumping... Time: ");
      Serial.print(dumpElapsed);
      Serial.println(" sec | Target: 60 sec");

      // Dump for 60 seconds
      if (dumpElapsed >= 60) {
        stopAllRelays();
        mode = 1; // Return to Continuous Mode
        Serial.println("✅ Dump complete. Returning to Mode 1 (Continuous Mode)");
      }
      break;
    }
  }
}

// Function to log data once per hour (includes date)
void logHourlyData(float latestTemp, float latestTDS, float latestPH) {
    static String lastLoggedHour = "";
    static float tempBuf[10], tdsBuf[10], phBuf[10];
    static int sampleIndex = 0;
    static bool dataLoggedThisHour = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    char hourKey[20]; // e.g., 2025-04-04_13
    strftime(hourKey, sizeof(hourKey), "%Y-%m-%d_%H", &timeinfo);
    String currentHour = String(hourKey);
    int secondNow = timeinfo.tm_sec;
    // New hour, reset buffer & state
    if (currentHour != lastLoggedHour) {
        lastLoggedHour = currentHour;
        sampleIndex = 0;
        dataLoggedThisHour = false;
    }
    // Collect samples in first 10 seconds
    if (!dataLoggedThisHour && secondNow < 10) {
        if (sampleIndex < 10) {
            tempBuf[sampleIndex] = latestTemp;
            tdsBuf[sampleIndex] = latestTDS;
            phBuf[sampleIndex] = latestPH;
            sampleIndex++;
        }
        // Once 10 samples collected, log to Firestore
        if (sampleIndex == 10) {
            char timestamp[25];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:00:00", &timeinfo);
            String documentPath = "hourly_logs/" + String(timestamp);
            FirebaseJson content;
            
            // Create proper Firestore array values for each sensor type
            FirebaseJsonArray tempArray, tdsArray, phArray;
            
            // Add values to arrays with proper Firestore format
            for (int i = 0; i < 10; i++) {
                FirebaseJson tempValue, tdsValue, phValue;
                
                tempValue.set("doubleValue", tempBuf[i]);
                tempArray.add(tempValue);
                
                tdsValue.set("doubleValue", tdsBuf[i]);
                tdsArray.add(tdsValue);
                
                phValue.set("doubleValue", phBuf[i]);
                phArray.add(phValue);
                
            }
            
            // Assign arrays to content
            content.set("fields/temp/arrayValue/values", tempArray);
            content.set("fields/tds/arrayValue/values", tdsArray);
            content.set("fields/ph/arrayValue/values", phArray);
            content.set("fields/timestamp/stringValue", String(timestamp));
            
            Serial.print("Logging 10-sample hourly data to Firestore... ");
            if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
                Serial.println("Success!");
                dataLoggedThisHour = true;
            } else {
                Serial.println("Error: " + fbdo.errorReason());
            }
        }
    }
}


void logParameterOutOfRange(float latestTemp, float latestTDS, float latestPH) {
    // Check if any parameter is out of range
    if ((latestTemp > 32) ||
        (latestTDS > 299) ||
        (latestPH > 8.9)) {
        
        String timestamp = getFormattedTime(); // Get Manila time
        
        // Create unique document ID that's Firestore-compatible
        // Replace any characters that aren't allowed in Firestore document IDs
        String docID = timestamp;
        docID.replace(":", "-");
        docID.replace(" ", "_");
        docID.replace("/", "-");
        
        String documentPath = "error_logs/" + docID; // Store logs under "error_logs" collection

        // Create FirebaseJson object for Firestore data
        FirebaseJson content;
        
        // Set proper field values with correct types
        content.set("fields/temp/doubleValue", latestTemp);
        content.set("fields/tds/doubleValue", latestTDS);
        content.set("fields/ph/doubleValue", latestPH);
        content.set("fields/timestamp/stringValue", timestamp);
        
        // Also add which parameter(s) triggered the error
        FirebaseJsonArray errorParamsArray;
        
        if (latestTemp > 32) {
            FirebaseJson errorParam;
            errorParam.set("stringValue", "temperature");
            errorParamsArray.add(errorParam);
        }
        if (latestTDS > 299) {
            FirebaseJson errorParam;
            errorParam.set("stringValue", "TDS");
            errorParamsArray.add(errorParam);
        }
        if (latestPH > 8.9) {
            FirebaseJson errorParam;
            errorParam.set("stringValue", "pH");
            errorParamsArray.add(errorParam);
        }
        /* Uncomment when you want to include turbidity*/
        
        
        content.set("fields/errorParameters/arrayValue/values", errorParamsArray);

        // Log data to Firestore
        Serial.print("Logging out-of-range parameters to Firestore... ");
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
            Serial.println("Success!");
        } else {
            Serial.println("Error: " + fbdo.errorReason());
        }
    }
}


String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⚠️ Failed to obtain time");
    return "Unknown";
  }

  char timeStr[30]; // Buffer for formatted time
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(timeStr);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // 🔹 Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

  // 🔹 Firebase Setup
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // 🔹 Anonymous Firebase Authentication
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Anonymous authentication successful.");
    signupOK = true; // ✅ Now it is declared and updated
  }else {
    Serial.printf("Firebase SignUp Error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  
  Firebase.reconnectWiFi(true);

  // 🔹 Configure and Sync NTP Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  delay(2000);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);
  stopAllRelays();
}

void loop() {
  while (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    if (data.startsWith("{")) {
      latestL1 = data.substring(data.indexOf("\"l1\":") + 5, data.indexOf(',', data.indexOf("\"l1\":"))).toFloat();
      latestL2 = data.substring(data.indexOf("\"l2\":") + 5, data.indexOf(',', data.indexOf("\"l2\":"))).toFloat();
      latestL3 = data.substring(data.indexOf("\"l3\":") + 5, data.indexOf(',', data.indexOf("\"l3\":"))).toFloat();
      latestTemp = data.substring(data.indexOf("\"temp\":") + 7, data.indexOf(',', data.indexOf("\"temp\":"))).toFloat();
      latestPH = data.substring(data.indexOf("\"ph\":") + 5, data.indexOf(',', data.indexOf("\"ph\":"))).toFloat();
      latestTDS = data.substring(data.indexOf("\"tds\":") + 6, data.indexOf('}', data.indexOf("\"tds\":"))).toFloat();
      checkMode();
    }
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Firebase.RTDB.setFloat(&fbdo, "sensors/temp", latestTemp);
    Firebase.RTDB.setFloat(&fbdo, "sensors/tds", latestTDS);
    Firebase.RTDB.setFloat(&fbdo, "sensors/ph", latestPH);
  }

  // Call the functions to log hourly and out-of-range data
    logHourlyData(latestTemp, latestTDS, latestPH);
    logParameterOutOfRange(latestTemp, latestTDS, latestPH);
}
