#include <WiFi.h>
#include "ThingSpeak.h"

const char* WIFI_SSID      = "WIFI_NAME";
const char* WIFI_PASSWORD  = "WIFI_PASSWORD";
unsigned long CHANNEL_ID   = 0;
const char*  WRITE_API_KEY = "API_KEY";

#define SOUND_PIN 34
#define SAMPLE_COUNT 20
#define SAMPLE_DELAY 50

unsigned long lastCloudUpdate = 0;
const long CLOUD_INTERVAL = 15000;

int samples[SAMPLE_COUNT];
float avgValue = 0;
int peakValue = 0;
int minValue = 4095;
float variance = 0;
int windowCount = 0;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Edge AI Project — Sound Data Collection ===");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed — running offline");
  }

  ThingSpeak.begin(client);

  Serial.println("Window# | Avg   | Peak | Min  | Variance");
  Serial.println("--------|-------|------|------|----------");
}

void loop() {
  collectSamples();
  calculateStats();
  printToSerial();

  unsigned long now = millis();
  if (now - lastCloudUpdate >= CLOUD_INTERVAL) {
    lastCloudUpdate = now;
    sendToThingSpeak();
  }

  windowCount++;
}

void collectSamples() {
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = analogRead(SOUND_PIN);
    delay(SAMPLE_DELAY);
  }
}

void calculateStats() {
  long sum = 0;
  peakValue = 0;
  minValue = 4095;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += samples[i];
    if (samples[i] > peakValue) peakValue = samples[i];
    if (samples[i] < minValue) minValue = samples[i];
  }

  avgValue = (float)sum / SAMPLE_COUNT;

  float varSum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float diff = samples[i] - avgValue;
    varSum += diff * diff;
  }
  variance = varSum / SAMPLE_COUNT;
}

void printToSerial() {
  Serial.printf("%7d | %5.1f | %4d | %4d | %8.1f\n",
    windowCount, avgValue, peakValue, minValue, variance);
}

void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;

  ThingSpeak.setField(1, (int)avgValue);
  ThingSpeak.setField(2, peakValue);
  ThingSpeak.setField(3, (int)variance);
  ThingSpeak.setField(4, minValue);
  ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
}
