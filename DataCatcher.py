# DataCatcher.py - Reads and logs IMU and EMG data from Arduino over serial to CSV
# This script opens a serial connection to an Arduino, reads CSV-formatted sensor lines
# at 115200 baud, and writes them to a timestamped output file.
#
# Each line from the Arduino is expected to contain 10 comma-separated fields:
# a timestamp (ms), three EMG channels, three accelerometer axes, and three gyroscope axes.
# Data is buffered in memory and flushed to disk in chunks to minimise I/O overhead.
# Recording stops cleanly on Ctrl+C, flushing any remaining buffered samples before exit.

import serial
import csv
import time
from datetime import datetime

# --- Config ---
COM_PORT = "COM7"
BAUD_RATE = 115200   # matches Arduino
# Timestamped filename so each session creates a unique file (no overwrites)
OUTPUT_FILE = f"data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

# --- Setup ---
# Open the serial port; timeout=1 means readline() waits at most 1 second per line
ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # Wait for Arduino to reset after serial connection is established
print(f"Connected to {COM_PORT}. Logging to {OUTPUT_FILE}")
print("Press Ctrl+C to stop.")

with open(OUTPUT_FILE, "w", newline="") as csvfile:
    writer = csv.writer(csvfile)

    # Write the CSV header row (must match the column order sent by the Arduino)
    writer.writerow([
        "timestamp_ms", "emg1", "emg2", "emg3",
        "accel_x", "accel_y", "accel_z",
        "gyro_x", "gyro_y", "gyro_z"
    ])

    # In-memory buffer to batch writes and reduce disk I/O overhead
    buffer = []
    FLUSH_INTERVAL = 200  # flush to disk every 200 samples

    try:
        while True:
            # Read one line from serial; decode bytes to string, ignoring bad bytes
            line = ser.readline().decode("utf-8", errors="ignore").strip()

            # Skip empty lines (e.g. timeouts or blank serial output)
            if not line:
                continue

            # Split the comma-separated values into a list of fields
            parts = line.split(",")

            # Discard any malformed lines that don't have exactly 10 fields
            if len(parts) != 10:
                continue

            buffer.append(parts)

            # Flush the buffer to disk once it reaches the threshold,
            # keeping memory usage low while minimising write calls
            if len(buffer) >= FLUSH_INTERVAL:
                writer.writerows(buffer)
                buffer.clear()

    except KeyboardInterrupt:
        print("\nStopping... saving remaining data.")
        # Write any samples still in the buffer that haven't been flushed yet
        if buffer:
            writer.writerows(buffer)

    finally:
        # Always close the serial port, even if an unexpected error occurred
        ser.close()
        print(f"Data saved to {OUTPUT_FILE}")