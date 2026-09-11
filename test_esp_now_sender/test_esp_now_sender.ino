// TEST FILE -- flash temporarily to the PITCH-POLE ESP32 to verify the
// wireless link to the shed BEFORE trusting it with real camera data.
// Pair with test_esp_now_receiver.ino on the shed ESP32.
//
// Paste the shed ESP32's MAC address (from esp32_mac_finder.ino) below.

#include <esp_now.h>
#include <WiFi.h>

uint8_t shedAddress[] = {0xE0, 0x8C, 0xFE, 0xE6, 0x41, 0x08}; // use the Hardware Fused MAC from the shed board

typedef struct { 
  int counter; 
} TestMessage;

TestMessage msg;

void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send FAILED");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) { 
    Serial.println("ESP-NOW init failed");
    return; 
  }
  
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, shedAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  msg.counter = 0;
}

void loop() {
  msg.counter++;
  esp_now_send(shedAddress, (uint8_t *)&msg, sizeof(msg));
  Serial.println("Sending: " + String(msg.counter));
  delay(1000);
}
