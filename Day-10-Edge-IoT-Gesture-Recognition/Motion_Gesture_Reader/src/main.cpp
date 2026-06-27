/*
 * MPU6050 3-Axis Accelerometer Reader
 * - Sample rate: 100 Hz
 * - Output: timestamp(ms), X, Y, Z
 * - Removes gravity offset from each axis independently
 */
#include<Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// ========== OFFSET VALUES (will be set during calibration) ==========
float offsetX = 0;
float offsetY = 0;
float offsetZ = 0;

bool isCalibrated = false;

// ========== TIMING FOR 100Hz ==========
unsigned long lastSampleTime = 0;
const long SAMPLE_INTERVAL_US = 1000000 / 100;  // 10,000 microseconds for 100Hz

unsigned long startTime = 0;

void calibrateOffsets() {
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
    //delay(2);
  }
  
  // Calculate average offset (this IS the gravity vector)
  offsetX = sumX / numSamples;
  offsetY = sumY / numSamples;
  offsetZ = sumZ / numSamples;
  
  Serial.println("\nCalibration complete!");
  Serial.print("Offsets - X: ");
  Serial.print(offsetX, 2);
  Serial.print(", Y: ");
  Serial.print(offsetY, 2);
  Serial.print(", Z: ");
  Serial.println(offsetZ, 2);
  Serial.println("(These values will be subtracted from all readings)\n");
  
  isCalibrated = true;
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println("\n========================================");
  Serial.println("  MPU6050 - Raw X,Y,Z with Gravity Removed");
  Serial.println("  Sample Rate: 100Hz");
  Serial.println("  Output: timestamp,X,Y,Z");
  Serial.println("========================================\n");
  
  // Initialize I2C
  Wire.begin(21, 22);
  Wire.setClock(400000);
  mpu.begin(0x68, &Wire);
  
  // Initialize MPU6050
/*
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("ERROR: MPU6050 not found!");
    while (1) {
    delay(1000);
    }
  }*/
  
  // Configure sensor
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);  // Good for 100Hz (44Hz filter bandwidth is sufficient)
  
  Serial.println("MPU6050 connected!");
  
  // Calibrate - find the offset values
  calibrateOffsets();
  
  startTime = millis();
  lastSampleTime = micros();
  
  // Print CSV header
  Serial.println("timestamp_ms,X,Y,Z");
  
  Serial.println("\nReady! Sampling at 100Hz...");
  Serial.println("========================================\n");
}

/*
 * CALIBRATION - Measure sensor when STILL
 * The values we read ARE the gravity offset
 * We'll subtract these from all future readings
 */

void loop() {
  if (!isCalibrated) {
    delay(100);
    return;
  }
  
  unsigned long now = micros();
  
  // Check if it's time to sample (100Hz = every 10,000 microseconds)
  if (now - lastSampleTime >= SAMPLE_INTERVAL_US) {
    lastSampleTime = now;
    
    // Read sensor
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    // Subtract offset from EACH axis individually
    float x = a.acceleration.x - offsetX;
    float y = a.acceleration.y - offsetY;
    float z = a.acceleration.z - offsetZ;
    
    // Calculate timestamp
    unsigned long timestamp = millis() - startTime;
    
    // Print: timestamp, X, Y, Z
    Serial.print(timestamp);
    Serial.print(",");
    Serial.print(x, 3);
    Serial.print(",");
    Serial.print(y, 3);
    Serial.print(",");
    Serial.println(z, 3);
  }
}