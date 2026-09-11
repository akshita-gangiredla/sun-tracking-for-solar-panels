// PRODUCTION FILE -- flash to the ESP32 in the Operations Shed.
//
// BEFORE FLASHING:
//   1. Replace "YOUR_WIFI_SSID" / "YOUR_WIFI_PASSWORD" below with the real shed WiFi credentials.
//   2. Replace vaneTable[] with the REAL calibrated voltages from test_windvane.ino.
//   3. Confirm MAX_TENSION and the wind thresholds with the mechanical/structural team
//      before relying on this in the field (see the write-up's Open Items section).
//
// This board receives the angle from the pitch-pole ESP32 over ESP-NOW,
// checks the tracking schedule, runs the pulse/check/repeat state machine,
// and layers in rain, wind, and load-cell safety interlocks on the relay.
//
// BUILT-IN SERIAL TEST COMMANDS (type into Serial Monitor, 115200 baud, line ending = Newline):
//   relay fwd       -> raw test: pulse FWD relay for 1s, no camera involved
//   relay rev       -> raw test: pulse REV relay for 1s, no camera involved
//   test <angle>    -> run the full pulse/check/repeat loop toward this target angle
//   stop            -> cancel and return to idle

#include <esp_now.h>
#include <WiFi.h>
#include <time.h>
#include "HX711.h"

// ============== ESP-NOW: angle from pitch-pole ESP32 ==============
typedef struct { 
  float angle; 
  bool found; 
} AngleMessage;

volatile AngleMessage latestReading;
volatile bool newDataAvailable = false;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy((void*)&latestReading, incomingData, sizeof(latestReading));
  newDataAvailable = true;
}

// ============== Schedule ==============
struct TrackingWindow { 
  int hour; 
  int minute; 
  float targetAngle; 
};

// modify this with what angle(third number for each time) we expect the panels to be at for that time of day
TrackingWindow schedule[] = { 
  {10, 0, 30.0}, 
  {14, 0, 0.0}, 
  {19, 0, -30.0} 
}; 

const float TOLERANCE_DEG = 2.0;

// ============== Rain gauge (GPIO27) ==============
const int RAIN_PIN = 27;
const float RAIN_PER_TIP = 0.011;
const unsigned long RAIN_DEBOUNCE_MS = 15;
const unsigned long RAIN_ACTIVE_WINDOW_MS = 5UL * 60UL * 1000UL;
volatile int rainTipCount = 0;
volatile unsigned long lastRainTipTime = 0;
void IRAM_ATTR onRainTip() {
  unsigned long now = millis();
  if (now - lastRainTipTime > RAIN_DEBOUNCE_MS) { 
    rainTipCount++; 
    lastRainTipTime = now; 
  }
}

bool isRainingNow() { 
  return (millis() - lastRainTipTime) < RAIN_ACTIVE_WINDOW_MS; 
}

// ============== Anemometer (GPIO14) ==============
const int ANEMOMETER_PIN = 14;
volatile unsigned long anemPulseCount = 0;
void IRAM_ATTR onAnemometerPulse() { 
  anemPulseCount++; 
}

const unsigned long WIND_SAMPLE_INTERVAL_MS = 2000;
unsigned long lastWindSampleTime = 0;
float currentWindMph = 0.0;
void updateWindSpeed() {
  if (millis() - lastWindSampleTime >= WIND_SAMPLE_INTERVAL_MS) {
    noInterrupts(); 
    unsigned long count = anemPulseCount; 
    anemPulseCount = 0; 
    interrupts();
    float hz = count / (WIND_SAMPLE_INTERVAL_MS / 1000.0);
    currentWindMph = hz * 1.492;
    lastWindSampleTime = millis();
  }
}

// ============== Wind vane (GPIO34, ADC1 only) ==============
const int WIND_VANE_PIN = 34;
const float VCC = 3.3;
struct VaneCalibration { float voltage; float docAngle; }; // South = 0, clockwise

// REPLACE with real values from test_windvane.ino {voltage, angle}:
VaneCalibration vaneTable[] = { {2.90, 0}, {1.10, 90}, {2.10, 180}, {3.00, 270} };


const int VANE_TABLE_SIZE = sizeof(vaneTable) / sizeof(vaneTable[0]);
float readWindVaneVoltage() { 
  return analogRead(WIND_VANE_PIN) * (VCC / 4095.0); 
}

