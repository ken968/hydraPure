#define BLYNK_TEMPLATE_ID "TMPL6UOE74GoI"
#define BLYNK_TEMPLATE_NAME "HydraPure"
#define BLYNK_AUTH_TOKEN "TY1y90CG1cqAFfHXpE8__aIRDAPfOY_8"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Informasi WiFi dan Blynk
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "AndroidAP_5618";
char pass[] = "12345677";

// Pin
const int TRIGGER_PIN = 11;
const int ECHO_PIN = 12;
const int RELAY_PIN = 23;
const int BUZZER_PIN = 16;
const int LED_PIN = 25;
const int TRIGGER_LEVEL_PIN = 27;
const int ECHO_LEVEL_PIN = 14;
const int LED_POWER_PIN = 17;

// Constants for Object Detection
const float DETECTION_THRESHOLD = 10.0;
const int PUMP_DURATION = 10000;
const int BUZZER_DURATION = 500;

// Constants for Water Level Monitoring (Parameter Galon Baru)
const float EMPTY_GALON_DISTANCE = 34.0; // Jarak sensor ke permukaan galon (tinggi wadah) (cm)
const float FULL_GALON_DISTANCE = 9.0;  // Jarak sensor ke air saat penuh (cm)
const float GALON_TINGGI = 34.0;       // Tinggi galon (cm)
const float GALON_DIAMETER = 15.0;    // Diameter galon (cm)
const float GALON_RADIUS = GALON_DIAMETER / 2.0;
const float GALON_LUAS_ALAS = PI * GALON_RADIUS * GALON_RADIUS; // Luas alas galon (cm^2)

// Global Variables
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
unsigned long pumpStartTime = 0;
bool pumpRunning = false;

BlynkTimer timer;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  Serial.println("ESP32 Water Dispenser & Level Monitoring Starting...");

  // Configure pin modes
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIGGER_LEVEL_PIN, OUTPUT);
  pinMode(ECHO_LEVEL_PIN, INPUT);
  pinMode(LED_POWER_PIN, OUTPUT);

  // Initialize all outputs to OFF state
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_POWER_PIN, HIGH);

  // Initialize WiFi and Blynk
  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Blynk.begin(auth, ssid, pass);

  // Set timer to send water level data to Blynk every 1 detik
  timer.setInterval(1000L, sendWaterLevelData);

  // Wait for system to stabilize
  delay(1000);
  Serial.println("System ready!");
}

// Function to measure distance using HC-SR04 ultrasonic sensor
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  if (duration == 0) {
    return 500;
  }
  float distance = duration * 0.034 / 2.0;
  return distance;
}

void sendWaterLevelData() {
  float waterLevelDistance = measureDistance(TRIGGER_LEVEL_PIN, ECHO_LEVEL_PIN);
  Serial.print("Jarak Sensor Level Air: ");
  Serial.print(waterLevelDistance);
  Serial.println(" cm");

  // Hitung tinggi air: tinggi wadah - jarak sensor
  float waterHeight = GALON_TINGGI - waterLevelDistance;
  waterHeight = constrain(waterHeight, 0, GALON_TINGGI);

  // Hitung volume air (dalam liter)
  float waterVolume = GALON_LUAS_ALAS * waterHeight; // Volume dalam cm^3
  waterVolume = waterVolume / 1000.0;             // Konversi ke liter (1 liter = 1000 cm^3)
  waterVolume = constrain(waterVolume, 0, (GALON_LUAS_ALAS * GALON_TINGGI) / 1000.0); // Batasi volume

  Serial.print("Tinggi Air: ");
  Serial.print(waterHeight);
  Serial.println(" cm");
  Serial.print("Volume Air: ");
  Serial.print(waterVolume);
  Serial.println(" L");

  // Kirim data ke Blynk
  Blynk.virtualWrite(V0, waterHeight); // Virtual Pin untuk Tinggi Air (cm)
  Blynk.virtualWrite(V1, waterVolume); // Virtual Pin untuk Volume Air (L)
}

void loop() {
  Blynk.run();
  timer.run();

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
    delay(5000); // Coba lagi setiap 5 detik
    return;      // Penting: Kembali ke awal loop untuk mencoba koneksi lagi
  }

  // Measure distance for object detection
  float currentDistance = measureDistance(TRIGGER_PIN, ECHO_PIN);

  // Object detection logic
  if (currentDistance < DETECTION_THRESHOLD) {
    // Object detected (glass is present)
    Serial.println("Object detected!");

    // LED logic
    digitalWrite(LED_PIN, pumpRunning);

    // Buzzer logic
    if (!buzzerActive) {
      Serial.println("Activating buzzer...");
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStartTime = millis();
    }

    if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Buzzer turned off after duration");
    }

    // Pump logic
    if (!pumpRunning) {
      Serial.println("Starting pump...");
      digitalWrite(RELAY_PIN, HIGH);
      pumpRunning = true;
      pumpStartTime = millis();
      digitalWrite(LED_PIN, HIGH);
    }

    if (pumpRunning && (millis() - pumpStartTime >= PUMP_DURATION)) {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Pump turned off after 10 seconds");
      pumpRunning = false;
      digitalWrite(LED_PIN, LOW);
    }
  } else {
    // No object detected
    Serial.println("No object detected");
    buzzerActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    pumpRunning = false;
  }

  delay(10);
}
