// UTILITY FILE -- flash this temporarily to EACH ESP32, one at a time,
// to read its permanent hardware MAC address. Run it on the pitch-pole
// ESP32 first and write down the address, then run it on the shed ESP32
// and write that one down too. You will paste the SHED's address into
// pitchpole_sender.ino before flashing the production code.
//
// After recording the address, re-flash the board with its real
// production file (pitchpole_sender.ino or shed_controller.ino).

#include <WiFi.h>
#include "esp_mac.h"

void setup() {
  Serial.begin(115200);
  delay(3000); // gives you time to open the Serial Monitor before setup() prints
  Serial.println("\n--- ESP32 Booted ---");

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  delay(100);

  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);

  Serial.printf("Hardware MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  Serial.println("Write this address down -- you'll need it in the other board's code.");
}

void loop() {}