float getWindDirDocConvention() {
  float v = readWindVaneVoltage(), best = 999, ang = 0;
  for (int i = 0; i < VANE_TABLE_SIZE; i++) {
    float d = abs(v - vaneTable[i].voltage);
    if (d < best) { best = d; ang = vaneTable[i].docAngle; }
  }
  return ang;
}

// ============== Directional wind safety ==============
const float WIND_DIRECTION_STOW_MPH = 30.0;   // PLACEHOLDER -- confirm real structural limit
const float WIND_DIRECTION_RESUME_MPH = 20.0;
const float DANGER_WINDOW_DEG = 45.0;
const float STOW_TILT_ANGLE = 5.0;

bool isDangerousWindDirection(float a) {
  float dS = min(a, 360.0f - a), dN = abs(a - 180.0f);
  return (dS <= DANGER_WINDOW_DEG) || (dN <= DANGER_WINDOW_DEG);
}

bool directionalWindStowActive = false;

void updateDirectionalWindSafety() {
  float dir = getWindDirDocConvention();
  bool dangerous = isDangerousWindDirection(dir);

  if (!directionalWindStowActive && dangerous && currentWindMph >= WIND_DIRECTION_STOW_MPH) {
    directionalWindStowActive = true;
    Serial.println("DANGEROUS WIND -> stow " + String(STOW_TILT_ANGLE));
  } else if (directionalWindStowActive && (!dangerous || currentWindMph <= WIND_DIRECTION_RESUME_MPH)) {
    directionalWindStowActive = false;
    Serial.println("Wind cleared -- resuming schedule");
  }
}

// ============== Load cell (HX711: DOUT=GPIO4, SCK=GPIO5) ==============
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
HX711 scale;
const float MAX_TENSION = 500.0; // PLACEHOLDER -- calibrate to real units and real limit
bool isTensionSafe() { 
  return scale.is_ready() ? (scale.get_units(3) < MAX_TENSION) : true; 
}

// ============== Relay / motor control (FWD=25, REV=26) ==============
const int FWD_PIN = 25, REV_PIN = 26;
const bool RELAY_ACTIVE_LOW = true; // verify empirically with "relay fwd"/"relay rev" commands
const int PULSE_MS = 1000, MAX_ITERATIONS = 10, SETTLE_MS = 500, DIRECTION_CHANGE_DELAY_MS = 500;
int lastDirection = 0;
void relayWrite(int pin, bool energize) {
  bool level = RELAY_ACTIVE_LOW ? !energize : energize;
  digitalWrite(pin, level ? HIGH : LOW);
}
void stopMotor() { 
  relayWrite(FWD_PIN, false); 
  relayWrite(REV_PIN, false); 
}

// ============== State machine ==============
enum State { IDLE, WAITING_FOR_READING, PULSING, SETTLING };
State state = IDLE;

int lastTriggeredHour = -1, lastTriggeredMinute = -1, iterationCount = 0;
float activeTargetAngle = NAN;
unsigned long stateStartTime = 0;
bool stowTriggeredThisEvent = false;

void startPulse(bool forward) {
  int newDir = forward ? 1 : -1;
  if (newDir != lastDirection && lastDirection != 0) { 
    stopMotor(); 
    delay(DIRECTION_CHANGE_DELAY_MS); 
  }
  relayWrite(forward ? FWD_PIN : REV_PIN, true);
  lastDirection = newDir; 
  stateStartTime = millis(); 
  state = PULSING;
  Serial.println(forward ? ">>> RELAY FWD ON <<<" : ">>> RELAY REV ON <<<");
}

void beginTrackingTo(float target) {
  activeTargetAngle = target; 
  iterationCount = 0; 
  newDataAvailable = false; 
  state = WAITING_FOR_READING;
}

void setup() {
  Serial.begin(115200);
  pinMode(FWD_PIN, OUTPUT); pinMode(REV_PIN, OUTPUT); stopMotor();

  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), onRainTip, FALLING);

  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), onAnemometerPulse, FALLING);

  analogReadResolution(12);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare();

  WiFi.mode(WIFI_STA);
  WiFi.begin("WIFI-USERNAME", "WIFI-PASSWORD"); // add shed's wifi username and password
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nConnected!");
  configTime(-8 * 3600, 3600, "pool.ntp.org", "time.nist.gov");

  if (esp_now_init() != ESP_OK) { 
    Serial.println("ESP-NOW init failed"); 
    return; 
  }
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Setup complete.");
  Serial.println("Commands: test <angle> | relay fwd | relay rev | stop");
}


