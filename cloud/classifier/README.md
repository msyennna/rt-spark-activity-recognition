# PyTorch activity classifier

The active classifier uses the PyTorch framework named in Laboratory Activity 3.
It consumes five ordered, nominally one-second readings from all six sensor
axes. Eight features per instant (the six axes plus two magnitudes) enter a
single GRU with eight hidden units and a two-class output layer: **450 trainable
parameters**. The saved CPU checkpoint is 5,005 bytes.

The six-field MQTT payload, five-reading window, WALKING/RUNNING IDs, confidence
field, SQLite schema and Android API contract are preserved. The Random Forest
checkpoint has been removed from the active tree; the previous working version
remains in Git history at c23b596.

The Ubuntu cutover reported PASS on October 26, 2026 at 04:08:05 UTC with a
matching model startup hash and new prediction ID 5942, followed by local API
ID 5943. See [the supplied deployment evidence](../../docs/testing/2026-10-26_pytorch_cloud_deployment.md).
Subsequent [public TLS/HTTPS and Android Live/History observations](../../docs/testing/2026-10-26_pytorch_android_public.md)
show later records 6060 and 6067/6068 respectively. These establish service and
display progress, not labelled live movement accuracy.

## Dataset and units

Use the handout's dataset, Viktor Malyi's
[Run or Walk version 2](https://www.kaggle.com/datasets/vmalyi/run-or-walk).
It contains 88,588 rows, one participant, both wrists and twelve recording dates.
The dataset license is CC BY-NC-SA 4.0; it remains excluded from Git.

CSV SHA-256:
56ab5e861da22f7b1b4eaefa2d6fdc170f6febc2cf03d5a1883a180dab083271

The creator describes raw accelerometer/gyro collection in
[Part 2](https://vmalyi.com/blog/run-or-walk-part-2/).
Raw Core Motion acceleration is in g and includes gravity; gyro rotation is in
rad/s. Those match the board's scaled payload. The creator's article discusses
50 Hz, while the version-2 CSV's median within-burst interval is about 0.200001
seconds and its Kaggle metadata says approximately 5.4 readings/second.
Preparation therefore uses actual CSV timestamps, not the article's frequency.

Run from the repository root:

~~~powershell
python tools/datasets/fetch_run_or_walk.py
python cloud/classifier/train_model.py
~~~

Install the pinned dependencies from requirements.txt in your Python environment
first. PyTorch is pinned to the CPU build to avoid downloading CUDA packages.

## Preparation and evaluation

A boundary is inserted on date/user/wrist/activity changes, time reversal, or
a gap greater than 0.65 seconds. Within each segment, non-overlapping five-second
windows select the nearest reading to each one-second target, within 0.20 seconds.
There is no interpolation or cross-boundary windowing.

The most recent two dates, July 16 and 17, are reserved before scoring as an
untouched chronological holdout. Five-fold development validation keeps complete
recording dates together. Normalizers and class weights use only each training
partition. Sixty epochs and the architecture were fixed before evaluation.
Training applies one proper 3D rotation to the entire window and to both
sensors together. Inputs are clipped to the configured board's +/-2 g and
+/-250 degree/second limits. This aids orientation robustness without removing
axis or time-order information.

Actual results recorded in
[the machine-readable training report](../../docs/testing/pytorch_training_2026-10-26.json):

| Evaluation | Correct / windows | Accuracy |
| --- | ---: | ---: |
| Development out-of-fold | 2219 / 2258 | 98.27% |
| Untouched latest-two-day holdout | 1197 / 1300 | 92.08% |

Holdout confusion matrix (actual rows, predicted columns; Walking then Running):
[[921, 101], [2, 276]]. This is a one-participant, cross-date dataset evaluation,
not an across-person score and not a live RT-Spark trial.

After evaluation, the deployment checkpoint is refit on all 3,558 windows.
Its reported evaluation scores belong to the pre-refit development/holdout
models, not an independent evaluation of that refit on already seen data.

## Inference and validation

predictor.py loads activity_model.pt using weights_only=True, verifies its
SHA-256 against activity_model.json, validates the contract/units, and returns
the existing activity_id/activity/confidence object. The MQTT subscriber drops
incomplete windows after reconnect, invalid data, or a delivery gap over 2.5
seconds; a failed window cannot wedge subsequent predictions.

~~~powershell
python -m unittest discover -s tests -p "test_*temporal*.py" -v
~~~

These tests exercise temporal order/all-axis gradients, rotation invariants,
time-window and date-fold isolation, checkpoint integrity, malformed inputs,
and the actual PyTorch-to-callback-to-temporary-SQLite-to-Flask path.

The prior 98.1% six-trial result belongs to the **Random Forest baseline**.
It remains preserved as historical evidence and must not be claimed for this
PyTorch model. A later participant-reported six-trial live PyTorch test completed
with all trials marked valid: **29/53 correct windows (54.7%)**, comprising
Walking **26/26** and jogging/Running **3/27**. This shows weak live jogging
recognition under that participant's test conditions. Independent replay of the
uploaded original session now confirms all six valid trials and the 29/53
result; see the [original-session audit and evidence limits](../../docs/testing/2026-10-27_pytorch_walk_jog_audit.md).
The planned live evaluation is complete. Model generalization remains a
limitation, and no weights or scoring rules were changed for the audit.
There is no resting class.
