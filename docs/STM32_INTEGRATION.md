# STM32CubeMX integration milestones and verification

Current handout alignment: the active classifier source/checkpoint and Ubuntu
service now use PyTorch. User-supplied cutover output reports matching model
hash and new ID 5942, followed by local API ID 5943. Earlier hardware/phone and
movement results retain their Random Forest attribution. See
[the deployment record](testing/2026-10-26_pytorch_cloud_deployment.md) and
[HANDOUT_ALIGNMENT.md](HANDOUT_ALIGNMENT.md).
The subsequent public checker passed all three TLS/HTTPS checks with ID 6060.
Post-cutover Android screens show LIVE record 6067 and History through 6068.
See [the public/Android evidence and unchanged screenshots](testing/2026-10-26_pytorch_android_public.md).
The user's later [Windows coreMQTT build](testing/2026-10-26_windows_coremqtt_build.md)
passed with static SRAM 66,756 bytes and flash 162,788 bytes. The private-header
file was present. The subsequent user-supplied CubeMX 6.18.1 output has now
been reviewed and reconciled into the active sources; see
[the generation review](testing/2026-10-27_cubemx_generation_review.md).
The subsequent [Windows rebuild](testing/2026-10-27_windows_reconciled_build.md)
pulled `b44d2ba`, passed the guard and succeeded in 24.05 seconds: static SRAM
66,756 bytes and flash 162,780 bytes. The subsequent
[ST-LINK upload](testing/2026-10-27_coremqtt_stlink_upload.md) reports programming
finished, Verified OK, target reset and SUCCESS in 12.62 seconds. The subsequent
[post-upload runtime](testing/2026-10-27_coremqtt_runtime.md) shows ICM20608
identity 0xAF, DHCP BOUND, verified TLSv1.2, accepted MQTT login and PUBACKs
1–7. The later [Ubuntu broker/API check](testing/2026-10-27_coremqtt_cloud_check.md)
returned five sensor messages and database-backed API record 6746, WALKING,
confidence 0.9989190101623535, created October 27 at 01:32:41 UTC. The user
then [confirmed Android is working](testing/2026-10-27_android_confirmation.md);
no new screenshot or exact displayed ID was supplied. The new labelled
PyTorch trial was attempted, but [stopped for missing prediction coverage](testing/2026-10-27_pytorch_trial_stopped.md):
first WALKING interval had zero eligible windows, then the next preflight
reported the latest prediction older than 15 seconds. No score is available;
raw-session and stream-health diagnostics remain pending.
The firmware source now links actual AWS coreMQTT 2.3.1; its ARM build and five
native suites passed. Its local image has now been programmed and verified
through ST-LINK in the supplied output, followed by positive sensor/network
runtime and seven PUBACKs. The excerpt omits the library-version banner;
attribution follows the build/upload sequence. Historical PUBACKs below retain
their previous custom-encoder attribution. The exact current
checks and Linux TLS-test skips are recorded in
[the coreMQTT report](testing/2026-10-26_coremqtt_port.md).

This branch integrates the reviewed Shanice firmware with Yena's existing cloud
and Android baseline. Build evidence and physical-board evidence are recorded
separately. No automatic flashing is part of this work.

## Sources and scope

| Input | Reviewed commit |
| --- | --- |
| `origin/feature/sensor-data-collection` | `398c4bf26515e2d953827b4c4e16ea37ee1934ef` |
| `origin/shanice-phase2-dhcp` | `a224cfa45ecfe4a846c470a3db167b3e3fbbbffe` |

The integration branch is `integration/stm32cubemx`, created in a separate Git
worktree from the working baseline. Shanice's tracked `firmware` tree replaces
the previous tracked RT-Thread tree. Cloud, Android, trained model, provisioning
scripts, existing documentation and trial evidence, and root secret exclusions
are preserved. RT-Thread-specific patch/flashing helpers are archived under
[`tools/legacy/rtthread`](../tools/legacy/rtthread/README.md).

## Milestone record

