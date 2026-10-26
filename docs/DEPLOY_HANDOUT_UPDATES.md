# Deploy the handout-alignment updates

The new PyTorch model and AWS coreMQTT source have been built/tested. The
Ubuntu PyTorch cutover now has [a supplied PASS and local API result](testing/2026-10-26_pytorch_cloud_deployment.md).
Do not repeat that successful deployment just to rerun this guide. The new
coreMQTT firmware still needs physical verification. Follow the handout's
cloud, firmware, then mobile order. Existing trial evidence remains historical.
The subsequent [public/Android check](testing/2026-10-26_pytorch_android_public.md)
also passed public TLS/HTTPS and shows new Live/History records. Remaining
work is local firmware rebuild/flashing and a fresh labelled movement trial, retaining
the hardware-instruction differences in the record. Another cloud cutover is
not needed.
The user has chosen to proceed with the installed RT-Spark hardware while
retaining the handout's hardware/AC6 differences in
[HANDOUT_ALIGNMENT.md](HANDOUT_ALIGNMENT.md).

## 1. Get the committed source in Windows PowerShell

```powershell
Set-Location "$env:USERPROFILE\rt-spark-activity-recognition\.worktrees\stm32cubemx"
git status --short --branch
git branch --show-current
```

The branch should be `integration/stm32cubemx`. If the status lists changes,
preserve/review them before pulling; do not clean/reset the working project.
With a clean tracked working tree:

```powershell
git pull --ff-only origin integration/stm32cubemx
python tools\provisioning\build_pytorch_update.py --output "$env:USERPROFILE\Downloads\rtspark-pytorch-update.zip"
scp -i "$env:USERPROFILE\Downloads\rtspark-yena.pem" "$env:USERPROFILE\Downloads\rtspark-pytorch-update.zip" ubuntu@15.134.214.242:/home/ubuntu/
ssh -i "$env:USERPROFILE\Downloads\rtspark-yena.pem" ubuntu@15.134.214.242
```

The ZIP uses an explicit file whitelist and SHA-256 manifest. It includes the
model, source, temporary-database tests and update helper. It excludes the live
database, private headers/keys and firmware builds. The cloud IP/hostname above
must still be the instance's actual address. Keep the PEM on your computer.

## 2. Inspect the staged update in the Ubuntu SSH terminal

The following commands are **Bash on Ubuntu**, not Windows PowerShell. Keep
this terminal open so its `rtspark_stage` variable remains available.

```bash
rtspark_stage=$(mktemp -d "$HOME/rtspark-pytorch-update.XXXXXX")
python3 -m zipfile -e "$HOME/rtspark-pytorch-update.zip" "$rtspark_stage"
"$HOME/rt-spark-activity-recognition/.venv/bin/python" \
  "$rtspark_stage/tools/provisioning/apply_pytorch_update.py"
```

This checks the staged hashes, existing project and actual classifier service,
then prints a read-only inspection with the unit name and baseline database ID.
It does not replace files or restart services. If service discovery cannot
uniquely match the project, stop and inspect the actual unit; do not guess it.
An inspected unit can be supplied with `--service actual-name.service`.

## 3. Apply the cloud model update on Ubuntu

Keep the board powered and publishing its real sensor readings using its
existing working firmware. Check reception first:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
```

After five current readings arrive, run:

```bash
"$HOME/rt-spark-activity-recognition/.venv/bin/python" \
  "$rtspark_stage/tools/provisioning/apply_pytorch_update.py" --apply
