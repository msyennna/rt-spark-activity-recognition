import json
import math
import time
from getpass import getpass
from threading import Event

import paho.mqtt.client as mqtt
import serial

SERIAL_PORT = "COM9"
BAUD_RATE = 115200
BROKER = "52.62.47.179"
MQTT_PORT = 1883
MQTT_USERNAME = "yena_sensor"
TOPIC = "activity/sensor"

connection_reply = Event()


def on_connect(client, userdata, flags, reason_code, properties):
    print(f"MQTT connection: {reason_code}")
    connection_reply.set()


client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    reconnect_on_failure=False,
)
client.on_connect = on_connect

try:
    password = getpass("MQTT password: ")
    client.username_pw_set(MQTT_USERNAME, password)
    client.connect(BROKER, MQTT_PORT, keepalive=60)
    client.loop_start()

    if not connection_reply.wait(10) or not client.is_connected():
        raise RuntimeError("MQTT connection was not accepted.")

    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as board:
        board.reset_input_buffer()
        next_output = time.monotonic()
        print("Forwarding sensor readings once per second. Ctrl+C to stop.")

        while True:
            line = board.readline().decode("utf-8", errors="ignore").strip()
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

            if not client.is_connected():
                raise RuntimeError("MQTT disconnected. Rerun the publisher.")

            # Default ranges: +/-2 g and +/-250 degrees/second.
            payload = {
                "acceleration_x": round(ax / 16384.0, 6),
                "acceleration_y": round(ay / 16384.0, 6),
                "acceleration_z": round(az / 16384.0, 6),
                "gyro_x": round(math.radians(gx / 131.0), 6),
                "gyro_y": round(math.radians(gy / 131.0), 6),
                "gyro_z": round(math.radians(gz / 131.0), 6),
            }

            text = json.dumps(payload)
            message = client.publish(TOPIC, text, qos=1, retain=False)

            if message.rc != mqtt.MQTT_ERR_SUCCESS:
                raise RuntimeError("MQTT could not queue the reading.")

            message.wait_for_publish(timeout=5)
            if not message.is_published():
                raise RuntimeError("Broker acknowledgement timed out.")

            print("Sent:", text)

            next_output += 1.0
            if next_output <= time.monotonic():
                next_output = time.monotonic() + 1.0

except KeyboardInterrupt:
    print("\nPublisher stopped.")
except Exception as error:
    raise SystemExit(f"Publisher stopped: {error}")
finally:
    client.disconnect()
    client.loop_stop()