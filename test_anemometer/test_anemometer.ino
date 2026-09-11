// TEST FILE -- flash temporarily to the SHED ESP32 to verify the
// anemometer wiring in isolation before trusting it in shed_controller.ino.
// Spin the cups by hand and confirm the reported mph responds sensibly.

const int ANEMOMETER_PIN = 14;
volatile unsigned long pulseCount = 0;

void IRAM_ATTR onAnemometerPulse() {
  pulseCount++;
}

unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(115200);
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), onAnemometerPulse, FALLING);
  Serial.println("Anemometer test ready. Spin the cups by hand.");
}

void loop() {
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();

    float hz = count / (SAMPLE_INTERVAL_MS / 1000.0);
    float windMph = hz * 1.492; // 1 closure/sec = 1.492 mph, per kit spec
    Serial.println("Pulses: " + String(count) + " | Wind speed: " + String(windMph, 2) + " mph");
    lastSampleTime = millis();
  }
}
