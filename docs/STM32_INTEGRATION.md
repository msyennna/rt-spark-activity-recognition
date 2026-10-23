# STM32CubeMX integration milestones and verification

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

| Stage | Status | Evidence and limits |
| --- | --- | --- |
| Import unchanged Shanice firmware | Complete | `pio run -d firmware` passed on October 23, 2026: RAM 27,560 bytes; flash 56,212 bytes. This establishes a clean build before porting. |
| Review network MAC and CubeMX/ICM20608 differences | Pending integration review | Imported MAC and `.ioc` mismatch are documented below; changes require a separate build. |
| Direct MQTT/TLS port with foreground polling | Pending | Requires TCP, DNS, UTC clock, hardware RNG, verified TLS, authenticated MQTT, and matching QoS 1 PUBACK. |
| Sensor and Wi-Fi/DHCP regression checks | Pending physical board | The imported build has not been flashed or observed running during this integration. |
| Live TLS/MQTT acknowledgement | Pending physical board and network | A successful build does not establish TLS compatibility or broker receipt. |
| New database/API records | Pending live integration | Historical baseline records are preserved; they are not evidence of this firmware. |
| Android display and recovery | Pending live integration | Requires new board-originated predictions and Android observation. |

## CubeMX regeneration and hardware differences

The active PlatformIO build uses `firmware/src`, `firmware/include`, and the
lwIP build script. CubeMX regenerates into `firmware/activity_recognition/Core`
and related ignored framework directories. Regeneration alone does not update
the code PlatformIO compiles.

The imported `.ioc` contains an X-CUBE-MEMS1 LSM6DSL selection. The installed
sensor and compiled `icm20608.c` driver are ICM20608. That driver uses I2C2,
checks WHO_AM_I, configures ±2 g and ±250 degrees/second, and exposes both raw
and scaled readings. Do not replace it with generated LSM6DSL code. Review the
CubeMX sensor selection and pin assignments, then reconcile generated
initialization with the compiled sources through an explicit reviewed diff.

SPI2 is shared by the RW007 and onboard flash. Preserve the proven RW007 reset,
SPI handshake, transaction spacing, queued receive handling, and flash chip
select behavior. Preserve foreground polling and DHCP timer servicing.

The imported lwIP interface has a hard-coded MAC while startup separately
queries the RW007 station MAC. Compare the queried value with the interface
address used for Ethernet and DHCP; the module's real unicast station MAC
must be used consistently. Do not invent a shared replacement MAC for multiple
boards.

Until the `.ioc` and compiled initialization are reconciled, treat generated
files as review inputs. Review clock source/PLL settings, SPI prescalers,
GPIO levels and pulls, I2C2 bus behavior, UART, interrupt behavior, and any new
RNG initialization before copying generated code. Rebuild after reconciliation.

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

## Verification procedure

Run each stage separately and record its actual result before proceeding to the
next. Board checks require a deliberately flashed integration image; no flash
command is included in the build workflow.

| Check | Procedure and acceptance evidence | Physical board required? | Current result |
| --- | --- | --- | --- |
| Unchanged import build | `pio run -d firmware`; record RAM/flash and build completion. | No | Pass: 27,560 / 56,212 bytes |
| Port build | Run the same command after network/clock/RNG/TLS/MQTT changes; keep outputs private. | No | Pending port |
| Sensor output | Observe ICM20608 identity, six finite fields each second, stationary acceleration magnitude near 1 g and gyro near zero; rotate/move board to confirm axes respond. | Yes | Pending |
| Wi-Fi/DHCP | Confirm station join, actual module MAC, IP/netmask/gateway/DNS lease, lease renewal and reconnect; ensure poll continues while other connection work runs. | Yes | Pending |
| DNS/TCP/UTC/RNG | Confirm broker resolution, TCP 8883 connection, plausible synchronized UTC, and functioning hardware RNG; failures must stop TLS connection attempts. | Yes | Pending |
| TLS/authentication | Confirm CA, hostname and date verification succeed, valid MQTT CONNACK, and incorrect trust/credentials fail in a controlled test without printing secrets. | Yes | Pending |
| MQTT QoS 1/timing | Observe packet-ID-matched PUBACK and one six-field reading/second without catch-up bursts; check recovery after network interruption. | Yes | Pending |
| Database | Read new `activity_records` IDs and UTC `created_at` values while the integration board runs; nominally one prediction per five one-second readings. | Yes, for board-originated evidence | Pending |
| HTTPS API | Read `/api/activity/latest` and `/api/activity/history?limit=50` over verified HTTPS; IDs must match new database rows. | Yes, for board-originated evidence | Pending |
| Android | Confirm new matching IDs, latest prediction and newest-first history; power removal should show WAITING FOR SENSOR after 20 seconds while preserving the last reading, then return to LIVE after recovery. | Yes, plus Android device or emulator | Pending |

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
