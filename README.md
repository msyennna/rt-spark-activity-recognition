# RT-Spark Activity Recognition

A cloud-side activity-recognition prototype for classifying walking and running from accelerometer and gyroscope readings. It receives sensor messages over MQTT, predicts an activity with a trained scikit-learn model, stores predictions in SQLite, and exposes the latest prediction and history through a Flask API.

This repository contains the cloud components only. It does not currently include RT-Spark firmware, an Android/mobile app, or the training dataset.

## System flow

```text
+----------------------------+
| RT-Spark / Sensor Publisher|
+----------------------------+
              |
              | MQTT: activity/sensor
              v
+----------------------------+       +-------------------+       +-----------------+
|      MQTT Classifier       | ----> |  Activity Model   | ----> |     SQLite      |
+----------------------------+       +-------------------+       +-----------------+
              |                                                           ^
              | MQTT: activity/prediction                                 |
              v                                                           |
+----------------------------+                                   +-----------------+
| Mobile or other API Client | --------------------------------> |    Flask API    |
+----------------------------+                                   +-----------------+
```

The classifier expects six numeric fields per sensor message:

| Field | Sensor axis |
| --- | --- |
| `acceleration_x` | Accelerometer X |
| `acceleration_y` | Accelerometer Y |
| `acceleration_z` | Accelerometer Z |
| `gyro_x` | Gyroscope X |
| `gyro_y` | Gyroscope Y |
| `gyro_z` | Gyroscope Z |

The incoming sensor topic is `activity/sensor`; predictions are published to `activity/prediction`. The classifier buffers five readings before predicting. The device publisher and model must use compatible sampling rates, units, and axis conventions.

## Requirements

- Python with support for the versions pinned in [requirements.txt](requirements.txt)
- An MQTT broker reachable at `localhost:1883` (the current broker address is configured in the classifier)
- The included model at `cloud/classifier/activity_model.joblib`
- For retraining only: the Run or Walk dataset saved as `dataset/dataset.csv`

## Setup

From the project root, create and activate a virtual environment, then install dependencies:

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

Start an MQTT broker separately. For example, with Mosquitto installed and running with its default local listener, the classifier can connect to it at `localhost:1883`.

## Run the services

Start the MQTT classifier in one terminal:

```powershell
python -m cloud.mqtt.mqtt_classifier
```

Start the Flask API in another terminal:

```powershell
python -m cloud.api.app
```

The API listens on port `5000`. The current implementation uses Flask's built-in development server; do not expose it as a production deployment.

## Sensor message format

Publish one JSON object per reading to `activity/sensor`. Include all six sensor fields:

```json
{
  "acceleration_x": 0.12,
  "acceleration_y": -0.04,
  "acceleration_z": 9.78,
  "gyro_x": 0.01,
  "gyro_y": -0.02,
  "gyro_z": 0.03
}
```

The values above are illustrative only. Use the same measurement units, sensor orientation, and scaling as the data used to train the model.

## API

| Method | Endpoint | Description |
| --- | --- | --- |
| `GET` | `/` | Service name and status |
| `GET` | `/api/activity/latest` | Most recent stored prediction; returns `404` if there are no records |
| `GET` | `/api/activity/history` | Recent prediction records; accepts an optional `limit` query parameter (default `50`, bounded from `1` to `500`) |

Example requests:

```text
http://localhost:5000/
http://localhost:5000/api/activity/latest
http://localhost:5000/api/activity/history?limit=20
```

History responses contain a `count` and a `records` array. Each stored prediction includes its database ID, activity ID, activity name, confidence, and creation time. SQLite data is created automatically at `cloud/database/activity.db`.

## Retrain the model

Place the dataset at `dataset/dataset.csv`, with the sensor columns shown above and the `activity` and `wrist` columns used by the training pipeline. Then run:

```powershell
python -m cloud.classifier.train_model
```

The script downsamples the source data, builds five-reading windows, evaluates a random forest with stratified group five-fold cross-validation, and saves the final model to `cloud/classifier/activity_model.joblib`. It prints evaluation metrics to the terminal; those metrics are not stored in the repository.

## Project layout

```text
cloud/
  api/app.py                    Flask API
  classifier/
    activity_model.joblib       Serialized trained model
    predictor.py                Feature extraction and inference
    train_model.py              Dataset preparation and model training
  database/database.py          SQLite storage and queries
  mqtt/mqtt_classifier.py       MQTT ingestion and classification
requirements.txt                Python dependencies
```

## Scope and next steps

The accompanying lab brief describes a complete IoT system, including RT-Spark firmware and a mobile client. Those components are outside this repository's current contents. To complete the end-to-end activity, add a device publisher that sends compatible sensor readings and a mobile app that consumes the API. Before reporting model performance, run the training/evaluation workflow with the intended dataset and validate it with the target hardware.

For the detailed implementation status and evaluation notes, see [LABORATORY_REPORT.md](LABORATORY_REPORT.md). The assignment brief is [BCA180 ESP - Laboratory Activity 3.md](<BCA180 ESP - Laboratory Activity 3.md>).