Completed commits pushed to `origin/integration/stm32cubemx`:

- `8cc22b9`: exact reviewed firmware import after its successful initial build.
- `4b780de`: active CubeMX workflow documentation and RT-Thread helper retirement.
- `c5cc43f`: MAC/CubeMX reconciliation, bare-metal MQTT/TLS port, and host tests.

| Stage | Status | Evidence and limits |
| --- | --- | --- |
| Import unchanged Shanice firmware | Complete | `pio run -d firmware` passed on October 23, 2026: RAM 27,560 bytes; flash 56,212 bytes. This establishes a clean build before porting. |
| PyTorch cloud cutover | PASS in supplied Ubuntu output | CPU PyTorch 2.14.1+cpu installed; ten pre-cutover model/contract tests passed; `rtspark-classifier.service` restarted with the expected model hash; baseline ID 5941 advanced to observed ID 5942 at 04:08:05 UTC, then local API ID 5943 at 04:08:10 UTC. |
| Post-PyTorch public/Android checks | Public checks PASS; Live/History observed | At 04:18:02.877096 UTC all three public checks passed with ID 6060 created 04:17:56 UTC. Android LIVE shows 6067 at 12:18:36 local time; History shows 6068–6064 newest first at five-second recorded-time gaps, all displayed as Walking 99.9%. No exact server-row match for screen IDs or labelled movement accuracy is claimed. |
| Handout AWS coreMQTT library integration | Source/build complete; positive post-upload runtime observed | Actual unmodified coreMQTT 2.3.1 low-level serializer/deserializer, default-config macro, five native suites and ARM build passed October 26. SRAM 66,756 bytes, separate CCM 57,344 bytes, flash 162,476 bytes. These software checks did not run CubeMX; the subsequent generator review is recorded separately. No new board runtime is claimed. |
| User-computer coreMQTT build | Windows build PASS | Supplied transcript pulls `ceb55b3`, confirms private-header existence, passes source guard and links coreMQTT 2.3.1. SUCCESS in 16.75 s, static SRAM 66,756 / 131,072 bytes and flash 162,788 / 1,048,576 bytes. Linker warnings remain; no binary hash, generation or flashing is shown. |
| Windows rebuild after CubeMX reconciliation | PASS in user-supplied transcript | Pull to `b44d2ba`, active guard PASS, coreMQTT 2.3.1 linked. SUCCESS in 24.05 s; static SRAM 66,756 bytes, flash 162,780 bytes. Existing linker warnings remain. No programming or runtime is shown. |
| coreMQTT ST-LINK programming | Programming and verification PASS in supplied output | Pull to documentation commit `341d360`; firmware source remains `b44d2ba`. OpenOCD reports programming finished, Verified OK, reset and SUCCESS in 12.62 s. This upload transcript alone does not show application networking; subsequent runtime is recorded separately. |
| Post-upload sensor/TLS/MQTT runtime | Positive path observed | WHO_AM_I 0xAF and sensor init success; Wi-Fi connected and DHCP BOUND at 192.168.1.17; DNS/TCP and UTC ready; TLSv1.2 verified; MQTT CONNACK accepted; PUBACK IDs/counts 1–7. No timestamps or independent version banner are present; new Ubuntu/DB/API/phone checks remain pending. |
| Post-upload Ubuntu broker and API | Five messages and newer record observed | Local subscriber returned five six-field sensor objects. Local HTTP latest returned 6746, WALKING, confidence 0.9989190101623535, October 27 01:32:41 UTC (09:32:41 Manila). This is database-backed API evidence, not a direct SQLite query, exact sensor-window association, measured response age or new public HTTPS check. Phone confirmation and labelled PyTorch trial remain pending. |
| Post-upload Android operation | User-confirmed | After being asked to check Live and History, the user replied that it is working. No new screenshot, displayed ID, timing measurement or exact API-row match was supplied. Confirmation is accepted to continue to the new PyTorch trial. |
| New PyTorch movement attempt | STOPPED; not scorable | Session `timed_walk_jog_20261027T020953Z_8619c6`, matching on-disk checkpoint hash, placement held in right hand. One confirmed 60-second WALKING interval has zero eligible windows/insufficient coverage; trial 2 preflight rejected a prediction older than 15 seconds. Raw session not yet reviewed; root cause is undetermined. Preserve this attempt and diagnose the stream before repeating. |
| CubeMX 6.18.1 generation review | Generated output supplied; source reconciliation complete | All 17 nonempty application USER CODE blocks survived. Removed the generated I2C2 main/MSP duplicates, retained custom_bus ownership, accepted GPIO output-latch preloads and disabled-EXTI cleanup. The original public ZIP and per-file hashes are retained. See the October 27 review for tests and build status. |
| Review network MAC and CubeMX/ICM20608 differences | Source complete; build passed | Module-reported station MAC replaces the shared constant; stale LSM6DSL selection removed, PLLQ corrected for RNG. CubeMX 6.18.1 output is now reviewed and reconciled; new firmware hardware checks remain pending. |
| Direct MQTT/TLS port with foreground polling | Source complete; build passed | `pio run -d firmware` passed on October 23, 2026: SRAM 66,756 / 131,072 bytes; separate CCM 57,344 / 65,536 bytes; flash 160,860 / 1,048,576 bytes. TCP, DNS, UTC, RNG, verified TLS and QoS 1 acknowledgement are implemented; initial physical-board TLS/CONNACK and broker reception were observed on October 23, 2026; October 26 runtime output now includes 17 accepted PUBACKs, IDs 1482–1498. Measured cadence and extended recovery checks remain pending. |
| Native C host verification | Pass | Actual packet, platform, MQTT state, bounded heap and ICM20608 modules pass five mock-driven native suites; no physical networking or cryptographic handshake is claimed. |
| Native TLS cryptographic verification | Pass: five cases | Actual Mbed TLS client completed verified ECDHE_RSA/AES-128-GCM with the 56 KiB arena. Wrong CA, hostname, premature date and expired date were rejected. The successful test yielded 68 times for crypto and twice for network readiness, with 86 foreground iterations. This is host evidence, not F407 polling latency. |
| Existing service reachability | Public checker PASS; later raw prediction flow reviewed | The October 25 14:55:32 UTC checker passed verified TLS/HTTPS checks but returned stale ID 3987. Later supplied raw session data contains 82 distinct October 25 prediction rows and 52 eligible movement-test windows. Raw audit passed. Later supplied local API responses match emulator History record 4754 and Live record 4877, and physical-phone Live record 5167. Two Live images show LIVE and advancing IDs. The local curl checks do not independently repeat public HTTPS trust verification. |
| Sensor and Wi-Fi/DHCP regression checks | October 23 lease observed; October 25 association restored | Earlier sensor/MAC/join/DHCP BOUND observations remain historical evidence. After an October 25 target-not-found failure, a later user-supplied reset excerpt shows Wi-Fi connected, LINK UP, DHCP START result=0 and ICM20608 identity 0xAF. A current DHCP lease, measured cadence and renewal remain unverified. |
| Live TLS/MQTT acknowledgement | PUBACK capture PASS; earlier TLS/login and broker reception observed | October 26 miniterm output contains 17 accepted PUBACKs, IDs/counters 1482–1498. The reviewed source prints these only after validating the pending publish's packet ID. Eighteen finite six-field readings are retained, with 17 complete reading/ACK pairs and one excerpt-tail reading. Timing is not recorded. See the [PUBACK report](testing/2026-10-26_cubemx_puback.md). |
| New database/API records | October 25 raw prediction evidence reviewed | Original session.json preserves 82 distinct records with IDs spanning 4083–4169 and current October 25 UTC timestamps. Independent time/ID selection reproduces 52 eligible windows and the saved metrics. Within-trial eligible prediction gaps are 5–6 seconds. See the [audited trial report](testing/2026-10-25_cubemx_walk_jog_trial.md). |
| Android display and recovery | Physical-phone Live/API match verified; History observed | Xiaomi 25028RN03A is selected in Android Studio, and a physical-phone photo shows History IDs 5027–5023 newest first. A later phone Live image shows LIVE and record 5167, matching supplied local API WALKING, confidence 0.675 (67.5%), 2026-10-26 02:08:39 UTC = 10:08:39 AM Asia/Manila. Earlier emulator matches remain 4754 (History) and 4877 (Live), with an advancing Live-image sequence. Earlier recovery remains user-confirmed; precise phone refresh timing and phone power-recovery measurements are separate. See the [phone report](testing/2026-10-26_android_phone.md). |
| Independently timed movement evaluation | Raw audit PASS; six valid trials | Session timed_walk_jog_20261025T153506Z_0abffb: 51/52 correct (98.1%), WALKING 25/26 and RUNNING 26/26. All recorded timing/data checks independently reproduce, and replay regenerates both original files byte-for-byte. Record 4092 is the single scored error: WALKING predicted RUNNING, confidence 0.505. This is one participant and one hand-held placement. |