```

The helper installs CPU PyTorch in the existing virtual environment and runs
the actual ten model/database/API tests **before** stopping the classifier.
It backs up the seven source/model targets outside the repository, replaces
them together, and restarts only the matched classifier unit. Success requires
both a new database ID and the expected model SHA-256 in the new service
startup log. It waits up to 45 seconds for real board data; it sends no test
MQTT messages and inserts no synthetic database rows.

The expected model SHA-256 is:

```text
be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331
```

On a failed cutover or interruption, the helper attempts to restore the previous
source/model files and restart the previous classifier. It leaves the added CPU
PyTorch package installed. It does not replace the database, API, broker,
Nginx, certificates or credentials, and does not delete the old deployed
`activity_model.joblib`. This helper has passed local staged-model and simulated
rollback/success tests; the October 26 Ubuntu cutover also reported PASS. A live
rollback has not been exercised. The helper uses `sudo -n` for service/journal
operations and reports an error if existing permissions do not allow them;
the standalone `sudo -v` that failed in the supplied run is not a required step.

On success, save the printed backup location and `deployment.json` from it.
This audit identifies the model and first observed new ID. Existing SQLite
rows do not have a model-version column, so use that audit and new UTC trial
times when reporting results; do not relabel old rows as PyTorch predictions.

Confirm the API now advances:

```bash
curl -sS --max-time 10 http://127.0.0.1:5000/api/activity/latest
```

In a separate Windows PowerShell window, verify public service access:

```powershell
Set-Location "$env:USERPROFILE\rt-spark-activity-recognition\.worktrees\stm32cubemx"
python tools\validation\check_cloud_services.py
```

The public checker validates TLS/API access; a PASS by itself does not prove
that records are fresh or identify their model. Keep the deployment audit.

## 4. Build and validate the firmware on the user's computer

The user has chosen to continue with RT-Spark's installed ICM20608/RW007 and
I2C2/SPI2 configuration before receiving instructor clarification. Use the
reviewed actual-board port, including AWS coreMQTT. Retain the documented
LSM6DSL/ISM43362/SPI3/AC6 reference differences; do not select drivers for absent
chips or claim that those different pack steps were executed.

Keep the existing ignored private configuration. Do not replace credential
aliases with plaintext in tracked headers. Follow the scratch-project CubeMX
regeneration and review procedure in [firmware/README.md](../firmware/README.md).
Record the actual installed CubeMX version and generation result.

The user has now supplied a successful Windows build with the private header
present; [the original log and metrics](testing/2026-10-26_windows_coremqtt_build.md)
are preserved. The subsequent CubeMX 6.18.1 output has now been
[reviewed and reconciled](testing/2026-10-27_cubemx_generation_review.md). Pull
the accepted source, run the guard and rebuild locally with the existing
private configuration. Future generator runs still require the review workflow;
the preparation helper itself does not generate or flash.

For the reviewed actual-board source, in Windows PowerShell:

```powershell
python firmware\scripts\check_cubemx.py
python tests\firmware_host\run.py
python -B tests\firmware_tls\test_tls.py
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = "No"
pio run -d firmware
```

The native tests require the native C compiler used previously. Build does not
flash. Use the already reviewed board-programming procedure on the resulting
configured image; do not flash the public-default build from another machine.
Capture the new AWS coreMQTT 2.3.1 startup banner, real sensor readings,
Wi-Fi/DHCP, verified TLS, accepted MQTT connection and PUBACKs. Obtain a
timestamped capture to measure the one-second publish interval. Verify actual
broker reception and new database/API IDs again.

## 5. Verify the phone and repeat the movement trial

Open the existing app on the physical Android phone. Compare a fresh Live ID,
activity, confidence and time with the API, then check populated History and
smooth refresh/interaction. The API schema is unchanged; confidence values
and predictions may change with the new model.

Use the existing independently timed test on Ubuntu after the deployment and
firmware checks. If needed, copy the helper from the integration worktree using
the existing PowerShell SCP command before starting the SSH session.

```bash
python3 /home/ubuntu/timed_walk_jog.py --check-only
python3 /home/ubuntu/timed_walk_jog.py
```

The default is three 60-second walking trials and three 60-second jogging
trials, with rest allowed between them. Keep placement consistent and record
it. Retain incorrect/low-confidence predictions and all raw session files.
The helper creates a new unique results folder; preserve the older 98.1%
Random Forest trial unchanged. Report the new PyTorch score only after the
new trial is complete, and include the deployment model hash.

Copy the new session folder, deployment audit and relevant logs/screenshots
back to the integration worktree, review them for private information, then
commit and push that evidence milestone. No fresh live result is claimed by
the source/build checks in this update.
