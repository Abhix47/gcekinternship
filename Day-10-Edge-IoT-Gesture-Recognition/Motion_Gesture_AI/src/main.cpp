/*
 * DUAL-CORE MOTION DETECTION - 4 CLASSES (100Hz sampling)
 * Classes: idle_1 | leftright_1 | nothing_1 | updown_1
 * Core 0: Sampling at 100Hz (never stops)
 * Core 1: Reads completed buffer directly (no copy)
 */

#include<Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Motion_Gesture_inferencing.h>

Adafruit_MPU6050 mpu;

// ===== DOUBLE BUFFER SYSTEM =====
const int BUFFER_SIZE = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;  // 100 samples * 3 axes = 300

// Two complete buffers
float bufferA[BUFFER_SIZE];
float bufferB[BUFFER_SIZE];

// Pointers to current buffers
float* activeWriteBuffer = bufferA;    // Sampling writes here
float* activeReadBuffer = bufferB;     // Inference reads from here

int writeIndex = 0;
bool bufferReady = false;

// Mutex only for swapping pointers (very fast)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// ===== CALIBRATION =====
float offsetX = 0, offsetY = 0, offsetZ = 0;
bool isCalibrated = false;

// ===== SAMPLING PARAMETERS (100Hz = 10ms interval) =====
const int SAMPLE_INTERVAL_US = 10000;  // 10,000 microseconds = 10ms = 100Hz
const int VALUES_PER_SAMPLE = 3;       // X, Y, Z

// ===== LED PINS (4 LEDs for 4 classes) =====
const int LED_IDLE = 2;
const int LED_LEFTRIGHT = 4;
const int LED_NOTHING = 5;
const int LED_UPDOWN = 18;

// ===== TASK HANDLES =====
TaskHandle_t samplingTaskHandle = NULL;
TaskHandle_t inferenceTaskHandle = NULL;

// ===== FUNCTION DECLARATIONS =====
void samplingTask(void *pvParameters);
void inferenceTask(void *pvParameters);
void calibrateSensor();
bool initSensor();

// ===== HELPER FUNCTION: Get class name without "_1" suffix =====
String getDisplayName(String label) {
    label.replace("_1", "");
    return label;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Configuring Hardware I2C Pins...");
  Wire.begin(21, 22); 
  Wire.setClock(100000); // Start at standard 100kHz for reliable handshake
  delay(200);
  
  Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
  Serial.println("║      DUAL-CORE MOTION DETECTION - 4 CLASSES (100Hz)           ║");
  Serial.println("╠════════════════════════════════════════════════════════════════╣");
  Serial.println("║  Classes: idle | leftright | nothing | updown                  ║");
  Serial.println("║  Core 0: Sampling at 100Hz (writes to active buffer)          ║");
  Serial.println("║  Core 1: Reads completed buffer directly (NO COPY)            ║");
  Serial.println("╚════════════════════════════════════════════════════════════════╝\n");
  
  // Initialize all 4 LED pins
  pinMode(LED_IDLE, OUTPUT);
  pinMode(LED_LEFTRIGHT, OUTPUT);
  pinMode(LED_NOTHING, OUTPUT);
  pinMode(LED_UPDOWN, OUTPUT);
  
  // Turn all LEDs off initially
  digitalWrite(LED_IDLE, LOW);
  digitalWrite(LED_LEFTRIGHT, LOW);
  digitalWrite(LED_NOTHING, LOW);
  digitalWrite(LED_UPDOWN, LOW);
  
  // Initialize buffers
  memset(bufferA, 0, sizeof(bufferA));
  memset(bufferB, 0, sizeof(bufferB));
  initSensor();
  
  // Initialize sensor
  if (!initSensor()) {
    Serial.println("ERROR: MPU6050 not found!");
    while(1) delay(1000);
  }
 delay(2000);
  
  // Calibrate
  calibrateSensor();
  
  // Display model info from metadata
  Serial.println("\n📊 Model Information:");
  Serial.print("   Label count: ");
  Serial.println(EI_CLASSIFIER_LABEL_COUNT);
  Serial.print("   Buffer size (total values): ");
  Serial.println(BUFFER_SIZE);
  Serial.print("   Raw samples per window: ");
  Serial.println(EI_CLASSIFIER_RAW_SAMPLE_COUNT);
  Serial.print("   Window duration: ");
  Serial.print(EI_CLASSIFIER_RAW_SAMPLE_COUNT * EI_CLASSIFIER_INTERVAL_MS / 1000.0);
  Serial.println(" seconds");
  Serial.print("   Sample rate: ");
  Serial.print(EI_CLASSIFIER_FREQUENCY);
  Serial.println(" Hz");
  Serial.print("   Classes: ");
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print(ei_classifier_inferencing_categories[i]);
    if (i < EI_CLASSIFIER_LABEL_COUNT - 1) Serial.print(", ");
  }
  Serial.println("\n");
  
  Serial.println("✅ System ready! Starting dual-core tasks...\n");
  
  // Create sampling task on Core 0
  xTaskCreatePinnedToCore(
    samplingTask,
    "SamplingTask",
    4096,
    NULL,
    2,     // High priority for precise timing
    &samplingTaskHandle,
    0      // Core 0
  );
  
  // Create inference task on Core 1
  xTaskCreatePinnedToCore(
    inferenceTask,
    "InferenceTask",
    8192,
    NULL,
    1,     // Lower priority
    &inferenceTaskHandle,
    1      // Core 1
  );
  
  Serial.println("Both tasks running!\n");
}

