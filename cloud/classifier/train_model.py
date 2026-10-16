from pathlib import Path

import joblib
import numpy as np
import pandas as pd

from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.model_selection import StratifiedGroupKFold


ROOT_DIR = Path(__file__).resolve().parents[2]
DATASET_PATH = ROOT_DIR / "dataset" / "dataset.csv"
MODEL_PATH = Path(__file__).resolve().parent / "activity_model.joblib"

SENSOR_COLUMNS = [
    "acceleration_x",
    "acceleration_y",
    "acceleration_z",
    "gyro_x",
    "gyro_y",
    "gyro_z",
]

DOWNSAMPLE_FACTOR = 5
WINDOW_SIZE = 5


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


def build_windows(df):
    feature_rows = []
    labels = []
    groups = []

    # New segment whenever activity OR wrist changes
    segment_id = (
        df["activity"].ne(df["activity"].shift())
        | df["wrist"].ne(df["wrist"].shift())
    ).cumsum()

    for group_id, segment in df.groupby(segment_id):
        activity = int(segment["activity"].iloc[0])

        # Approximate 1 Hz from ~5 Hz source data
        segment = segment.iloc[::DOWNSAMPLE_FACTOR].reset_index(drop=True)

        for start in range(0, len(segment) - WINDOW_SIZE + 1, WINDOW_SIZE):
            window = segment.iloc[start:start + WINDOW_SIZE]

            feature_rows.append(extract_features(window))
            labels.append(activity)
            groups.append(group_id)

    return (
        pd.DataFrame(feature_rows),
        np.array(labels),
        np.array(groups),
    )


def create_model():
    return RandomForestClassifier(
        n_estimators=200,
        random_state=42,
        n_jobs=-1,
    )


def main():
    print("Loading dataset...")
    df = pd.read_csv(DATASET_PATH)

    print(f"Raw samples: {len(df)}")

    X, y, groups = build_windows(df)

    print(f"Temporal windows created: {len(X)}")
    print(f"Recording segments: {len(np.unique(groups))}")
    print()

    print("Class distribution:")
    unique, counts = np.unique(y, return_counts=True)

    for label, count in zip(unique, counts):
        name = "WALKING" if label == 0 else "RUNNING"
        print(f"  {label} ({name}): {count}")

    print()
    print("Running segment-aware 5-fold cross-validation...")
    print()

    cv = StratifiedGroupKFold(
        n_splits=5,
        shuffle=True,
        random_state=42,
    )

    fold_accuracies = []
    all_true = []
    all_pred = []

    for fold, (train_idx, test_idx) in enumerate(
        cv.split(X, y, groups),
        start=1,
    ):
        model = create_model()

        model.fit(X.iloc[train_idx], y[train_idx])

        predictions = model.predict(X.iloc[test_idx])

        accuracy = accuracy_score(
            y[test_idx],
            predictions,
        )

        fold_accuracies.append(accuracy)
        all_true.extend(y[test_idx])
        all_pred.extend(predictions)

        train_groups = len(np.unique(groups[train_idx]))
        test_groups = len(np.unique(groups[test_idx]))

        print(
            f"Fold {fold}: "
            f"{accuracy * 100:.2f}% "
            f"(train segments={train_groups}, "
            f"test segments={test_groups})"
        )

    print()
    print("========================================")
    print("CROSS-VALIDATION RESULTS")
    print("========================================")

    print(
        f"Mean accuracy: "
        f"{np.mean(fold_accuracies) * 100:.2f}%"
    )

    print(
        f"Std deviation: "
        f"{np.std(fold_accuracies) * 100:.2f}%"
    )

    print()

    print("Combined Classification Report:")
    print(
        classification_report(
            all_true,
            all_pred,
            target_names=["WALKING", "RUNNING"],
        )
    )

    print("Combined Confusion Matrix:")
    print(confusion_matrix(all_true, all_pred))

    print()
    print("Training final model on all available windows...")

    final_model = create_model()
    final_model.fit(X, y)

    model_package = {
        "model": final_model,
        "feature_names": list(X.columns),
        "window_size": WINDOW_SIZE,
        "sensor_columns": SENSOR_COLUMNS,
        "labels": {
            0: "WALKING",
            1: "RUNNING",
        },
        "downsample_factor": DOWNSAMPLE_FACTOR,
    }

    joblib.dump(model_package, MODEL_PATH)

    print()
    print("Final model saved to:")
    print(MODEL_PATH)


if __name__ == "__main__":
    main()