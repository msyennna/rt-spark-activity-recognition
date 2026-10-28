# Cloud services

The cloud pipeline receives motion readings through Mosquitto, runs a lightweight
PyTorch temporal classifier, stores predictions in SQLite and serves the Android
app through a Flask API.

| Path | Responsibility |
| --- | --- |
| [classifier/](classifier/README.md) | Trained GRU, metadata, inference, dataset preparation and training |
| [mqtt/mqtt_classifier.py](mqtt/mqtt_classifier.py) | Sensor subscription, five-reading buffering and prediction publication |
| [database/database.py](database/database.py) | SQLite initialization, inserts and latest/history queries |
| [api/app.py](api/app.py) | JSON REST API |

## Run locally

From the repository root, activate a Python environment and install
`requirements.txt`. Start Mosquitto separately on `localhost:1883`, then run the
classifier and API in separate terminals:

```powershell
python -m cloud.mqtt.mqtt_classifier
```

```powershell
python -m cloud.api.app
```

The SQLite file `cloud/database/activity.db` is created automatically and is
excluded from Git. The included `activity_model.pt` and `activity_model.json`
are required together; the loader checks the checkpoint's SHA-256.

The subscriber currently connects to the local broker on port 1883. The lab's
external device connection uses authenticated MQTT/TLS on port 8883, and Android
uses the HTTPS API. See [RTSPARK_SETUP.md](../docs/RTSPARK_SETUP.md) for deployment
and [the server-update procedure](#update-an-existing-server) below for an
existing server's model update. The direct Python API command starts Flask's
development server on port 5000.

## Message contract

Sensor topic: `activity/sensor`. Each message carries `acceleration_x`,
`acceleration_y`, `acceleration_z` in **g**, and `gyro_x`, `gyro_y`, `gyro_z` in
**rad/s**. The active firmware schedules a nominal one-second interval.

The classifier consumes non-overlapping windows of five ordered readings.
Reconnects, invalid messages and arrival gaps greater than 2.5 seconds discard
an incomplete window. Each successful inference is stored and published to
`activity/prediction`, including `record_id`, `activity_id`, `activity` and
`confidence`.

| Request | Response |
| --- | --- |
| `GET /` | Service name and status |
| `GET /api/activity/latest` | Latest stored row, or `404` if empty |
| `GET /api/activity/history?limit=50` | `count` and newest-first `records`; limit 1–500 |

Stored rows contain `id`, `activity_id`, `activity`, `confidence` and
`created_at`. Activity IDs are `0 = WALKING` and `1 = RUNNING`.

## Model and evaluation

See the [classifier guide](classifier/README.md) for dataset attribution, units,
training splits, the model hash and test commands. The final six-trial live
score is **29/53 (54.7%)**; dataset evaluation and earlier Random Forest results
are separate. The [evidence index](../docs/testing/README.md) links the original
session audit and deployment records.

## Update an existing server

This procedure updates an existing Ubuntu deployment. The original lab cutover
completed on October 26, 2026; see the [deployment record](../docs/testing/2026-10-26_pytorch_cloud_deployment.md).
A completed deployment does not need another cutover to demonstrate completion.

Use the completed source on `main`. Preserve local changes before updating the
checkout. From the repository root in Windows PowerShell, set the SSH key and
server for the intended deployment, then build and transfer the update:

```powershell
$rtsparkSshKey = "C:\path\to\your-key.pem"
$rtsparkServer = "ubuntu@your-server"
$rtsparkBundle = Join-Path $env:USERPROFILE "Downloads\rtspark-pytorch-update.zip"
python tools\provisioning\build_pytorch_update.py --output "$rtsparkBundle"
scp -i "$rtsparkSshKey" "$rtsparkBundle" "${rtsparkServer}:/home/ubuntu/"
ssh -i "$rtsparkSshKey" "$rtsparkServer"
```

The bundle uses a file whitelist and SHA-256 manifest. It contains source,
model files, temporary-database tests and the update helper. It excludes the
live database, private headers, SSH keys and firmware builds.

In the Ubuntu SSH terminal, inspect the staged bundle before applying it.
Keep this terminal open so its staging variable remains available:

```bash
rtspark_stage=$(mktemp -d "$HOME/rtspark-pytorch-update.XXXXXX")
python3 -m zipfile -e "$HOME/rtspark-pytorch-update.zip" "$rtspark_stage"
"$HOME/rt-spark-activity-recognition/.venv/bin/python" \
  "$rtspark_stage/tools/provisioning/apply_pytorch_update.py"
```

The default mode checks staged hashes, the existing project and classifier
service, and prints the discovered unit and baseline database ID without
replacing files or restarting services. If the service cannot be matched
uniquely, inspect the actual unit; an explicit unit can be supplied with
`--service actual-name.service`.

Keep the board powered and publishing. Confirm that five fresh sensor messages
arrive:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
```

After five current readings arrive, apply the update:

```bash
"$HOME/rt-spark-activity-recognition/.venv/bin/python" \
  "$rtspark_stage/tools/provisioning/apply_pytorch_update.py" --apply
```

The helper installs CPU PyTorch in the existing virtual environment and runs
the ten model/database/API tests before stopping the classifier. It backs up
the seven source/model targets outside the repository, replaces them together,
and restarts the matched classifier unit. PASS requires a new database ID and
the expected checkpoint hash in fresh startup output. The helper waits up to
45 seconds for actual board data; it publishes no synthetic sensor messages
and inserts no synthetic prediction rows.

Expected checkpoint SHA-256:

```text
be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331
```

On a failed cutover or interruption, the helper attempts to restore the
previous source/model files and restart the previous classifier. The added
CPU PyTorch package remains installed. The database, API, broker, Nginx,
certificates, credentials and old deployed `activity_model.joblib` are not
replaced. Local tests cover simulated success/rollback; a live rollback has
not been exercised. Service and journal operations use `sudo -n` and require
existing permission; a separate `sudo -v` is not a prerequisite.

Retain the printed backup location and its `deployment.json`. Existing SQLite
rows have no model-version column, so use the deployment audit and new UTC
trial times to distinguish model versions. Confirm a fresh API record:

```bash
curl -sS --max-time 10 http://127.0.0.1:5000/api/activity/latest
```

From Windows, check the configured public TLS/HTTPS endpoints with
`python tools\validation\check_cloud_services.py`. This check alone does not
establish record freshness or model identity. The [firmware guide](../firmware/README.md#integration-verification),
[phone setup](../docs/RTSPARK_SETUP.md#run-the-same-app-on-a-physical-android-phone)
and [timed evaluation protocol](../docs/testing/TIMED_VALIDATION.md) cover the
separate physical checks. The recorded final live result remains **29/53
(54.7%)**; a future evaluation must use a new session and retain incorrect
predictions and the original scoring rules.
