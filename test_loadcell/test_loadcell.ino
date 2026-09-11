// TEST FILE -- flash temporarily to the SHED ESP32 to verify the load
// cell/HX711 wiring in isolation, then calibrate it, before trusting it
// in shed_controller.ino.
//
// Requires the HX711 Arduino library (Tools -> Manage Libraries -> "HX711"
// by Bogdan Necula).
//
// STEP 1: Upload and watch "Raw reading" -- confirm it's stable near
//         zero at rest and changes clearly and repeatably when you
//         apply gentle hand pressure. (Do NOT stand on the load cell --
//         confirm its rated capacity first; overloading it can
//         permanently shift its zero point.)
// STEP 2: Type a known weight (in the same units you want to use for
//         MAX_TENSION, e.g. grams) into the Serial Monitor and press
//         Enter WHILE that exact weight is resting on/in-line with the
//         load cell. The sketch will print a calibration factor.
// STEP 3: Use that printed calibration factor with scale.set_scale(...)
//         and use the load cell's known safe limit (in the same units)
//         as MAX_TENSION, both inside shed_controller.ino.

#include "HX711.h"

const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("Taring... keep the load cell unloaded.");
  delay(2000);
  scale.tare();
  Serial.println("Tare complete.");
  Serial.println("Apply gentle hand pressure to test, or type a known weight and press Enter to calibrate.");
}

void loop() {
  if (Serial.available()) {
    float knownWeight = Serial.readStringUntil('\n').toFloat();
    if (knownWeight > 0) {
      float rawReading = scale.get_units(10);
      float calibrationFactor = rawReading / knownWeight;
      Serial.println("Raw reading with known weight on: " + String(rawReading));
      Serial.println("CALIBRATION FACTOR = " + String(calibrationFactor));
      Serial.println("Use this value in scale.set_scale(...) inside shed_controller.ino");
    }
  }

  if (scale.is_ready()) {
    Serial.println("Raw reading: " + String(scale.get_units(5)));
  } else {
    Serial.println("HX711 not found -- check wiring");
  }
  delay(500);
}
