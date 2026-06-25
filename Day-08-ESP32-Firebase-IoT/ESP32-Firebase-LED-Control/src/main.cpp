#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// 1. Enter Your Local Network Credentials Here
#define WIFI_SSID "Ritwin's S24"
#define WIFI_PASSWORD "123456788"

// 2. Direct JSON Endpoint URL (Notice the .json at the end)
const char* firebase_url = "https://ledcontrolproject-a827f-default-rtdb.firebaseio.com/led_state.json";

// 3. Hardware Pin Selection
const int LED_PIN = 2; // On-board LED

// Polling Timer Variables
unsigned long lastExecutionTime = 0;
const unsigned long pollingInterval = 1500; // Check every 1.5 seconds

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }
  Serial.println("\nWi-Fi connection established!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  initWiFi();
  Serial.println("Native HTTPS Polling initialized.");
}

void loop() {
  // Non-blocking timer
  if (millis() - lastExecutionTime >= pollingInterval) {
    lastExecutionTime = millis();

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      // Firebase uses SSL/TLS. Setting insecure skips root certificate validation 
      // making connection fast and leaving memory free.
      client.setInsecure(); 

      HTTPClient http;
      
      // Initialize connection to the explicit JSON node
      if (http.begin(client, firebase_url)) {
        int httpCode = http.GET(); // Execute standard HTTPS GET request

        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          payload.trim(); // Clean up any trailing whitespaces/newlines

          Serial.printf("Polled successfully! Raw Data: %s\n", payload.c_str());

          // Parse string response directly to drive the pin logic
          if (payload == "1") {
            digitalWrite(LED_PIN, HIGH);
          } else if (payload == "0") {
            digitalWrite(LED_PIN, LOW);
          }
        } else {
          Serial.printf("HTTPS GET Request failed. HTTP Error Code: %d\n", httpCode);
        }
        
        http.end(); // Close connection cleanly
      } else {
        Serial.println("Unable to connect to Firebase Host endpoint.");
      }
    } else {
      Serial.println("Wi-Fi disconnected. Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}