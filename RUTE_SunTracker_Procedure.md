OpenMV + ESP32 Solar Tracking Prototype --- Wiring, Flashing, and
Testing

Prepared by: Akshita Gangiredla

Status: Tested prototype, individual subsystems validated

# Overview {#overview .unnumbered}

This document is a step-by-step procedure for reproducing the RUTE
SunTracker prototype at a field site. It covers physical wiring,
flashing each microcontroller with the correct file from the
accompanying code package, and testing each piece before relying on it.

The system uses a camera at the pitch pole to measure the Blume\'s tilt
angle, and relays that reading wirelessly to a controller in the
Operations Shed. The shed controller compares the measured angle against
a schedule (10am / 2pm / 7pm) and drives a relay-controlled winch to
correct the angle in 1-second pulses, rechecking after each pulse until
the Blume is within tolerance. Rain, wind, and cable-tension sensors add
safety interlocks on top of this core loop: they can pause a move or
override the schedule entirely to drive the Blume to a safe stow
position.

Three physical zones are involved:

-   Pitch Pole --- OpenMV Cam + one ESP32 (\"the sender\")

-   Operations Shed --- one ESP32 (\"the controller\"), relay module,
    and all weather/tension sensors

-   Winch / Rotary Drive --- connected to the relay\'s output, not
    directly to any microcontroller

# 1. Code Package Contents {#code-package-contents .unnumbered}

This document refers to files by name. All files below are included
alongside this document.

## Production Files (flash these for permanent field operation)

## Requires software Arduino IDE (.ino files) and OpenMV IDE (.py files).

  ------------------------------------------------------------------------
  **Filename**             **Goes On**        **Purpose**
  ------------------------ ------------------ ----------------------------
  openmv_tracker.py        OpenMV Cam         Measures Blume tilt angle,
                                              streams it over UART

  pitchpole_sender.ino     Pitch-pole ESP32   Relays the camera\'s angle
                                              to the shed over ESP-NOW

  shed_controller.ino      Shed ESP32         Full tracking logic + relay
                                              control + all sensor
                                              interlocks
  ------------------------------------------------------------------------

## 1.2 Utility File {#utility-file .unnumbered}

  ------------------------------------------------------------------------
  **Filename**             **Goes On**        **Purpose**
  ------------------------ ------------------ ----------------------------
  esp32_mac_finder.ino     Both ESP32s        Reads out the board\'s
                           (temporarily, one  permanent hardware MAC
                           at a time)         address

  ------------------------------------------------------------------------

##  {#section .unnumbered}

##  {#section-1 .unnumbered}

##  {#section-2 .unnumbered}

## 1.3 Test Files (flash temporarily, one at a time, during setup only) {#test-files-flash-temporarily-one-at-a-time-during-setup-only .unnumbered}

  -------------------------------------------------------------------------
  **Filename**                **Goes On**       **Tests**
  --------------------------- ----------------- ---------------------------
  test_esp_now_sender.ino     Pitch-pole ESP32  Wireless link to the shed
                                                (pairs with the receiver
                                                test file)

  test_esp_now_receiver.ino   Shed ESP32        Wireless link from the
                                                pitch pole

  test_rain_gauge.ino         Shed ESP32        Rain gauge tip counting

  test_anemometer.ino         Shed ESP32        Wind speed reading

  test_windvane.ino           Shed ESP32        Wind direction voltage ---
                                                also used to calibrate

  test_loadcell.ino           Shed ESP32        Load cell wiring --- also
                                                used to calibrate
  -------------------------------------------------------------------------

Relay wiring and the full tracking loop are tested using commands built
into shed_controller.ino itself (see Section 7) --- no separate test
file is needed for those.

# 2. Powering the Boards {#powering-the-boards .unnumbered}

## 2.1 Operations Shed {#operations-shed .unnumbered}

Power the shed ESP32 from a regular outlet via a standard 5V USB supply.

## 2.2 Pitch Pole (Camera + Sender ESP32) {#pitch-pole-camera-sender-esp32 .unnumbered}

There is no AC power at the pitch pole per the site diagram, and this
location needs to run unattended 24/7. The recommended approach is a
small sealed 12V battery (SLA or LiFePO4) with a small PV trickle
charger --- the same concept already used for the shed\'s battery bank,
just scaled down for two low-power boards --- feeding a buck converter
stepped down to 5V for both the OpenMV Cam and the pitch-pole ESP32.

-   This avoids manual battery swaps/recharging trips that a plain USB
    power bank would require.

-   Confirm the OpenMV Cam\'s accepted input voltage/current before
    wiring it to the buck converter\'s output --- verify against the
    board\'s documentation rather than assuming.

-   Both boards and the battery/charger should be in a weatherproof
    enclosure at the pitch pole.

# 3. Pitch-Pole Wiring Procedure {#pitch-pole-wiring-procedure .unnumbered}

## 3.1 Mount the OpenMV Cam {#mount-the-openmv-cam .unnumbered}

1.  Mount the OpenMV Cam H7 R2 so it has a clear, stable view of the
    side profile of the Blume it will track.

2.  Confirm the mount is rigid --- the camera measures angle relative to
    its own frame, so any camera movement between readings will corrupt
    the measurement.

![](./media/image1.png){width="3.216409667541557in"
height="2.5786384514435694in"}

## 3.2 Wire the OpenMV Cam to the Pitch-Pole ESP32 {#wire-the-openmv-cam-to-the-pitch-pole-esp32 .unnumbered}

Both boards need soldered pin headers for reusable jumper-wire
connections --- the OpenMV\'s through-holes do not ship with headers
installed.

  -----------------------------------------------------------------------
  **OpenMV Pin**          **Signal**              **Connects to ESP32
                                                  Pin**
  ----------------------- ----------------------- -----------------------
  P4                      UART3 TX                GPIO16 (RX2)

  P5                      UART3 RX                GPIO17 (TX2)

  GND                     Ground                  GND
  -----------------------------------------------------------------------

TX connects to RX, and RX connects to TX (crossed) --- this is the most
common wiring mistake. Confirm grounds are tied together or the link
will be unreliable.

## ![](./media/image2.jpeg){width="3.2140048118985125in" height="3.4252701224846893in"} {#section-3 .unnumbered}

##  {#section-4 .unnumbered}

## 3.3 Power Both Pitch-Pole Boards {#power-both-pitch-pole-boards .unnumbered}

Connect the OpenMV Cam and the pitch-pole ESP32 to the pitch-pole power
supply described in Section 2.2.

# 4. Finding the ESP32 MAC Addresses {#finding-the-esp32-mac-addresses .unnumbered}

Before flashing the production sender code, you need the shed ESP32\'s
permanent hardware MAC address, since the pitch-pole ESP32 must be told
exactly which board to send its data to.

1.  Connect the SHED ESP32 to your laptop and flash
    **esp32_mac_finder.ino** to it.

2.  Open its Serial Monitor (115200 baud) and record the printed
    \"Hardware MAC Address" and write it down exactly.

3.  Repeat on the PITCH-POLE ESP32 with the same
    **esp32_mac_finder.ino** file and record its address too (kept for
    your records --- not currently needed by any other file, but useful
    if the system later needs to send data back to the pitch pole).

4.  You will paste the SHED\'s MAC address into **pitchpole_sender.ino**
    in the next section.

Some ESP32 core versions report an unreliable MAC via WiFi.macAddress()
immediately after boot. esp32_mac_finder.ino reads the address directly
from the chip\'s hardware fuses instead, which is more reliable --- use
its printed value, not one read any other way.

# 5. Flash the Pitch-Pole Sender {#flash-the-pitch-pole-sender .unnumbered}

1.  Open **pitchpole_sender.ino** in Arduino IDE.

2.  Edit the **shedAddress\[\]** array near the top of the file,
    replacing the placeholder bytes with the shed ESP32\'s MAC address
    from Section 4.

3.  Select the pitch-pole ESP32\'s COM port under Tools -\> Port.

4.  Click Upload.

5.  Leave this board running --- it will now automatically read from the
    OpenMV Cam and transmit over ESP-NOW every time it powers on.

# 6. Operations Shed Wiring Procedure {#operations-shed-wiring-procedure .unnumbered}

Complete the following wiring before flashing the shed controller code.
A full pin reference table and diagram is in Section 6.6 for quick
lookup.

## 6.1 Relay Module {#relay-module .unnumbered}

  -----------------------------------------------------------------------
  **ESP32 Pin**                       **Connects To**
  ----------------------------------- -----------------------------------
  GPIO25                              Relay module IN1 (FWD)

  GPIO26                              Relay module IN2 (REV)

  5V / VIN                            Relay module VCC

  GND                                 Relay module GND
  -----------------------------------------------------------------------

Most relay modules are active-LOW (a LOW signal turns the relay ON).
This will be verified empirically in Section 7 --- do not assume either
way before testing. The relays provided in the ESP32 starter kit were
used for testing and were active low.

##  {#section-5 .unnumbered}

## ![](./media/image3.jpeg){width="3.3348665791776027in" height="4.601094706911636in"} {#section-6 .unnumbered}

##  {#section-7 .unnumbered}

##  {#section-8 .unnumbered}

##  {#section-9 .unnumbered}

##  {#section-10 .unnumbered}

## 6.2 Rain Gauge {#rain-gauge .unnumbered}

Connect the RJ11 connector cable to the RJ11 connector and confirm the
correct pair of RJ11 active pins with a multimeter in continuity mode by
identifying which 2 of the 6 pins show a closed circuit when you
manually tip the bucket mechanism. The one linked in Section 8.1 and
used by me will be Pins 3 & 4.

  -----------------------------------------------------------------------
  **Rain Gauge (confirmed pin)**      **ESP32 Pin**
  ----------------------------------- -----------------------------------
  Active pin 1                        GPIO27

  Active pin 2                        GND
  -----------------------------------------------------------------------

##  {#section-11 .unnumbered}

## ![](./media/image4.jpeg){width="5.13242782152231in" height="4.824333989501312in"} {#section-12 .unnumbered}

##  {#section-13 .unnumbered}

##  {#section-14 .unnumbered}

## 6.3 Anemometer and Wind Vane {#anemometer-and-wind-vane .unnumbered}

Confirm each sensor\'s correct pin pair with a multimeter (continuity
mode for the anemometer\'s switch (same and rain gauge), resistance mode
for the wind vane) before wiring. For the wind vane, test across
different pin pairs while manually rotating the vane through several
positions. You\'re looking for the pair where **resistance changes as
you rotate** --- record several actual resistance readings at different
known positions (e.g., point it N, E, S, W) as you go. With the specific
connectors used for testing, the middle pins (3 & 4) related to the
anemometer and the ones outside that (2 & 5) related to the anemometer.
Refer to the diagram in Section 6.6 for more information.

  -----------------------------------------------------------------------
  **Sensor**              **ESP32 Pin**           **Notes**
  ----------------------- ----------------------- -----------------------
  Anemometer              GPIO14                  Switch closure ---
                                                  needs INPUT_PULLUP, no
                                                  external resistor

  Wind Vane signal        GPIO34                  Must be an ADC1 pin
                                                  (GPIO32-39) --- ADC2 is
                                                  unreliable whenever
                                                  WiFi is active, and
                                                  this system runs WiFi
                                                  continuously
  -----------------------------------------------------------------------

Wind vane voltage divider (look at Section 6.6 for clearer diagram):

-   ESP32 3.3V -\> 10k ohm resistor -\> node (this node also connects to
    GPIO34) -\> Wind Vane -\> ESP32 GND

##  {#section-15 .unnumbered}

## ![](./media/image5.jpeg){width="6.160862860892388in" height="4.452830271216098in"} {#section-16 .unnumbered}

##  {#section-17 .unnumbered}

##  {#section-18 .unnumbered}

## 6.4 Load Cell {#load-cell .unnumbered}

Solder the load cell\'s wires onto the HX711 board\'s J1 header:

  -----------------------------------------------------------------------
  **Load Cell Wire**                  **HX711 Pin (J1)**
  ----------------------------------- -----------------------------------
  EXC+ (red)                          E+

  EXC- (black)                        E- (twist together with shield,
                                      solder as one joint)

  SIG+ (green)                        A+

  SIG- (white)                        A-

  Shield (bare)                       E- (see above)
  -----------------------------------------------------------------------

Solder a 4-pin header onto the HX711\'s output side, then wire to the
ESP32:

  -----------------------------------------------------------------------
  **HX711 Pin**                       **ESP32 Pin**
  ----------------------------------- -----------------------------------
  VCC                                 3.3V

  GND                                 GND

  DT                                  GPIO4

  SCK                                 GPIO5
  -----------------------------------------------------------------------

##  {#section-19 .unnumbered}

## ![](./media/image6.jpeg){width="3.5901290463692037in" height="2.2075470253718286in"} {#section-20 .unnumbered}

##  {#section-21 .unnumbered}

## ![](./media/image7.jpeg){width="3.0314490376202974in" height="4.688056649168854in"} {#section-22 .unnumbered}

## 6.5 Relay Output to the Winch {#relay-output-to-the-winch .unnumbered}

This is the connection that actually moves the Blume. The relay\'s
control side (IN1/IN2, wired in Section 6.1) is driven by the ESP32. The
relay\'s OUTPUT side (the COM/NO contacts) is what switches power to the
physical winch and is completely separate from the ESP32\'s logic
wiring.

-   Relay 1 (FWD channel) output contacts -\> in series with Winch A\'s
    power leads

-   Relay 2 (REV channel) output contacts -\> in series with Winch B\'s
    power leads

In the production design, confirm winch voltage/current ratings against
the relay/contactor\'s rated capacity, before energizing anything. Can
also switch around the relays connecting to the winches if they appear
to be moving backwards.

-   Confirm which physical winch (FWD vs. REV) actually corresponds to
    which real-world rotation direction BEFORE trusting the automatic
    correction logic --- test manually first (Section 7.6).

## 6.6 Full Shed ESP32 Pin Reference {#full-shed-esp32-pin-reference .unnumbered}

  -----------------------------------------------------------------------
  **ESP32 Pin**           **Connected To**        **Function**
  ----------------------- ----------------------- -----------------------
  GPIO25                  Relay IN1               FWD winch trigger

  GPIO26                  Relay IN2               REV winch trigger

  GPIO27                  Rain gauge              Tip interrupt

  GPIO14                  Anemometer              Speed pulse interrupt

  GPIO34                  Wind vane signal node   Direction voltage read
                                                  (ADC1 only)

  GPIO4                   HX711 DOUT              Load cell data

  GPIO5                   HX711 SCK               Load cell clock

  3.3V / VIN              HX711 VCC, relay VCC    Power

  GND                     All sensor grounds,     Common ground ---
                          relay GND               required for every
                                                  device
  -----------------------------------------------------------------------

Video for how my prototype was connected:

![](./media/image8.jpeg){width="7.16795384951881in"
height="3.962264873140857in"}

# 7. Testing Procedure {#testing-procedure .unnumbered}

Test every subsystem in isolation, in this order, before combining them.
Do not skip ahead to full-system testing if an earlier step doesn\'t
behave as expected. To test each subsystem listed here, connect the
relevant pins to the Shed/receiver ESP32 and follow the instructions.

## 7.1 OpenMV Camera Angle Measurement {#openmv-camera-angle-measurement .unnumbered}

1.  Flash **openmv_tracker.py** to the OpenMV Cam as main.py (Tools -\>
    Save file to OpenMV Cam).

2.  With OpenMV IDE\'s serial terminal open, physically level the
    reference edge/line with a real level or protractor while the camera
    is mounted.

3.  Note the reported angle, then set CAMERA_OFFSET in the file so a
    level object reads 0.0. Re-flash.

4.  Rotate to 2-3 other known angles (protractor-verified) and confirm
    the corrected output tracks correctly in both magnitude and sign.

## 7.2 ESP-NOW Wireless Link {#esp-now-wireless-link .unnumbered}

1.  Flash **test_esp_now_receiver.ino** to the SHED ESP32.

2.  Flash **test_esp_now_sender.ino** to the PITCH-POLE ESP32 (with the
    shed\'s MAC address already filled in).

3.  Confirm the shed\'s Serial Monitor shows \"Received: 1, 2, 3\...\"
    incrementing steadily, about once per second.

## 7.3 Rain Gauge {#rain-gauge-1 .unnumbered}

1.  Flash **test_rain_gauge.ino** to the shed ESP32.

2.  Manually tip the bucket mechanism and confirm the Tips count
    increases by exactly 1 per physical tip (not 0, not multiple).

## 7.4 Anemometer {#anemometer .unnumbered}

1.  Flash **test_anemometer.ino** to the shed ESP32.

2.  Spin the cups by hand and confirm the reported wind speed responds
    sensibly.

## 7.5 Wind Vane (also serves as calibration) {#wind-vane-also-serves-as-calibration .unnumbered}

1.  Flash **test_windvane.ino** to the shed ESP32.

2.  Rotate the vane by hand to each direction (at minimum due South and
    due North) and record the voltage shown at each --- this builds the
    real calibration table.

3.  Transfer these recorded voltages into the vaneTable\[\] array inside
    shed_controller.ino before flashing it.

## 7.6 Relay Wiring (using shed_controller.ino\'s built-in commands) {#relay-wiring-using-shed_controller.inos-built-in-commands .unnumbered}

1.  Flash the production **shed_controller.ino** (Fill in WiFi
    credentials and vane table).

2.  Open its Serial Monitor, set line ending to \"Newline\".

3.  Type \"relay fwd\" and confirm the correct relay clicks and the
    correct winch/direction moves. Repeat with \"relay rev\".

4.  If the wrong relay channel activates for a given command, correct
    the wiring or swap FWD_PIN/REV_PIN in the code --- do not proceed
    until this is verified, since a reversed mapping will make the
    automatic correction logic drive the Blume the wrong way.

## 7.7 Load Cell (also serves as calibration) {#load-cell-also-serves-as-calibration .unnumbered}

1.  Flash **test_loadcell.ino** to the shed ESP32.

2.  Confirm a stable near-zero reading at rest, and a clear, repeatable
    change with gentle hand pressure (confirm the load cell\'s rated
    capacity before applying any real test load).

3.  Apply a known weight in-line with the load cell (matching its real
    tension-loading direction) and record the calibration factor it
    prints.

4.  Update MAX_TENSION in shed_controller.ino using this calibration.

## 7.8 Full Tracking Loop (manual target) {#full-tracking-loop-manual-target .unnumbered}

1.  With the production **shed_controller.ino** running and *everything*
    connected, type \"test \<angle\>\" (e.g. \"test 20\") into its
    Serial Monitor.

2.  Rotate the reference object in front of the OpenMV Cam toward that
    angle and confirm the relay pulses, pauses, rechecks, and eventually
    reports IN POSITION.

## 7.9 Weather and Tension Interlocks {#weather-and-tension-interlocks .unnumbered}

-   Rain: tip the rain gauge mid-test and confirm movement pauses,
    resuming roughly 5 minutes after the last tip.

-   Wind speed alone (safe direction): spin the anemometer fast while
    the vane points away from S/N --- confirm stow does NOT trigger.

-   Wind direction alone (low speed): point the vane at S or N without
    spinning the anemometer fast --- confirm stow does NOT trigger.

-   Wind speed AND direction together: confirm stow DOES trigger, and
    that it interrupts an in-progress move rather than waiting for it to
    finish.

-   Tension: apply pressure to the load cell mid-test and confirm
    movement pauses.

## 7.10 Full End-to-End Test {#full-end-to-end-test .unnumbered}

1.  With no manual overrides active, let the system run through an
    actual scheduled time (10am, 2pm, or 7pm) unattended.

2.  Confirm the complete behavior matches what was validated in
    isolation: correct schedule trigger, correct angle convergence, and
    correct interlock behavior if weather or tension conditions occur
    during the window.

## 7.11 Summary of Bench Testing Already Completed {#summary-of-bench-testing-already-completed .unnumbered}

The table below records what has already been confirmed working on the
bench

  -----------------------------------------------------------------------
  **Subsystem**        **Result**       **Notes**
  -------------------- ---------------- ---------------------------------
  OpenMV angle         Confirmed        Stable, repeatable angle output
  measurement          working          after tuning find_lines()
                                        parameters and confirming the
                                        correct line-object attribute
                                        names for this firmware version

  OpenMV -\>           Confirmed        Verified with a simple counter
  pitch-pole ESP32     working          message before trusting real
  (UART)                                angle data

  Pitch-pole ESP32 -\> Confirmed        Verified with a simple counter
  shed ESP32 (ESP-NOW) working          message; required reading each
                                        board\'s MAC via esp_read_mac()
                                        rather than WiFi.macAddress()

  NTP time sync        Confirmed        Verified independently before
                       working          combining with tracking logic

  Full tracking loop   Confirmed        Reliably reaches and reports IN
  (manual target       working          POSITION using the \"test
  angle)                                \<angle\>\" command

  Relay FWD/REV        Confirmed        Verified relay clicks correctly
  pulsing              working          via the \"relay fwd\"/\"relay
                                        rev\" commands; active-low
                                        behavior confirmed on the
                                        prototype\'s 2-channel module

  Rain gauge           Confirmed        Correct pins identified via
                       working          multimeter continuity test; tip
                                        counting confirmed accurate by
                                        hand

  Anemometer           Confirmed        Wind speed reading responds
                       working          correctly to manual spinning

  Wind vane wiring     Confirmed        Voltage divider circuit verified;
                       working          full direction calibration table
                                        still needs to be populated with
                                        real recorded values (Section
                                        7.5)

  Wind speed +         Confirmed        Verified that stow only triggers
  direction stow       working          when both conditions are met
  interlock            (simulated)      together, and correctly
                                        interrupts an in-progress move

  Rain interlock       Confirmed        Verified movement pauses during
                       working          simulated rain and resumes after
                                        the quiet window elapses

  Load cell wiring     Confirmed        Initial flat-zero readings traced
                       working          to a solder joint issue and
                                        resolved; calibration with a
                                        known weight still pending
  -----------------------------------------------------------------------

# 8. Bill of Materials {#bill-of-materials .unnumbered}

## 8.1 Prototype Components {#prototype-components .unnumbered}

  ----------------------------------------------------------------------------------
  **Component**                         **Qty**   **Notes**
  ------------------------------------- --------- ----------------------------------
  OpenMV Cam H7 R2 -- provided          1         No onboard WiFi --- communicates
                                                  via UART3 (pins P4/P5)

  ESP32 dev board -- provided           2         One at pitch pole (sender), one at
                                                  shed (controller)

  5V 2-channel relay module -- from     1         Drives FWD/REV winch power
  starter kit provided                            

  [HX711 load cell                      1         Required --- load cell signal is
  amplifier](https://a.co/d/08XOMK44)             too small to read directly

  Load cell (4-wire + shield) --        1         EXC+/EXC-/SIG+/SIG- per its wiring
  provided                                        diagram

  Rain gauge (reed switch, tipping      1         0.011 in / tip; RJ11 terminated
  bucket) -- provided                             

  Anemometer (reed switch) -- provided  1         1 closure/sec = 1.492 mph; shares
                                                  a cable with the wind vane

  Wind vane (resistive) -- provided     1         Reports direction as a voltage via
                                                  internal resistor network

  [RJ11 breakout board                  2         For the rain gauge and the
  (6P6C)](https://a.co/d/02PpfwL9)                combined anemometer/wind vane
                                                  cable

  10k ohm resistor -- from starter kit  1         Pull-up for the wind vane voltage
  provided                                        divider

  12V battery + PV trickle charger      1 set     Recommended power source --- see
  (pitch pole)                                    Section 2.2

  12V-to-5V buck converter              2         One at pitch pole, one at shed if
                                                  not using AC/USB directly
  ----------------------------------------------------------------------------------

#  {#section-23 .unnumbered}

# 9. Open Items --- Confirm Before Field Deployment {#open-items-confirm-before-field-deployment .unnumbered}

These are placeholder values or unresolved questions. Confirm each with
the mechanical/structural team before running unattended.

  -----------------------------------------------------------------------
  **Item**                    **Current              **What\'s Needed**
                              Placeholder**          
  --------------------------- ---------------------- --------------------
  Directional wind stow       30 mph trigger / 20    Real structural
  threshold                   mph resume             wind-load limit for
                                                     this rig

  Stow tilt angle             5 degrees              Confirm this is
                                                     genuinely the
                                                     lowest-load tilt for
                                                     the array geometry

  Danger wind window          +/-45 deg from due S   Confirm this matches
                              or due N               the panel\'s actual
                                                     wide-face
                                                     orientation

  Max cable tension           500 (uncalibrated      Real safe tension
                              units)                 limit for the
                                                     control cable, in
                                                     calibrated units

  Relay active-high vs.       Assumed active-low     Confirm empirically
  active-low                                         per Section 7.6

  FWD/REV to physical         Assumed, not           Must be confirmed
  direction mapping           field-verified         per Section 7.6
                                                     before trusting
                                                     auto-correction

  Winch controller vs. direct Assumed relay -\>      Confirm whether a
  motor wiring                contactor coil -\>     separate \"winch
                              motor                  controller\" unit
                                                     sits in this chain
                                                     per the BOM
  -----------------------------------------------------------------------