October 23 live observations and their remaining checks are recorded in
[the live MQTT and local API report](testing/2026-10-23_cubemx_live_pipeline.md).
The exact flashed binary hash was not captured; the reviewed integration source
baseline for these observations is `63842cc`.

The [October 25 restart and Wi-Fi checkpoint](testing/2026-10-25_wifi_recovery.md)
records verified public reachability, stale prediction output, and subsequent
Wi-Fi association with DHCP started. It does not yet establish a new lease or
resumed sensor-to-database delivery at that checkpoint. A later
[user-supplied six-trial movement result](testing/2026-10-25_cubemx_walk_jog_trial.md)
records resumed prediction flow and 51/52 correct windows. The supplied raw
session files are now preserved unchanged, and independent time/ID selection,
quality checks and exact helper replay reproduce the result.

An [Android History/API observation](testing/2026-10-26_android_history.md)
reviewed October 26 confirms populated, newest-first visible cards and a direct
match to supplied local API record 4754. The local response contains nine newer
records with five-second completion gaps. A later
[Live/API observation](testing/2026-10-26_android_live.md) shows LIVE in both
images, advancing IDs 4874 to 4877, and an exact local API match for 4877. This
supports the requested automatic-refresh check; precise timing and intervening
input were not independently instrumented. The user subsequently confirmed the
target is the virtual phone in Android Studio. The emulator workflow is
established. Later [physical-phone evidence](testing/2026-10-26_android_phone.md)
shows populated History on the phone and Xiaomi 25028RN03A selected in Android
Studio. A subsequent phone Live image shows LIVE and record 5167, matching the
supplied local API by activity, confidence and UTC-to-Manila time conversion.
The physical-phone display/record comparison is complete.

