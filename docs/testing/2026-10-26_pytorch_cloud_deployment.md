# PyTorch cloud deployment: October 26, 2026

The user supplied Ubuntu terminal output from the reviewed update helper and
the subsequent local API request. This report transcribes that evidence;
the reviewer did not independently access the EC2 instance. The structured
[evidence record](2026-10-26_pytorch_cloud_deployment.json) preserves the
reported model hash, cutover audit and API record without adding synthetic data.

## Outcome

**Cloud cutover reported PASS at 2026-10-26 04:08:05.963386 UTC**
(12:08:05.963386 Asia/Manila). The actual service is
`rtspark-classifier.service`. The deployed checkpoint SHA-256 matches the
reviewed public PyTorch artifact:

```text
be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331
```

| Observation | Supplied result |
| --- | --- |
| Update source | Integration commit `19dfcbea1dc388a9427bb9388ab3e8cdeb97ca91` |
| Read-only preflight | Correct model hash and classifier service; database ID 5888 |
| Real board reception before cutover | Five six-field messages received on `activity/sensor` through local Mosquitto |
| CPU package installation | `torch-2.14.1+cpu` successfully installed in the existing Python 3.12 virtual environment |
| Pre-cutover model/contract tests | Ten tests passed in 0.525 seconds; actual model and temporary database/API, not live-database test inserts |
| Database ID immediately before cutover | 5941 |
| New ID observed by helper | 5942 |
| Original files backed up by helper | `/home/ubuntu/rtspark-model-backups/20261026T040758Z_1961e7` |
| Subsequent local API record | ID 5943; WALKING (class 0), confidence 0.9988260865211487, created 2026-10-26 04:08:10 UTC |

The deployed helper requires both a new database ID and the expected full
model hash in the restarted classifier's fresh journal output before reporting
PASS. This is stronger evidence of the model change than a successful package
installation or an API response alone. The later API ID 5943 also exceeds the
helper's observed ID 5942. The server's saved `deployment.json` remains at the
reported backup location; the values here are the supplied console JSON, not
a separately downloaded original file.

## Privilege-check output

The preceding standalone `sudo -v` failed after three incorrect password
attempts. Bash continued to the separate Python command. The helper then
completed its service and journal operations and reported PASS. Its reviewed
implementation uses `sudo -n` for those specific operations. The exact sudo
policy that allowed them was not inspected; do not infer that the validation
failure was fixed or that unrestricted administrative access was enabled.
No password or sudoers changes are required by this successful deployment.

The runbook now invokes the helper directly instead of requiring the
standalone validation command that failed here. A live rollback was not
exercised; rollback coverage remains the earlier local simulated tests.

## Interpretation and remaining checks

The API confidence is about **99.88%**, not measured movement accuracy. The
actual activity during this API observation was not labelled. WALKING/RUNNING
remain the only model classes; this response does not establish resting
detection. The previous 98.1% six-trial result still belongs to Random Forest.

Cloud deployment is now evidenced with the existing board publisher. The
new coreMQTT firmware has not yet been flashed/observed. Public HTTPS access
after this cutover, physical-phone display of new PyTorch results, a fresh
labelled PyTorch movement trial, actual CubeMX regeneration and the handout's
hardware/toolchain conflict remain separate checks. The untimed sensor sample
also does not measure one-second publishing.

Continue with the phone/public check and the remaining applicable steps in
[the deployment guide](../DEPLOY_HANDOUT_UPDATES.md). Do not repeat an already
successful cutover just to collect another success message.

Subsequent evidence is now recorded in [the public/Android report](2026-10-26_pytorch_android_public.md):
all public checks passed with ID 6060 and later Android screens show LIVE 6067
and History through 6068. The limitations above describe this cutover transcript
itself; the later report separately states what the new evidence establishes.
