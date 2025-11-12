# Smart Bike Project group A

This repository contains the work for our **Smart Bike** project, part of the university IoT lab.  
The goal is to design a lightweight, low-power system that collects and transmits sensor data from a bicycle in real time — the first step toward a fully autonomous bike.

---

## Project Overview

The Smart Bike is equipped with several sensors to monitor movement, position, and environment data.  
All information is sent wirelessly to a gateway, which forwards it to a dashboard for live visualization.

We aim to:
- Collect data from multiple onboard sensors (speed, IMU, GPS, environment, …)
- Transmit it using **BLE 5 Long Range** to a **low-power gateway**
- Forward the processed data to a **dashboard** via **Wi-Fi**
- Keep everything lightweight, efficient, and modular

---

## System Architecture

**1. Bike (Sensor Node)**  
- Equipped with sensors for motion, speed, and environment  
- Communicates via **Bluetooth Low Energy (BLE 5 LR)**  

**2. Gateway**  
- Based on **ESP32-C3** microcontroller  
- Receives BLE data and sends it to the dashboard over Wi-Fi  
- Optimized for low power and simple integration  

**3. Dashboard**  
- Displays live bike data  
- Can be web-based or desktop-based  

---

## Group A Members

| Name               | Team                 | Role                   |
|--------------------|----------------------|------------------------|
| Beau Forrez        | Gateway & Dashboard  |                        |
| Elias Neels        | Gateway & Dashboard  |                        |
| Kjell Naessens     | Gateway & Dashboard  | Team Leader            |
| Wout Batselé       | Gateway & Dashboard  |                        |
| Charlotte Bert     | Sensors              | Time manager           |
| Niels Verhoeve     | Sensors              | Communication manager  |
| Arne Van der Stede | Sensors              |                        |
| Syme Vandenbosch   | Sensors              | Buget manager          |
| Daan Vercammen     | Sensors              | Team Leader            |
