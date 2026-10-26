# PyTorch deployment bundle verification

The cloud migration source/checkpoint is committed as `f607eab`. This record
tests the deployment packaging/helper; it is not a live deployment result.

## Local verification

- Built `rtspark-pytorch-update.zip` from the explicit 12-file whitelist, plus
  its SHA-256 manifest. No live database, key, private header or firmware binary
  is included.
- Extracted into a fresh staging directory and ran the helper's `--stage-only`
  mode with CPU PyTorch 2.14.1+cpu. All ten actual model, MQTT callback,
  temporary SQLite and Flask API contract tests passed.
- Four helper tests passed: another project's service is rejected; failed
  restart restores previous files and removes newly introduced targets;
  KeyboardInterrupt follows the restore path; success requires a fresh record
  and the matching loaded-model startup hash. Service commands are mocked in
  these four tests and only temporary files are changed.

The helper defaults to read-only inspection. `--apply` changes only the seven
listed classifier/model files after dependency and staged-model checks, with a
backup outside the repository and stop/start of the matched classifier unit.
It installs CPU PyTorch in the existing venv; that package remains installed
after source rollback. Success requires real sensor-driven database progress
and the expected model hash from the service journal. It emits no synthetic
MQTT readings and does not replace the database or API infrastructure.

## Pending live evidence

The actual Ubuntu package installation, unit discovery, restart, journal check,
live rollback and sensor-driven cutover have not run here. The user must run
the [deployment instructions](../DEPLOY_HANDOUT_UPDATES.md) on their Ubuntu
host with the board publishing, and preserve the resulting `deployment.json`.
The current public checkpoint SHA-256 is
`be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331`.
