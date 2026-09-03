# EdgeAI_Based_Solar_Panel_Health_Monitoring_System

# Edge AI-Based Solar Panel Health Monitoring System

An end-to-end Edge AI + IoT project that monitors a solar panel's health in real time using a multi-sensor ESP32 setup, an on-device TinyML classifier, and a live Node-RED dashboard.

Built during my internship at **RDL Technologies Pvt Ltd**.

> For the complete project report, methodology, dataset details, and evaluation results, refer to `Solar_Panel_Internship_Report_Ashlesh_Y_Saliyan.pdf` in this repository.

---

## Overview

Conventional solar monitoring systems typically track only voltage and current, using simple threshold checks to flag underperformance. This approach can't distinguish *why* output has dropped — dust accumulation, partial shading, overheating, and electrical faults can all look similar from voltage/current alone.

This project addresses that gap with a low-cost Edge AI system that classifies the panel's operating condition directly on an ESP32 microcontroller, using a TinyML model trained and deployed via Edge Impulse.

## System Architecture

```
Solar Panel
   |
Sensors (Voltage, Current, Temperature, Light)
   |
ESP32
  |-- Read sensor values
  |-- Calculate power
  |-- Run TinyML model (on-device inference)
   |
Prediction (Healthy / Dust / Partial Shade / No Sunlight / Overheating / Fault)
   |
MQTT (HiveMQ Cloud, TLS-secured)
   |
Node-RED Dashboard
```

## Features

- **Multi-sensor data acquisition**: voltage (resistive divider), current (estimated), panel temperature (DS18B20), and ambient light (LDR)
- **On-device TinyML inference**: 6-class classifier (Healthy, Dust Accumulation, Partial Shading, No Sunlight, Overheating, Fault) running directly on the ESP32
- **Cloud connectivity**: live sensor data and AI predictions published via MQTT over a TLS-secured connection
- **Real-time dashboard**: Node-RED dashboard with Live Parameters, AI Health Prediction, and System Status views, plus a live multi-parameter chart

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main microcontroller — sensor reading, AI inference, WiFi/MQTT |
| Voltage Sensor Module (0–25V) | Scales panel voltage to a safe 0–3.3V ADC range |
| DS18B20 | Digital temperature sensor mounted on the panel's rear surface |
| LDR | Measures ambient light intensity, independent of panel output |
| Small solar panel (test unit) | Monitored device |

## Software & Tools

| Tool | Role |
|---|---|
| Arduino IDE | Firmware development for the ESP32 |
| Edge Impulse Studio | Dataset management, model training, Arduino library export |
| HiveMQ Cloud | TLS-secured MQTT broker |
| Node-RED + node-red-dashboard | Real-time web dashboard |
| PubSubClient / WiFiClientSecure | MQTT communication over TLS |

## Machine Learning Pipeline

- **Data collection**: ~100–150 samples per class, logged at 1 sample/second, physically simulating each condition (covering the panel, warming the temperature sensor, etc.)
- **Dataset**: 688 rows collected → 598 unique samples after deduplication, split 80/20 train/test
- **Model**: Flatten processing block + Classification (Neural Network) with two dense hidden layers (20, 10 neurons), 6-class output, trained for 500 cycles
- **Performance**: 93.8% validation accuracy, 89.17% accuracy on held-out test data, AUC of 0.99
  - Fault, No Sunlight, Overheating, and Partial Shade classified with 100% accuracy
  - Dust and Healthy (more overlapping signatures) achieved 73.9% and 70.8% respectively
- **Deployment**: exported as an Arduino library (Float32, EON Compiler enabled) — ~3ms inference time, 1.4KB peak RAM, 13.9KB flash usage

## Dashboard

The Node-RED dashboard subscribes to the MQTT telemetry topic and provides:
- **Live Parameters** view — voltage, current, power, temperature, light intensity
- **AI Health Prediction** view — predicted condition with confidence score and derived power-generation status
- **System Status** view — ESP32/WiFi/MQTT connectivity status
- A continuously updating live chart of all sensor values

## Repository Contents

- `Solar_Panel_Internship_Report_Ashlesh_Y_Saliyan.pdf` — full project report (architecture, methodology, ML pipeline details, results, and source code appendix)
- Firmware source (Arduino sketch)
- Node-RED flow (function node source code)

## Future Scope

- Replace estimated (voltage-derived) current with an independently calibrated current sensor
- Collect additional Healthy/Dust samples under varied lighting conditions to improve classification accuracy
- Extend the dashboard with historical data logging and automated fault-alert notifications

## Acknowledgements

Built during an internship at **RDL Technologies Pvt Ltd**, under the guidance of Ms. Sharanya.
