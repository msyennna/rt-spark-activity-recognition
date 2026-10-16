import csv
import time
from datetime import datetime
from pathlib import Path

import serial

PORT = "COM9"
BAUD_RATE = 115200
LABEL = "walking"  # Change this to "running" for a running recording

output_file = Path(__file__).with_name(f"{LABEL}_{datetime.now():%Y%m%d_%H%M%S}.csv")

print(f"Connecting to {PORT}...")
with serial.Serial(PORT, BAUD_RATE, timeout=2) as board, \
        output_file.open("w", newline="") as csv_file:

    writer = csv.writer(csv_file)
    writer.writerow(["time", "ax", "ay", "az", "gx", "gy", "gz", "label"])
    print(f"Recording {LABEL} data to {output_file.name}. Press Ctrl+C to stop.")

    try:
        while True:
            line = board.readline().decode("utf-8", errors="ignore").strip()
            values = line.split(",")

            if len(values) == 6:
                try:
                    numbers = [int(value) for value in values]
                except ValueError:
                    continue

                writer.writerow([time.time(), *numbers, LABEL])
                csv_file.flush()
                print(line)

    except KeyboardInterrupt:
        print(f"\nRecording stopped. Saved to {output_file}")