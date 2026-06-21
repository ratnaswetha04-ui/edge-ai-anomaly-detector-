# Edge AI Anomaly Detection Node

**Author:** K Ratna Swetha , 3rd Year ECE

## Overview

Most IoT projects send raw sensor data to the cloud and let a server decide what's "normal" or "abnormal." This project explores the opposite approach  **Edge AI**  where the decision-making model itself is small enough to run directly on the microcontroller, with no internet dependency for detection.

I built and trained a 118-parameter autoencoder that learns what "normal" ambient sound looks like, then flags anything that doesn't reconstruct well as a potential anomaly , all without ever seeing labeled "anomaly" examples during the unsupervised training step.

## 

I wanted to move beyond basic cloud-connected sensor projects and explore how machine learning models can run directly on resource-constrained microcontrollers (TinyML / Edge AI) — a space I hadn't worked in before, despite prior ESP32 experience.

**MY Task** was to Design, train, and attempt to deploy a lightweight anomaly-detection model on an ESP32, using real sensor data, with the full pipeline from data collection through on-device inference.

**This is what I did**
- Collected real-time acoustic data from a **KY-038 sound sensor** on ESP32, logging average/peak/variance/min features per 1-second window to ThingSpeak.
- Cleaned and augmented the dataset (97 real samples → 582 samples via controlled noise injection) to support more stable model training.
- Designed and trained a 4-8-2-8-4 autoencoder (118 total parameters) in **TensorFlow/Keras** on normal-only data, using reconstruction error as the anomaly signal.
- Converted the trained model to TensorFlow Lite format and quantised it to a 2.85 KB C array for embedded deployment.
- Attempted on-device deployment across three different ESP32 TFLite runtime libraries (TensorFlowLite_ESP32, EloquentTinyML + tflm_esp32, esp-tflite-micro), diagnosing and resolving multiple API and registration issues along the way.
- Hit a hard compiler/ABI incompatibility between the available precompiled TFLite runtime binaries and the current ESP32 Arduino toolchain (GCC 14.2.0) — a known, actively-discussed gap in the ESP32 TinyML ecosystem as of mid-2026.

**Result:** Successfully built and validated a working anomaly-detection model achieving 100% classification accuracy on held-out data, with a reconstruction error gap of roughly 3,500x between normal and anomalous samples — and a model small enough (2.85 KB, 118 params) to comfortably fit ESP32's memory constraints. On-device deployment was blocked by a toolchain version conflict rather than a modeling or design issue; the model, training pipeline, and deployment-ready artifacts are complete and documented for deployment once the runtime library ecosystem catches.

## The Deployment Blocker 

I attempted on-device deployment using three separate approaches:

1. **`TensorFlowLite_ESP32`** (tanakamasayuki) — hit a hard compile error in the bundled flatbuffers code (`assignment of read-only member`), caused by incompatibility between this library's 2022-era code and the modern GCC compiler shipped with current ESP32 board packages.
2. **`EloquentTinyML` + `tflm_esp32`** — got furthest with this approach. Fixed several real API issues (missing op registration, incorrect `predict()` signature) by reading the library source directly. Ultimately hit a **precompiled binary / toolchain ABI mismatch**: the library's precompiled runtime was built against an older GCC ABI than the one in my installed ESP32 board package (GCC 14.2.0).
3. **`esp-tflite-micro`** (Espressif's official library) — discovered this is published for ESP-IDF (Espressif's native build system) via the ESP Component Registry, not for Arduino IDE: a fundamentally different toolchain than what this project used.

This is a real, currently discussed pain point in the ESP32 TinyML community, not a modelling or design flaw. The model itself is fully trained, validated, and deployment-ready.

**In due course**, either (a) downgrade the ESP32 Arduino board package to match the library's expected GCC version, or (b) rebuild the project using ESP-IDF directly with Espressif's official `esp-tflite-micro` component, bypassing the Arduino layer entirely.

## Technical Stack

 Microcontroller - ESP32 Dev Module 
 Sensor - KY-038 Sound Sensor 
 ML Framework - TensorFlow / Keras (training), TensorFlow Lite (deployment format) 
 Cloud Platform - ThingSpeak (data logging during collection phase) 
 Training Environment - Google Colab 
 Attempted Runtimes - TensorFlowLite_ESP32, EloquentTinyML, esp-tflite-micro 

## Results

- **Normal data reconstruction error:** mean 0.0087, max 0.3768
- **Anomaly data reconstruction error:** mean 2425.48, min 30.83
- **Classification accuracy on held-out data:** 100% (444/444 normal, 138/138 anomaly correctly classified)

**Caveat:** This 100% accuracy reflects high-contrast acoustic anomalies (claps/knocks close to the sensor vs. ambient silence) in data augmented from a relatively small base sample (97 real readings). Real-world deployment with subtler anomaly classes, more diverse environments, or longer-term drift would need further data collection and validation before this accuracy figure could be expected to hold.

