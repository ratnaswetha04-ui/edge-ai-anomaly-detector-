
#include "model_data.h"

#include <tflm_esp32.h>
#include <eloquent_tinyml.h>

#define NUMBER_OF_INPUTS 4    
#define NUMBER_OF_OUTPUTS 4   
#define TENSOR_ARENA_SIZE 8*1024 

#define NUM_OPS 4
Eloquent::TF::Sequential<NUM_OPS, TENSOR_ARENA_SIZE> tf;
#define SOUND_PIN 34
#define SAMPLE_COUNT 20
#define SAMPLE_DELAY 50
float feature_min[4] = {0.0, 0.0, 0.0, 0.0};
float feature_max[4] = {97.65305390478412, 564.7684314267642, 16262.79957799886, 0.2021185548714905};

const float NORMALIZED_ERROR_THRESHOLD = 0.01;
int samples[SAMPLE_COUNT];
int windowCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("  EDGE AI ANOMALY DETECTION");
  Serial.println("  Running TensorFlow Lite on ESP32!");
  Serial.println("  (via EloquentTinyML)");

  tf.setNumInputs(NUMBER_OF_INPUTS);
  tf.setNumOutputs(NUMBER_OF_OUTPUTS);
  tf.resolver.AddFullyConnected();
  tf.resolver.AddRelu();
  tf.resolver.AddLogistic();  

  while (!tf.begin(autoencoder_model).isOk()) {
    Serial.println(tf.exception.toString());
    delay(1000);
  }

  Serial.println("Model loaded successfully!");
  Serial.println("\nListening for sounds...");
  Serial.println("Window# | Avg   | Peak | Var      | Error    | Status");
  Serial.println("--------|-------|------|----------|----------|--------");
}

void loop() {

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = analogRead(SOUND_PIN);
    delay(SAMPLE_DELAY);
  }

   float avg, peak, variance, minVal;
  calculateFeatures(avg, peak, variance, minVal);

  float input[4];
  input[0] = normalize(avg, feature_min[0], feature_max[0]);
  input[1] = normalize(peak, feature_min[1], feature_max[1]);
  input[2] = normalize(variance, feature_min[2], feature_max[2]);
  input[3] = normalize(minVal, feature_min[3], feature_max[3]);

  unsigned long startTime = micros();
  if (!tf.predict(input).isOk()) {
    Serial.println(tf.exception.toString());
    return;
  }
  unsigned long inferenceTime = micros() - startTime;

  float output[4];
  for (int i = 0; i < 4; i++) {
    output[i] = tf.output(i);
  }

   float error = 0;
  for (int i = 0; i < 4; i++) {
    float diff = input[i] - output[i];
    error += diff * diff;
  }
  error = error / 4.0;

  bool isAnomaly = (error > NORMALIZED_ERROR_THRESHOLD);

  Serial.printf("%7d | %5.1f | %4.0f | %8.0f | %8.5f | %s",
    windowCount, avg, peak, variance, error,
    isAnomaly ? "ANOMALY!" : "normal");

  Serial.printf("  (inference: %lu us)\n", inferenceTime);

  windowCount++;
}

void calculateFeatures(float &avg, float &peak, float &variance, float &minVal) {
  long sum = 0;
  peak = 0;
  minVal = 4095;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += samples[i];
    if (samples[i] > peak) peak = samples[i];
    if (samples[i] < minVal) minVal = samples[i];
  }

  avg = (float)sum / SAMPLE_COUNT;

  float varSum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float diff = samples[i] - avg;
    varSum += diff * diff;
  }
  variance = varSum / SAMPLE_COUNT;
  minVal = 0;  // we always saw 0 in our training data
}

float normalize(float value, float minV, float maxV) {
  if (maxV - minV == 0) return 0;
  float norm = (value - minV) / (maxV - minV);
  if (norm < 0) norm = 0;
  if (norm > 1) norm = 1;
  return norm;
}
