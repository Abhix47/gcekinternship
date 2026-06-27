# Day 10 - Edge IoT Gesture Recognition

## Objective

Understand the implementation of an Edge AI application for real-time motion gesture recognition using an ESP32, MPU6050 accelerometer, and an Edge Impulse machine learning model.

---

## Topics Covered

- Introduction to Edge IoT
- Motion Gesture Recognition
- MPU6050 Accelerometer
- I2C Communication
- Sensor Data Collection
- Edge Impulse Model Integration
- Dual-Core Processing on ESP32
- Real-Time Machine Learning Inference
- Firebase Hosting
- Motion Gesture Web Application

---

## Projects

### 1. Motion_Gesture_Reader

Reads acceleration data from the MPU6050 sensor at a sampling rate of **100 Hz**. The collected sensor data is used for gesture recording and machine learning model development.

### 2. Motion_Gesture_AI

Implements a real-time gesture recognition system using an Edge Impulse machine learning model. The ESP32 classifies motion into predefined gesture classes and controls LEDs based on the prediction.

Supported gesture classes:

- Idle
- Left Right
- Up Down
- Nothing

---

## Assignment

The assignment document provided during the internship is included in the **Assignment** folder.

---

## Live Demo

https://motion-gesture-a7c24.web.app/

---

## Hardware Used

- ESP32 Development Board
- MPU6050 Accelerometer
- LEDs
- Jumper Wires

---

## Software Used

- PlatformIO
- Arduino Framework
- Edge Impulse
- Firebase Hosting
- HTML
- CSS
- JavaScript

---

## Learning Outcome

Learned how Edge AI can be implemented on an ESP32 by collecting accelerometer data, integrating an Edge Impulse machine learning model, and performing real-time motion gesture recognition. Also gained an understanding of deploying IoT applications with a web interface using Firebase Hosting.