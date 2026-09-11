// TEST FILE -- flash temporarily to the SHED ESP32 to verify the rain
// gauge wiring in isolation before trusting it in shed_controller.ino.
// Manually tip the bucket mechanism by hand while watching Serial Monitor.
// Each physical tip should increment "Tips" by exactly 1.

const int RAIN_PIN = 27;
const float RAIN_PER_TIP = 0.011;
const unsigned long DEBOUNCE_MS = 100;

volatile int tipCount = 0;
volatile unsigned long lastTipTime = 0;

void IRAM_ATTR onRainTip() {
  unsigned long now = millis();
  if (now - lastTipTime > DEBOUNCE_MS) {
    tipCount++;
    lastTipTime = now;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), onRainTip, FALLING);
  Serial.println("Rain gauge test ready. Tip the bucket by hand to test.");
}

void loop() {
  noInterrupts();
  int count = tipCount;
  interrupts();

  float totalRain = count * RAIN_PER_TIP;
  bool rainingNow = (millis() - lastTipTime) < (5UL * 60UL * 1000UL);

  Serial.println("Tips: " + String(count) +
                  " | Total: " + String(totalRain, 3) + " in" +
                  " | Raining now: " + String(rainingNow));
  delay(1000);
}
