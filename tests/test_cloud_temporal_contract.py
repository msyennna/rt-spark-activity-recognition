"""Actual PyTorch -> callback -> temporary SQLite -> Flask API contract."""
import contextlib
from io import StringIO
import importlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from cloud.database import database
from cloud.mqtt import mqtt_classifier
from cloud.classifier.temporal_model import SENSOR_COLUMNS


class Client:
    def __init__(self):
        self.sent = []

    def publish(self, topic, text):
        self.sent.append((topic, json.loads(text)))


class CloudContractTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.db = patch.object(database, "DATABASE_PATH", Path(self.temp.name)/"activity.db")
        self.db.start()
        self.output = contextlib.redirect_stdout(StringIO())
        self.output.__enter__()
        database.initialize_database()
        mqtt_classifier.sensor_buffer.clear()
        mqtt_classifier.last_sensor_time = None
        self.client = Client()

    def tearDown(self):
        self.output.__exit__(None, None, None)
        self.db.stop()
        self.temp.cleanup()

    def send(self, payload, at):
        message = type("Message", (), {"payload": json.dumps(payload).encode()})()
        with patch.object(mqtt_classifier.time, "monotonic", return_value=at):
            mqtt_classifier.on_message(self.client, None, message)

    def test_real_prediction_roundtrip_database_and_http(self):
        reading = {field: 0.1 for field in SENSOR_COLUMNS}
        reading["acceleration_z"] = 1.0
        for i in range(5):
            self.send(reading, float(i))
        row = database.get_latest_prediction()
        self.assertIsNotNone(row)
        self.assertIn(row["activity"], ("WALKING", "RUNNING"))
        self.assertTrue(0 <= row["confidence"] <= 1)
        self.assertEqual(len(self.client.sent), 1)
        topic, prediction = self.client.sent[0]
        self.assertEqual(topic, "activity/prediction")
        self.assertEqual(prediction["record_id"], row["id"])
        api = importlib.import_module("cloud.api.app").app.test_client()
        response = api.get("/api/activity/latest")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.get_json(), row)
        response = api.get("/api/activity/history?limit=20")
        self.assertEqual(response.get_json(), {"count": 1, "records": [row]})

    def test_invalid_data_and_delivery_gap_do_not_form_a_stale_window(self):
        reading = {field: 0.1 for field in SENSOR_COLUMNS}
        for i in range(4):
            self.send(reading, float(i))
        self.send(reading, 7.0)  # delivery gap resets the incomplete old window
        self.assertEqual(len(mqtt_classifier.sensor_buffer), 1)
        self.send({**reading, "gyro_x": float("nan")}, 8.0)
        self.assertIsNone(database.get_latest_prediction())
        for i in range(9, 14):
            self.send(reading, float(i))
        self.assertEqual(len(database.get_activity_history()), 1)

    def test_a_failed_prediction_does_not_wedge_all_later_windows(self):
        reading = {field: 0.1 for field in SENSOR_COLUMNS}
        with patch.object(mqtt_classifier, "predict_activity", side_effect=ValueError("test failure")):
            for i in range(5):
                self.send(reading, float(i))
        self.assertEqual(mqtt_classifier.sensor_buffer, [])
        for i in range(5, 10):
            self.send(reading, float(i))
        self.assertEqual(len(database.get_activity_history()), 1)


if __name__ == "__main__":
    unittest.main()
