// TEST FILE -- flash temporarily to the SHED ESP32 to verify the wireless
// link from the pitch pole BEFORE trusting it with real camera data.
// Pair with test_esp_now_sender.ino on the pitch-pole ESP32.
//
// Expected result: this board's Serial Monitor prints "Received: 1, 2, 3..."
// incrementing steadily, roughly once per second.

#include <esp_now.h>
#include <WiFi.h>

typedef struct { int counter; } TestMessage;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  TestMessage msg;
  memcpy(&msg, incomingData, sizeof(msg));
  Serial.println("Received: " + String(msg.counter));
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println("My MAC: " + WiFi.macAddress());

  if (esp_now_init() != ESP_OK) { 
    Serial.println("ESP-NOW init failed"); 
    return; 
  }
  
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {}
