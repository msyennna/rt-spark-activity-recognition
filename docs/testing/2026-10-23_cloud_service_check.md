# Read-only cloud service check — October 23, 2026

The integration worktree preserved the working cloud/Android baseline at
`398c4bf26515e2d953827b4c4e16ea37ee1934ef`. This check used the existing service
hostname, `ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com`, and the
unchanged Android trust anchor,
`android_app/app/src/main/res/raw/rtspark_ca.crt`.

## Observed result

At **2026-10-23 05:41:24 UTC / 13:41:24 Asia/Manila**, an authorized run outside
the filesystem/network sandbox returned:

| Check | Result | Observation |
| --- | --- | --- |
| MQTT port 8883, TLS 1.2 with lab CA and hostname verification | **UNREACHABLE** | Connection operation timed out after the configured 8-second timeout. |
| HTTPS GET `/api/activity/latest` | **UNREACHABLE** | Connection operation timed out after the configured 8-second timeout. |
| HTTPS GET `/api/activity/history?limit=1` | **UNREACHABLE** | Connection operation timed out after the configured 8-second timeout. |

The initial sandboxed run at 05:40:44 UTC returned `PermissionError` for all three
operations. It was retried with authorized network access; the table records
that second run. These observations do not establish a TLS certificate failure,
a service outage, a successful API schema check, or a firmware fault. No service
response or existing prediction record was obtained.

The helper sends no MQTT CONNECT, credentials, subscriptions, or sensor messages.
It uses HTTPS GET only and follows no redirects. It performs no provisioning or
database writes and logs no activity labels, confidence values or raw responses.

Rerun from the repository root when the existing services are reachable:

```powershell
python tools/validation/check_cloud_services.py
```

The helper reports `PASS`, `FAIL` or `UNREACHABLE` per check. A reachable API must
match Android's ID, WALKING/RUNNING label mapping, finite confidence in `[0, 1]`,
and strict SQLite UTC timestamp contract. History must respect `limit=1` and its
declared count. Output includes only record IDs/UTC times and record counts.
The helper uses only the lab CA and TLS 1.2, matching the firmware protocol.

## Preservation verification

All **58 files** present in the baseline paths below were checked against their
baseline Git blob hashes using `git hash-object --path=...`, which applies the
repository's normal line-ending filters. Every file matched; there were no
missing files. These checks include the model, Android code and CA, cloud code,
data collection utilities, provisioning scripts and historical trial evidence.

| Baseline path | Files checked | Result | Baseline Git tree |
| --- | ---: | --- | --- |
| `cloud/` | 6 | PASS | `e4688cc6b34fcbce03a3f579b434fd8eff7d6035` |
| `android_app/` | 44 | PASS | `6f23fc5a5d08be1f0d81c801509ce4769d76ce54` |
| `data_collection/` | 4 | PASS | `fe7b668f99a6fe869a6f4a18baa099324340ddd8` |
| `tools/provisioning/` | 2 | PASS | `860a36467eec5e66a07edbdd2de08bb4aaa39e19` |
| Historical `docs/testing/` files | 2 | PASS | `a23fb55314f59923a549e8758189c1ba1e37f11f` |

The root `.gitignore` and `requirements.txt` also exactly matched their baseline
Git blobs, `6a58f773c5851f61d0b1e0c6510ad7efbc1a246f` and
`fc12776a8490bf169194598268b463de80b0c375`, respectively. Root secret exclusions
were therefore retained.

The unchanged `cloud/classifier/activity_model.joblib` has SHA-256
`a4b31f34ee01d82e1994e7d10f03e0c3f61d9f54369029eadd8a426149c1bac1`.

## Evidence still required

A read-only API response, even when available, establishes only that existing
records can be read. It cannot establish that the new STM32CubeMX firmware
published acknowledged MQTT readings or produced new database rows. Those
checks require the physical board, configured local credentials, and a reachable
service. Correlate board PUBACKs with increasing database IDs/UTC times, then
verify those new rows on Android. No board was flashed for this check.

Wrong-CA/hostname rejection and the embedded TLS client's behavior require their
separate native TLS tests; this unreachable service run did not test them.
