import sensor, image, time, gc

sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QQVGA)
sensor.set_vflip(False)
sensor.set_hmirror(False)
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

clock = time.clock()
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

        img.draw_line((best_line.x1, best_line.y1, best_line.x2, best_line.y2),
                      color=255, thickness=2)

        print("Angle: {:.2f} deg   FPS: {:.1f}".format(corrected_angle, clock.fps()))
    else:
        print("No line found")

    lines = None
    gc.collect()
    time.sleep_ms(50)