The [October 26 PUBACK capture](testing/2026-10-26_cubemx_puback.md) closes the
previously missing runtime acknowledgement evidence. The [lab completion review](LAB_COMPLETION.md)
compares the working pipeline with the handout, including framework alignment,
CubeMX regeneration and training-data reproducibility.

## CubeMX regeneration and hardware differences

The active PlatformIO build uses `firmware/src`, `firmware/include`, and the
lwIP build script. CubeMX regenerates into `firmware/activity_recognition/Core`
and related ignored framework directories. Regeneration alone does not update
the code PlatformIO compiles.

The imported `.ioc` contained an X-CUBE-MEMS1 LSM6DSL selection. That stale
selection was removed; the installed sensor and compiled `icm20608.c` driver
remain ICM20608. That driver uses I2C2,
checks WHO_AM_I, configures ±2 g and ±250 degrees/second, and exposes both raw
and scaled readings. Do not replace it with generated LSM6DSL code. Review the
CubeMX sensor selection and pin assignments, then reconcile generated
initialization with the compiled sources through an explicit reviewed diff.

SPI2 is shared by the RW007 and onboard flash. Preserve the proven RW007 reset,
SPI handshake, transaction spacing, queued receive handling, and flash chip
select behavior. Preserve foreground polling and DHCP timer servicing.

