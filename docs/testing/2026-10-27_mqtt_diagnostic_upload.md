# Diagnostic firmware: local Windows upload verified

Reviewed October 27, 2026 (Asia/Manila). A user-supplied terminal transcript
shows the Windows integration worktree fast-forwarding to `118aa01`, passing
the CubeMX source guard, recompiling the MQTT module and uploading the image
through ST-LINK. This note records aggregate verification results; the raw
transcript and local machine identifiers are excluded.

| Check | Supplied result |
| --- | --- |
| Active source and USER CODE guard | OK |
| Target | STM32F407ZG, STM32Cube/HAL |
| MQTT library | coreMQTT 2.3.1 |
| Updated source compiled | `rtspark_mqtt.c` |
| Static SRAM | 66,764 / 131,072 bytes |
| Flash | 163,204 / 1,048,576 bytes |
| Programming | Finished |
| OpenOCD verification | Verified OK |
| Post-upload reset | Performed after verification |
| Overall result | SUCCESS in 27.20 seconds |

The transcript reports the existing syscall-stub and RWX load-segment linker
warnings. They did not stop compilation, linking or the reported upload.
No compiled image, private configuration or binary hash was supplied for
independent inspection. This is evidence of the reported local programming
and verification; application startup/network behavior is the next check.

The uploaded change is the [TLS-error and skipped-offer diagnostic update](2026-10-27_mqtt_diagnostics.md).
It preserves detailed TLS errors and reports a busy MQTT state when a sample
offer is skipped. A skipped-offer notice while waiting for startup prerequisites
does not by itself establish a fault. After connection/acknowledgements begin,
capture the first failure or skipped-offer report with nearby network/PUBACK
messages. If connection never completes, retain the startup/failure sequence.

Use the actual enumerated serial port at 115200 baud. Open the monitor before
one deliberate board reset to capture startup, then allow about 90 seconds
for observation. Keep raw logs local and publish only anonymized findings.

No new activity-accuracy result follows from the upload. The intermittent
delivery cause and the complete valid six-trial PyTorch movement evaluation
remain pending.