// ===== REQUIRED BY ARDUINO =====
void loop() {
  delay(1000);  // Empty - all work in tasks
}

// ============================================================================
// TASK 1: SAMPLING ON CORE 0
// Continuously writes to activeWriteBuffer at 100Hz
// ============================================================================
void samplingTask(void *pvParameters) {
  
  unsigned long lastSampleTime = micros();
  
  while (1) {
    
    if (micros() - lastSampleTime >= SAMPLE_INTERVAL_US) {
      lastSampleTime = micros();
      
      // Read sensor
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      
      // Remove gravity
      float x = a.acceleration.x - offsetX;
      float y = a.acceleration.y - offsetY;
      float z = a.acceleration.z - offsetZ;
      
      // Write to active buffer (X, Y, Z sequentially)
      if (writeIndex < BUFFER_SIZE) {
        activeWriteBuffer[writeIndex] = x;
        writeIndex++;
        activeWriteBuffer[writeIndex] = y;
        writeIndex++;
        activeWriteBuffer[writeIndex] = z;
        writeIndex++;
      }
      
      // Check if buffer is full
      if (writeIndex >= BUFFER_SIZE && !bufferReady) {
        
        // CRITICAL SECTION: Swap buffers (very fast - just pointer swap)
        portENTER_CRITICAL(&mux);
        
        // Swap pointers
        float* tempPtr = activeWriteBuffer;
        activeWriteBuffer = activeReadBuffer;
        activeReadBuffer = tempPtr;
        
        bufferReady = true;
        writeIndex = 0;
        
        portEXIT_CRITICAL(&mux);
      }
    }
    
    delay(1);
  }
}

