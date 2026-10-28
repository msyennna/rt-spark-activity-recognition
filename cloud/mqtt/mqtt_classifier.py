import json
import math
import time
import sys
from pathlib import Path

import paho.mqtt.client as mqtt


# Allow imports from the project root
ROOT_DIR = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT_DIR))

from cloud.classifier.predictor import (
    predict_activity,
    SENSOR_COLUMNS,
    WINDOW_SIZE,
    MODEL_METADATA,
)

from cloud.database.database import (
    initialize_database,
    save_prediction,
)


BROKER = "localhost"
PORT = 1883

SENSOR_TOPIC = "activity/sensor"
PREDICTION_TOPIC = "activity/prediction"

sensor_buffer = []
last_sensor_time = None


def on_connect(client, userdata, flags, reason_code, properties):
    global last_sensor_time
    sensor_buffer.clear()
    last_sensor_time = None
    if reason_code == 0:
        print("Connected to MQTT broker.")
        print(f"Listening on: {SENSOR_TOPIC}")
        print(f"Waiting for {WINDOW_SIZE} sensor readings...\n")

        client.subscribe(SENSOR_TOPIC)

    else:
        print(f"Connection failed: {reason_code}")


def on_message(client, userdata, message):
    global sensor_buffer, last_sensor_time

    try:
        payload = json.loads(
            message.payload.decode("utf-8")
        )

        missing = [
            field
            for field in SENSOR_COLUMNS
            if field not in payload
        ]

        if missing:
            sensor_buffer.clear()
            last_sensor_time = None
            print(f"Invalid message. Missing: {missing}")
            return

        if not isinstance(payload, dict):
            raise ValueError("Sensor payload must be a JSON object.")
        if any(isinstance(payload[field], bool) for field in SENSOR_COLUMNS):
            raise ValueError("Sensor fields must be numbers, not booleans.")

        reading = {
            field: float(payload[field])
            for field in SENSOR_COLUMNS
        }

        if not all(math.isfinite(value) for value in reading.values()):
            raise ValueError("Sensor readings must be finite.")
        now = time.monotonic()
        if last_sensor_time is not None and now - last_sensor_time > 2.5:
            sensor_buffer.clear()
            print("Sensor delivery gap; starting a new temporal window.")
        last_sensor_time = now
        sensor_buffer.append(reading)

        print(
            f"Sensor reading received "
            f"({len(sensor_buffer)}/{WINDOW_SIZE})"
        )

        if len(sensor_buffer) == WINDOW_SIZE:

            window = sensor_buffer[:WINDOW_SIZE]
            del sensor_buffer[:WINDOW_SIZE]
            result = predict_activity(window)

            record_id = save_prediction(
                result["activity_id"],
                result["activity"],
                result["confidence"],
            )

            print("\n==============================")
            print("ACTIVITY PREDICTION")
            print("==============================")
            print(f"Activity   : {result['activity']}")
            print(
                f"Confidence : "
                f"{result['confidence'] * 100:.2f}%"
            )
            print(f"Database ID: {record_id}")
            print("==============================\n")

            prediction_message = json.dumps({
                **result,
                "record_id": record_id,
            })

            client.publish(
                PREDICTION_TOPIC,
                prediction_message,
            )

            sensor_buffer = []

            print(
                f"Waiting for next "
                f"{WINDOW_SIZE} readings...\n"
            )

    except json.JSONDecodeError:
        sensor_buffer.clear()
        last_sensor_time = None
        print("Invalid JSON received.")

    except (TypeError, ValueError) as error:
        sensor_buffer.clear()
        last_sensor_time = None
        print(f"Invalid sensor data: {error}")

    except Exception as error:
        print(f"Error: {error}")



def main():
    initialize_database()

    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2
    )

    client.on_connect = on_connect
    client.on_message = on_message

    print("Starting MQTT activity classifier...", flush=True)
    print(f"PyTorch model SHA256: {MODEL_METADATA['model_sha256']}; "
          f"{MODEL_METADATA['trainable_parameters']} parameters", flush=True)

    client.connect(
        BROKER,
        PORT,
        keepalive=60,
    )

    client.loop_forever()


if __name__ == "__main__":
    main()