The imported lwIP interface had a hard-coded MAC while startup separately
queried the RW007 station MAC. The integration initializes the interface using
that queried station MAC before link/DHCP activation. Zero and multicast MACs
are rejected; missing or invalid MAC leaves networking disabled while sensor
diagnostics continue. Confirm the MAC in the runtime logs and DHCP lease on
the physical board. There is no shared replacement MAC.

The `.ioc` and compiled clock initialization now agree on PLLM 4, PLLN 168,
PLLP 2 and PLLQ 7, with an 8 MHz HSE: CPU remains 168 MHz and the RNG domain
is corrected to 48 MHz. The original SPI2 prescaler 8 remains. These changes
require the physical regression checks below.

Treat future generated files as review inputs. Review clock source/PLL settings, SPI prescalers,
GPIO levels and pulls, I2C2 bus behavior, UART, interrupt behavior, and any new
RNG initialization before copying generated code. `custom_bus.c` retains sole
ownership of I2C2 initialization; remove any duplicate handle/init that CubeMX
generates in main.c, plus any generated HAL I2C MSP functions. Seed a scratch CubeMX `Core` tree from the active files,
then run the guard against the regenerated scratch project:

```powershell
python firmware/scripts/check_cubemx.py
python firmware/scripts/check_cubemx.py --generated path/to/scratch-project
```

The guard checks reviewed settings and preserves nonempty active USER CODE
blocks. It copies no files. The user supplied actual CubeMX 6.18.1 output;
[its review](testing/2026-10-27_cubemx_generation_review.md) records accepted
non-user changes and the generated I2C2 conflicts removed from active sources.
CubeMX omits the disabled EXTI NVIC key; the guard accepts omission while
rejecting enabled values and actual enable calls. Future generation remains a
reviewed operation; follow [`firmware/README.md`](../firmware/README.md).

## Preserved publisher contract

| Setting | Required value |
| --- | --- |
| Broker | `ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com:8883` |
| Server trust | Existing lab CA, hostname verification, certificate dates checked against UTC; TLS 1.2 or later |
| Authentication | Username `yena_sensor`, existing password, unique client ID; private local configuration |
| MQTT | 3.1.1, clean session, 30-second keepalive |
| Publish | `activity/sensor`, QoS 1, retain false; verify matching packet-ID PUBACK |
| Payload | Exactly six numeric fields: `acceleration_x`, `acceleration_y`, `acceleration_z`, `gyro_x`, `gyro_y`, `gyro_z` |
| Units | Acceleration `raw / 16384` in g; gyro `(raw / 131) * pi / 180` in rad/s |
| Timing | One reading per second; skip missed deadlines instead of publishing a catch-up burst |

The public CA's DER SHA256 is
`8d43e9e5f7554ac78539b3fc5fe4ea05da1885ff616ec3c640d594cf82b1b202`.
The baseline firmware CA and Android's
`android_app/app/src/main/res/raw/rtspark_ca.crt` were compared and match.
Private CA/server keys and broker password files remain outside Git.

Wi-Fi uses ignored `firmware/include/rw007_wifi_config.h`; MQTT uses ignored
`firmware/include/rtspark_private.h`. The two example headers contain no working
credentials. An empty MQTT password or client ID disables the publisher. The
pinned Mbed TLS archive/cache and PlatformIO outputs are ignored.

Mbed TLS 3.6.7 comes from the complete official release archive, verified against
the build script's pinned SHA256 before extraction. The STM32 platform, framework
and ARM GCC versions are pinned as well. Firmware linking places its fixed 56
KiB TLS allocator in the F407's separate 64 KiB CCM bank, reserves a 16 KiB
foreground stack, and bounds the C library heap to 8 KiB in SRAM. lwIP retains
its separate bounded SRAM allocation pool. `_sbrk` returns `ENOMEM` without
moving its cursor on exhaustion or invalid shrink; it cannot grow into the stack.
The linker reserves the stack separately; its generic minimum-heap reservation
is zero because the C heap is already counted as static SRAM. The final linked
image has no RT-Thread, socket or pthread dependency symbols. The ARM build
passes with toolchain warnings concerning unused libnosys system calls and
the default RWX load segment; it is not a warning-free build.

