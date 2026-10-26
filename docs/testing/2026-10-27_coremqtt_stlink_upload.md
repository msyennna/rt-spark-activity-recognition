# coreMQTT firmware: ST-LINK programming and verification

Reviewed October 27, 2026 (Asia/Manila). The user supplied the PowerShell
pull/upload transcript in the conversation. This report transcribes that
output; it is not an independent physical programming operation or a byte-exact
archived transcript. See [the structured record](2026-10-27_coremqtt_stlink_upload.json).

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

## Next runtime evidence

Open the board's actual enumerated serial port at **115200 baud**, then press
RESET once with the monitor already open. Capture startup through at least
five accepted PUBACK lines. The expected source-defined sequence includes:

- `[RTSpark] AWS coreMQTT 2.3.1 (MQTT 3.1.1)`;
- ICM20608 identity/initialization and six-axis readings;
- Wi-Fi connection and DHCP lease;
- `[RTSpark] TLS verified: TLSv1.2`;
- `[RTSpark] MQTT CONNACK accepted; topic activity/sensor QoS1`;
- advancing `[RTSpark] MQTT PUBACK id=... count=...` entries.

These are requested observations, **not results already established**. Earlier
custom-encoder PUBACKs and Random Forest movement results retain their original
attribution. After the runtime capture, verify broker reception, fresh
SQLite/API IDs, Android display and a new timed PyTorch movement test. Capture
per-line timestamps if assessing publication cadence; an untimed log does not
measure the one-second interval.
