# coreMQTT firmware: ST-LINK programming and verification

Recorded October 27, 2026 (Asia/Manila), from supplied Windows PowerShell
pull/upload output. This report is a transcription, without an independent
physical programming operation or a byte-exact archived transcript.

## Observed result

**Upload SUCCESS in 12.62 seconds**, with OpenOCD **Verified OK**. The pull
advanced the integration worktree from `b44d2ba` to `341d360`; that intervening
commit changed documentation only. The firmware source accepted after the
CubeMX review remains `b44d2ba`.

| Item | Supplied output |
| --- | --- |
| Target environment | `black_f407zg`, configured STM32F407ZGT6 |
| Image programmed | Local `firmware/.pio/build/black_f407zg/firmware.elf` |
| Protocol / uploader | ST-LINK / tool-openocd 3.1200.7, xPack OpenOCD 0.12.0 development build |
| Target session | `stm32f4x.cpu`, halted due to debug request |
| Library dependency | Actual coreMQTT 2.3.1 |
| SRAM / flash summary | 66,756 / 162,780 bytes |
| Programming | Started, then finished |
| Verification | Started, then `Verified OK` |
| Reset / shutdown | `Resetting Target`, then `shutdown command invoked` |
| Final result | `[SUCCESS] Took 12.62 seconds` |

The decisive reported sequence is preserved here as a transcribed excerpt:

```text
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
```

This closes the previously pending **local firmware programming and OpenOCD
verification** step. The transcript does not supply a binary SHA-256, ST-LINK
serial or independent full-image byte comparison. A reported reset action is
not itself a capture of application startup or proof that networking works.
No compiled image or private configuration is included in this milestone.

## Runtime verification procedure

Open the board's actual enumerated serial port at **115200 baud**, then press
RESET once with the monitor already open. Capture startup through at least
five accepted PUBACK lines. The expected source-defined sequence includes:

- `[RTSpark] AWS coreMQTT 2.3.1 (MQTT 3.1.1)`;
- ICM20608 identity/initialization and six-axis readings;
- Wi-Fi connection and DHCP lease;
- `[RTSpark] TLS verified: TLSv1.2`;
- `[RTSpark] MQTT CONNACK accepted; topic activity/sensor QoS1`;
- advancing `[RTSpark] MQTT PUBACK id=... count=...` entries.

The upload output alone does not establish these runtime results. Subsequent
[sensor/TLS/PUBACK observations](2026-10-27_coremqtt_runtime.md),
[cloud readback](2026-10-27_coremqtt_cloud_check.md) and the
[final PyTorch trial audit](2026-10-27_pytorch_walk_jog_audit.md) retain their
separate evidence scope. Earlier custom-encoder and Random Forest results keep
their original attribution. For a future upload, verify broker reception,
fresh SQLite/API IDs, Android display and any new timed movement test. Capture
per-line timestamps if assessing publication cadence; an untimed log does not
measure the one-second interval.

## Build and source identifiers

| Item | Recorded value |
| --- | --- |
| Source after pull | `341d36066eb237f51d1a18c108cefaa8f5d21a40` |
| Firmware source change | `b44d2ba1478a7596a8161eb38d3fcbc8cf561f03` |
| Development branch | `integration/stm32cubemx` |
| Upload command | `platformio run -d firmware -e black_f407zg -t upload` |
| Platform | `ststm32@20.0.0` |
| Framework | `framework-stm32cubef4@1.28.3` |
| OpenOCD banner | `xPack Open On-Chip Debugger 0.12.0+dev-02228-ge5888bda3-dirty (2025-10-04-22:44)` |
| Telemetry environment setting | `No` |

The output does not establish application startup, sensor readings, current
Wi-Fi/DHCP/TLS/MQTT state, post-upload database/phone observations, or a new
accuracy result. It provides no firmware SHA-256, ST-LINK serial or independent
full-image comparison. These limits apply to the upload record itself.
