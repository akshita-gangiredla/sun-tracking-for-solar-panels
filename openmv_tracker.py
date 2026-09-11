# PRODUCTION FILE -- flash to OpenMV Cam as main.py
# (File -> Save file to OpenMV Cam -> save as "main.py" so it auto-runs on power-up)
#
# Measures the Blume's tilt angle relative to horizontal and streams it
# over UART3 (pins P4=TX, P5=RX) once every 200ms, as either a signed
# angle string or the literal string "NOTFOUND".

import sensor, image, time, gc
from pyb import UART

sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QQVGA)
sensor.set_vflip(False)
sensor.set_hmirror(False)
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

uart = UART(3, 115200, timeout_char=1000)

clock = time.clock()

# CALIBRATION: point the camera at a reference edge that has been
# physically leveled with a real level/protractor, note the raw output,
# then set CAMERA_OFFSET so that a level object reads as 0.0
CAMERA_OFFSET = 0.0

while True:
    clock.tick()
    img = sensor.snapshot()
    lines = img.find_lines(threshold=2000, theta_margin=15, rho_margin=15)

    if lines:
        best_line = max(lines, key=lambda l: l.length)
        raw_theta = best_line.theta
        angle_from_horizontal = 90 - raw_theta
        corrected_angle = angle_from_horizontal - CAMERA_OFFSET
        uart.write("{:.2f}\n".format(corrected_angle))
    else:
        uart.write("NOTFOUND\n")

    lines = None
    gc.collect()
    time.sleep_ms(200)
