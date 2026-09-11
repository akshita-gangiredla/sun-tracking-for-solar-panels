// PRODUCTION FILE -- flash to the ESP32 mounted at the pitch pole (with the OpenMV Cam).
//
// BEFORE FLASHING:
//   1. Run esp32_mac_finder.ino on the SHED ESP32 and record its MAC address.
//   2. Paste that address into shedAddress[] below, replacing the 0xXX placeholders.
//
// This board reads the angle string from the OpenMV Cam over UART and
// relays it wirelessly to the shed ESP32 via ESP-NOW.

#include <esp_now.h>
#include <WiFi.h>
#include <HardwareSerial.h>

HardwareSerial camSerial(2);

// PASTE THE SHED ESP32's MAC ADDRESS HERE (from esp32_mac_finder.ino):
uint8_t shedAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

typedef struct {
  float angle;
  bool found;
} AngleMessage;

AngleMessage outgoing;

void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send FAILED");
}

void setup() {
  Serial.begin(115200);
  camSerial.begin(115200, SERIAL_8N1, 16, 17); // RX=GPIO16, TX=GPIO17 -- to OpenMV P4/P5
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

  Serial.println("Pitch-pole sender ready.");
}

void loop() {
  if (camSerial.available()) {
    String data = camSerial.readStringUntil('\n');
    if (data == "NOTFOUND") {
      outgoing.found = false;
      outgoing.angle = 0;
    } else {
      outgoing.found = true;
      outgoing.angle = data.toFloat();
    }
    esp_now_send(shedAddress, (uint8_t *)&outgoing, sizeof(outgoing));
  }
}