// ============== Schedule Lookup ==============
float getTargetAngleNow(struct tm &t, int &oh, int &om) {
  for (auto &w : schedule) {
    if (t.tm_hour == w.hour && t.tm_min == w.minute) { 
      oh = w.hour; 
      om = w.minute; 
      return w.targetAngle; 
    }
  }
  return NAN;
}


// ============== Manual handling for testing ==============
void handleCommand(String cmd) {
  cmd.trim();
  if (cmd == "relay fwd") {
    relayWrite(FWD_PIN, true); delay(PULSE_MS); relayWrite(FWD_PIN, false);
    Serial.println("Raw FWD pulse done.");
  } else if (cmd == "relay rev") {
    relayWrite(REV_PIN, true); delay(PULSE_MS); relayWrite(REV_PIN, false);
    Serial.println("Raw REV pulse done.");
  } else if (cmd.startsWith("test ")) {
    float target = cmd.substring(5).toFloat();
    Serial.println("Manual test -- target = " + String(target));
    beginTrackingTo(target);
  } else if (cmd == "stop") {
    stopMotor(); 
    state = IDLE;
    Serial.println("Stopped.");
  } else {
    Serial.println("Unknown command.");
  }
}



// ============== Main loop ==============
void loop() {
  updateWindSpeed();
  updateDirectionalWindSafety();

  if (Serial.available()) handleCommand(Serial.readStringUntil('\n'));

  struct tm timeinfo;
  bool haveTime = getLocalTime(&timeinfo);

  switch (state) {
    case IDLE: {
      if (directionalWindStowActive && !stowTriggeredThisEvent) {
        Serial.println("Initiating stow due to dangerous wind.");
        stowTriggeredThisEvent = true;
        beginTrackingTo(STOW_TILT_ANGLE);
        break;
      }
      if (!directionalWindStowActive) stowTriggeredThisEvent = false;

      if (haveTime && !directionalWindStowActive) {
        int h, m;
        float target = getTargetAngleNow(timeinfo, h, m);
        bool already = (h == lastTriggeredHour && m == lastTriggeredMinute);
        if (!isnan(target) && !already) {
          Serial.println("Tracking window hit -- target = " + String(target));
          lastTriggeredHour = h; lastTriggeredMinute = m;
          beginTrackingTo(target);
        }
      }
      break;
    }
    case WAITING_FOR_READING: {
      if (newDataAvailable) {
        newDataAvailable = false;
        if (!latestReading.found) {
          Serial.println("Camera: no line found, aborting this cycle");
          state = IDLE; break;
        }
        float measured = latestReading.angle;
        float error = activeTargetAngle - measured;
        Serial.println("Measured: " + String(measured) + " | Target: " + String(activeTargetAngle));

        if (abs(error) <= TOLERANCE_DEG) {
          Serial.println("IN POSITION -- done.");
          stopMotor(); state = IDLE;
        } else if (iterationCount >= MAX_ITERATIONS) {
          Serial.println("Max iterations reached -- stopping (check for mechanical issue).");
          stopMotor(); state = IDLE;
        } else if (isRainingNow()) {
          Serial.println("BLOCKED: actively raining -- holding position.");
        } else if (!isTensionSafe()) {
          Serial.println("BLOCKED: cable tension too high -- holding position.");
        } else if (directionalWindStowActive && abs(activeTargetAngle - STOW_TILT_ANGLE) > 0.01) {
          Serial.println("Dangerous wind detected mid-cycle -- redirecting to stow.");
          beginTrackingTo(STOW_TILT_ANGLE);
        } else {
          iterationCount++;
          bool forward = error > 0;
          Serial.println("NEEDS ADJUSTMENT -- pulsing " + String(forward ? "FWD" : "REV") +
                          " (" + String(iterationCount) + "/" + String(MAX_ITERATIONS) + ")");
          startPulse(forward);
        }
      }
      break;
    }
    case PULSING: {
      if (millis() - stateStartTime >= PULSE_MS) {
        stopMotor();
        Serial.println(">>> RELAY OFF <<<");
        stateStartTime = millis(); 
        state = SETTLING;
      }
      break;
    }
    case SETTLING: {
      if (millis() - stateStartTime >= SETTLE_MS) {
        newDataAvailable = false; 
        state = WAITING_FOR_READING;
      }
      break;
    }
  }
}
