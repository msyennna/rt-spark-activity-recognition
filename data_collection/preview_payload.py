import json
import math
import time

import serial

PORT = "COM9"
BAUD_RATE = 115200

# These match the sensor driver's default ranges:
# Accelerometer: ±2 g
# Gyroscope: ±250 degrees/second
ACCEL_SCALE = 16384.0
GYRO_SCALE = 131.0

try:
    with serial.Serial(PORT, BAUD_RATE, timeout=1) as board:
        board.reset_input_buffer()
        next_output = time.monotonic()

        print("Previewing one converted reading per second.")
        print("Press Ctrl+C to stop.")

        while True:
            line = board.readline().decode(
                "utf-8", errors="ignore"
            ).strip()

            parts = line.split(",")

            if len(parts) != 6:
                continue

            try:
                ax, ay, az, gx, gy, gz = map(int, parts)
            except ValueError:
                continue

            now = time.monotonic()

            if now < next_output:
                continue

            next_output += 1.0

            # Resume normally after a long interruption.
            if next_output <= now:
                next_output = now + 1.0

            payload = {
                "acceleration_x": round(ax / ACCEL_SCALE, 6),
                "acceleration_y": round(ay / ACCEL_SCALE, 6),
                "acceleration_z": round(az / ACCEL_SCALE, 6),
                "gyro_x": round(math.radians(gx / GYRO_SCALE), 6),
                "gyro_y": round(math.radians(gy / GYRO_SCALE), 6),
                "gyro_z": round(math.radians(gz / GYRO_SCALE), 6),
            }

            print(json.dumps(payload))

except KeyboardInterrupt:
    print("\nPreview stopped.")

except serial.SerialException as error:
    raise SystemExit(f"Serial connection error: {error}")