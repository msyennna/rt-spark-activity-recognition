# Post-upload Android confirmation

Recorded October 27, 2026 (Asia/Manila). After the
[post-upload Ubuntu broker/API check](2026-10-27_coremqtt_cloud_check.md),
the user was asked to open the Android app's Live and History views and
confirm advancing predictions. The user replied: **“yes it's working.”**

This is **user-confirmed Android operation**. No new screenshot, displayed
prediction ID, timestamp, device identifier, measured refresh interval or
exact API/display comparison was supplied with that reply. The earlier
physical-phone and October 26 screenshots remain separate historical evidence;
no new image review or exact displayed-row match is claimed here. The user's
confirmation is accepted to continue the laboratory workflow.

The latest independently described server-side output in this conversation is
five received sensor messages and database-backed local API record **6746**,
WALKING, confidence **0.9989190101623535**, created **2026-10-27 01:32:41 UTC**.
The programming, runtime and cloud-check reports retain their respective
observations and limits.

## Next: new PyTorch movement trial

Use the existing [timed protocol](TIMED_VALIDATION.md): three 60-second walking
trials alternating with three 60-second jogging trials. Keep board placement
consistent, keep power/Wi-Fi available, minimize the prediction view and follow
START/STOP cues. Rest between trials as needed, and report participation honestly.

The deployed checkpoint was verified during the October 26 cutover as
`be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331`.
Record the current on-disk checkpoint hash before the new trial so its result
can retain that model identity. The hash command is read-only; it does not
independently prove which model is already loaded by the running process.
The prior deployment report provides that startup-hash evidence.

In the existing Ubuntu SSH session:

```bash
sha256sum /home/ubuntu/rt-spark-activity-recognition/cloud/classifier/activity_model.pt &&
python3 /home/ubuntu/timed_walk_jog.py
```

The timer runs its fresh-prediction preflight before starting. It opens SQLite
read-only and saves a new uniquely named session folder. Return the model hash,
final summary and `RESULTS FOLDER` path, then retrieve/review the original
`session.json` and `summary.md` before committing the new result milestone.
No trial has been run or scored as part of this confirmation. The October 25
51/52 result remains attributed to Random Forest and must not be presented as
new PyTorch accuracy.