The dependency build applies one hash-checked upstream adaptation so TLS 1.2
ECDHE_RSA uses Mbed TLS's existing restartable ECDH paths. RSA signature
verification remains synchronous. The actual-library localhost suite validates
the adaptation and negative verification cases; see
[`tests/firmware_tls/README.md`](../tests/firmware_tls/README.md) for the exact
source hashes and retest requirements when updating Mbed TLS. Board polling
latency and stack/arena high-water marks still need physical measurement.

The UTC implementation resolves `time.cloudflare.com`, checks SNTP source,
mode/version/leap/stratum and the random request token, and handles the 2036 NTP
era rollover. SNTP bootstraps certificate time; it is not authenticated time.
UTC refreshes hourly and expires after 24 hours. Blocking DNS or UDP 123 leaves
TLS waiting for UTC; there is no manual shell-time fallback in this port.

## Verification procedure

Run each stage separately and record its actual result before proceeding to the
next. Board checks require a deliberately flashed integration image; no flash
command is included in the build workflow.

| Check | Procedure and acceptance evidence | Physical board required? | Current result |
| --- | --- | --- | --- |
| Unchanged import build | `pio run -d firmware`; record RAM/flash and build completion. | No | Pass: 27,560 / 56,212 bytes |
| Port build | Run the same command after network/clock/RNG/TLS/MQTT changes; keep outputs private. | No | Pass: SRAM 66,756; CCM 57,344; flash 160,860 bytes |
| Native C verification | `python -B tests/firmware_host/run.py`; actual packet/platform/MQTT/heap/sensor modules with mocked HAL/lwIP/TLS/I2C. | No | Pass: five suites |
| Native TLS verification | `python -B tests/firmware_tls/test_tls.py`; local test certificates and the actual Mbed TLS client, no cloud messages. | No | Pass: verified handshake and four certificate rejection cases |
| CubeMX source guard | `python firmware/scripts/check_cubemx.py`; compare reviewed `.ioc` settings and USER CODE blocks. | No | Source guard and reconciled candidate pass. Actual CubeMX 6.18.1 output is preserved and reviewed; 12 regression tests pass. |
| Existing service reachability | `python tools/validation/check_cloud_services.py`; TLS 8883 and read-only verified HTTPS schema checks. | No | PASS in supplied October 25 output. MQTT application bytes sent: 0. Latest/history both returned stale ID 3987; this checker does not verify firmware database updates. |
| Sensor output | Observe ICM20608 identity, six finite fields each second, stationary acceleration magnitude near 1 g and gyro near zero; rotate/move board to confirm axes respond. | Yes | Partial: WHO_AM_I 0xAF and six-field readings observed. Motion response and measured cadence remain pending. |
| Wi-Fi/DHCP | Confirm station join, actual module MAC, IP/netmask/gateway/DNS lease, lease renewal and reconnect; ensure poll continues while other connection work runs. | Yes | October 23 lease observed. Latest October 25 excerpt confirms Wi-Fi connection, LINK UP and DHCP START result=0; current DHCP BOUND/address and renewal are not yet shown. |
| DNS/TCP/UTC/RNG | Confirm broker resolution, TCP 8883 connection, plausible synchronized UTC, and functioning hardware RNG; failures must stop TLS connection attempts. | Yes | Partial: DNS/TCP connected and UTC ready reported before verified TLS. Detailed clock/RNG and failure-path measurements remain pending. |
| TLS/authentication | Confirm CA, hostname and date verification succeed, valid MQTT CONNACK, and incorrect trust/credentials fail in a controlled test without printing secrets. | Yes | Partial: positive board TLSv1.2 and MQTT CONNACK observed. Negative board-runtime cases remain pending; native negative tests are separate evidence. |
| MQTT QoS 1/timing | Observe packet-ID-matched PUBACK and one six-field reading/second without catch-up bursts; check recovery after network interruption. | Yes | PUBACK capture PASS: 17 consecutive accepted IDs/counters 1482–1498 in the October 26 board transcript. The source parser checks PUBACK type, length and matching pending packet ID before logging. No per-line timestamps are present, so measured cadence and controlled network-interruption behavior remain separate. Earlier Android power recovery remains user-confirmed. |
| Database | Read new `activity_records` IDs and UTC `created_at` values while the integration board runs; nominally one prediction per five one-second readings. | Yes, for board-originated evidence | Raw session audit PASS: 82 distinct current rows retained, 52 eligible windows, no duplicate selected IDs, all stored analyses/metrics reproduced. Eligible completion gaps are 5–6 seconds. Later local API evidence matches Android History record 4754 and Live record 4877. The latest supplied history response contains IDs 5169–5150 with eighteen 5-second completion gaps and one 6-second gap, and its record 5167 matches the physical-phone Live display. Board PUBACK evidence is now preserved separately for IDs 1482–1498. Sensor acquisition cadence remains unmeasured; local curl does not independently repeat public HTTPS trust verification. |
| HTTPS API | Read `/api/activity/latest` and `/api/activity/history?limit=50` over verified HTTPS; IDs must match new database rows. | Yes, for board-originated evidence | Earlier verified HTTPS latest/history?limit=1 returned stale ID 3987. Later local HTTP history responses match emulator records 4754 and 4877, and physical-phone Live record 5167. The latest limit=20 response contains IDs 5169–5150 at 5–6-second completion gaps. These local curl checks do not independently repeat public HTTPS certificate verification or the full limit=50 check. |
| Android | Confirm new matching IDs, latest prediction and newest-first history; power removal should show WAITING FOR SENSOR after 20 seconds while preserving the last reading, then return to LIVE after recovery. | Yes, plus Android device or emulator | Physical-phone History and Live observed for the Xiaomi target. Live record 5167 matches local API WALKING, 67.5%, and 02:08:39 UTC = 10:08:39 AM Asia/Manila; LIVE badge is visible. Earlier emulator matches are 4754 and 4877, and its two Live images advance by three IDs over 15 seconds of recorded timestamps. Earlier power recovery remains user-confirmed. Precise polling and a repeated phone power-interruption test were not instrumented by these images. |
| Timed walking/jogging evaluation | Follow [TIMED_VALIDATION.md](testing/TIMED_VALIDATION.md); retain requested labels, start/end times, raw rows, quality exclusions and confusion matrix. | Yes | Raw audit PASS: six valid trials, 51/52 correct (98.1%). Original session.json and summary.md are preserved unchanged with hashes and audit.json. Independent selection and exact helper replay agree; no new physical trial was performed by the reviewer. |

Use read-only database/API checks for live validation. Do not publish synthetic
readings into the deployed `activity/sensor` topic as a substitute for a board
test; that would create unrelated predictions in the working database. Isolated
software tests may use temporary databases and local mocks.

The classifier receives messages locally on `localhost:1883`, collects five
readings, predicts WALKING or RUNNING, then inserts a database row. A PUBACK
proves broker receipt, not classifier processing. Verify each downstream stage
independently. The classifier's existing subscription defaults to QoS 0.

The provisioning scripts are already applied on the server. Their documented
`--check-only` modes can inspect the existing setup; do not reinstall listeners
or replace certificates merely to validate this branch. EC2 source-IP rules must
allow the board's public network on 8883 and Android's network on HTTPS 443.

## Evidence handling

Record firmware commit, build result, board identity, test time, and observed
outcome for each future hardware milestone. Keep credentials and compiled images
private. The imported serial diagnostics can print the configured SSID and its
bytes, so review logs before committing any excerpts.

The October 20, 2026 baseline observations and exploratory walking/jogging trial
remain historical evidence under `docs/testing`. They establish neither new
integration success nor an independently measured model accuracy score.

