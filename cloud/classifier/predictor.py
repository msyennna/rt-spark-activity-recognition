"""PyTorch inference preserving the six-field MQTT and Android API contract."""
import math
from numbers import Real

import torch

from cloud.classifier.temporal_model import (
    LABELS, SENSOR_COLUMNS, WINDOW_SIZE, load_model,
)

torch.set_num_threads(1)
model, MODEL_METADATA = load_model()


def predict_activity(sensor_readings):
    if len(sensor_readings) != WINDOW_SIZE:
        raise ValueError(f"Expected {WINDOW_SIZE} readings, received {len(sensor_readings)}.")
    rows = []
    for reading in sensor_readings:
        if not isinstance(reading, dict) or any(field not in reading for field in SENSOR_COLUMNS):
            raise ValueError("Missing sensor fields.")
        row = []
        for field in SENSOR_COLUMNS:
            value = reading[field]
            if isinstance(value, bool) or not isinstance(value, Real) or not math.isfinite(value):
                raise ValueError(f"Sensor field {field} must be a finite number.")
            row.append(float(value))
        rows.append(row)
    with torch.inference_mode():
        probabilities = model(torch.tensor([rows], dtype=torch.float32)).softmax(dim=1)[0]
        prediction = int(probabilities.argmax())
    return {
        "activity_id": prediction,
        "activity": LABELS[prediction],
        "confidence": float(probabilities[prediction]),
    }
