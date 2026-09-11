// TEST FILE -- flash temporarily to the SHED ESP32 to calibrate the wind
// vane in isolation before trusting it in shed_controller.ino.
//
// Rotate the vane by hand to each physical direction (at minimum: due
// South and due North, per the write-up's directional wind safety
// logic) and record the voltage shown at each position. Use these real
// recorded values to fill in vaneTable[] in shed_controller.ino --
// do not trust a generic published table.

const int WIND_VANE_PIN = 34; // must be an ADC1 pin (GPIO32-39) -- WiFi disables ADC2
const float PULLUP_RESISTOR = 10000.0; // ohms, matches the wired resistor
const float VCC = 3.3;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ESP32 ADC: 0-4095
  Serial.println("Wind vane test ready. Rotate through each direction and record the voltage.");
}

void loop() {
  int raw = analogRead(WIND_VANE_PIN);
  float voltage = raw * (VCC / 4095.0);
  float rVane = PULLUP_RESISTOR * voltage / (VCC - voltage);

  Serial.println("Raw: " + String(raw) +
                  " | Voltage: " + String(voltage, 3) + "V" +
                  " | R_vane: " + String(rVane, 0) + " ohm");
  delay(500);
}
