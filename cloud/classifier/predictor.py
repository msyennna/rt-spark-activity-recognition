from pathlib import Path

import joblib
import numpy as np
import pandas as pd


MODEL_PATH = Path(__file__).resolve().parent / "activity_model.joblib"

model_package = joblib.load(MODEL_PATH)

model = model_package["model"]
FEATURE_NAMES = model_package["feature_names"]
SENSOR_COLUMNS = model_package["sensor_columns"]
WINDOW_SIZE = model_package["window_size"]
LABELS = model_package["labels"]


def extract_features(window):
    features = {}

    for column in SENSOR_COLUMNS:
        values = window[column].to_numpy(dtype=float)

        features[f"{column}_mean"] = np.mean(values)
        features[f"{column}_std"] = np.std(values)
        features[f"{column}_min"] = np.min(values)
        features[f"{column}_max"] = np.max(values)
        features[f"{column}_range"] = np.max(values) - np.min(values)
        features[f"{column}_rms"] = np.sqrt(np.mean(values ** 2))

    accel_mag = np.sqrt(
        window["acceleration_x"].to_numpy() ** 2
        + window["acceleration_y"].to_numpy() ** 2
        + window["acceleration_z"].to_numpy() ** 2
    )

    features["accel_mag_mean"] = np.mean(accel_mag)
    features["accel_mag_std"] = np.std(accel_mag)
    features["accel_mag_min"] = np.min(accel_mag)
    features["accel_mag_max"] = np.max(accel_mag)
    features["accel_mag_range"] = np.max(accel_mag) - np.min(accel_mag)
    features["accel_mag_rms"] = np.sqrt(np.mean(accel_mag ** 2))

    gyro_mag = np.sqrt(
        window["gyro_x"].to_numpy() ** 2
        + window["gyro_y"].to_numpy() ** 2
        + window["gyro_z"].to_numpy() ** 2
    )

    features["gyro_mag_mean"] = np.mean(gyro_mag)
    features["gyro_mag_std"] = np.std(gyro_mag)
    features["gyro_mag_min"] = np.min(gyro_mag)
    features["gyro_mag_max"] = np.max(gyro_mag)
    features["gyro_mag_range"] = np.max(gyro_mag) - np.min(gyro_mag)
    features["gyro_mag_rms"] = np.sqrt(np.mean(gyro_mag ** 2))

    return features


def predict_activity(sensor_readings):
    if len(sensor_readings) != WINDOW_SIZE:
        raise ValueError(
            f"Expected {WINDOW_SIZE} readings, "
            f"received {len(sensor_readings)}."
        )

    window = pd.DataFrame(sensor_readings)

    missing = [
        column
        for column in SENSOR_COLUMNS
        if column not in window.columns
    ]

    if missing:
        raise ValueError(f"Missing sensor fields: {missing}")

    features = extract_features(window)

    X = pd.DataFrame(
        [features],
        columns=FEATURE_NAMES,
    )

    prediction = int(model.predict(X)[0])

    probabilities = model.predict_proba(X)[0]

    return {
        "activity_id": prediction,
        "activity": LABELS[prediction],
        "confidence": float(np.max(probabilities)),
    }