// ============================================================================
// TASK 2: INFERENCE ON CORE 1
// Reads activeReadBuffer directly (NO COPY, NO MUTEX during inference!)
// ============================================================================
void inferenceTask(void *pvParameters) {
  
  // Local reference to current buffer (updated after each swap)
  float* currentBuffer = NULL;
  bool hasBuffer = false;
  
  while (1) {
    
    // Check if a new buffer is ready
    if (bufferReady) {
      
      // Get the completed buffer (pointer only - NO COPY!)
      portENTER_CRITICAL(&mux);
      currentBuffer = activeReadBuffer;  // Just get pointer
      bufferReady = false;
      portEXIT_CRITICAL(&mux);
      
      hasBuffer = true;
    }
    
    // Run inference on the buffer (NO MUTEX - reading only!)
    if (hasBuffer && currentBuffer != NULL) {
      
      // Create signal from the buffer (direct access - no copy)
      signal_t signal;
      int err = numpy::signal_from_buffer(currentBuffer, BUFFER_SIZE, &signal);
      
      if (err == 0) {
        ei_impulse_result_t result;
        err = run_classifier(&signal, &result, false);
        
        if (err == 0) {
          
          // Store all scores and labels
          float scores[EI_CLASSIFIER_LABEL_COUNT];
          String labels[EI_CLASSIFIER_LABEL_COUNT];
          
          for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            labels[i] = String(result.classification[i].label);
            scores[i] = result.classification[i].value;
          }
          
          // Find the class with highest confidence
          int maxIndex = 0;
          for (int i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (scores[i] > scores[maxIndex]) {
              maxIndex = i;
            }
          }
          
          String prediction = labels[maxIndex];
          float confidence = scores[maxIndex];
          
          // ===== CONTROL LEDs BASED ON PREDICTION =====
          // Turn off all LEDs first
          digitalWrite(LED_IDLE, LOW);
          digitalWrite(LED_LEFTRIGHT, LOW);
          digitalWrite(LED_NOTHING, LOW);
          digitalWrite(LED_UPDOWN, LOW);
          
          // Turn on the LED for the predicted class
          if (prediction == "idle_1") {
            digitalWrite(LED_IDLE, LOW);
          } else if (prediction == "leftright_1") {
            digitalWrite(LED_LEFTRIGHT, HIGH);
          } else if (prediction == "nothing_1") {
            digitalWrite(LED_IDLE, LOW);
          } else if (prediction == "updown_1") {
            digitalWrite(LED_IDLE, HIGH);
          }
          
          // ===== PRINT RESULTS (with timestamp and all scores) =====
          unsigned long currentTime = millis();
          Serial.print("[");
          Serial.print(currentTime / 1000.0, 1);
          Serial.print(" sec] ");
          
          // Print all class scores
          for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            Serial.print(getDisplayName(labels[i]));
            Serial.print(": ");
            Serial.print(scores[i], 3);
            if (i < EI_CLASSIFIER_LABEL_COUNT - 1) Serial.print(" | ");
          }
          
          // Print prediction
          Serial.print(" -> ");
          Serial.print(getDisplayName(prediction));
          Serial.print(" (");
          Serial.print(confidence * 100, 1);
          Serial.println("%)");
        }
      }
      
      hasBuffer = false;
      currentBuffer = NULL;
    }
    
    delay(10);  // Prevent watchdog timeout
  }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool initSensor() {
  Serial.print("Connecting to MPU6050 via Adafruit API...");
  
  // Pass the already running Wire instance to the library
  if (!mpu.begin(0x68, &Wire)) {
    return false;
  }
  
  // If connection succeeds, safely upscale the bus frequency if desired
  Wire.setClock(400000); 
  
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);  // Good for 100Hz sampling
  
  Serial.println(" OK");
  return true;
}

void calibrateSensor() {
  Serial.print("Calibrating - Keep sensor STILL");
  
  int numSamples = 500;
  float sumX = 0, sumY = 0, sumZ = 0;
  
  for (int i = 0; i < numSamples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sumX += a.acceleration.x;
    sumY += a.acceleration.y;
    sumZ += a.acceleration.z;
    if (i % 100 == 0) Serial.print(".");
    delay(2);
  }
  
  offsetX = sumX / numSamples;
  offsetY = sumY / numSamples;
  offsetZ = sumZ / numSamples;
  
  Serial.println(" DONE!");
  Serial.print("Offsets: X=");
  Serial.print(offsetX, 2);
  Serial.print(", Y=");
  Serial.print(offsetY, 2);
  Serial.print(", Z=");
  Serial.println(offsetZ, 2);
  
  isCalibrated = true;
  delay(500);
}