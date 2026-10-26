# Windows build after CubeMX source reconciliation

Reviewed October 27, 2026 (Asia/Manila). The user pasted the full PowerShell
pull/guard/build output in the conversation. This report transcribes that
output; it is not an independent run on the user's PC or a byte-exact archived
log. The [JSON record](2026-10-27_windows_reconciled_build.json) retains the
reported metrics and evidence limits.

## Result

**SUCCESS in 24.05 seconds.** The pull fast-forwarded the integration worktree
from `02bf2e1` to `b44d2ba`, the reviewed CubeMX configuration/USER CODE guard
passed, and PlatformIO rebuilt the changed active modules and linked the
firmware. The dependency graph includes actual coreMQTT 2.3.1.

| Item | Reported value |
| --- | --- |
| Source after pull | `b44d2ba1478a7596a8161eb38d3fcbc8cf561f03` |
| Branch | `integration/stm32cubemx` |
| Board | STM32F407ZGT6, 168 MHz, black_f407zg |
| Platform / framework / compiler | ststm32 20.0.0 / STM32Cube F4 1.28.3 / GCC 12.3.1 |
| Static SRAM | 66,756 / 131,072 bytes (50.9%) |
| Flash | 162,780 / 1,048,576 bytes (15.5%) |
| Optional telemetry | `PLATFORMIO_SETTING_ENABLE_TELEMETRY=No` in supplied command |
| Outputs | `firmware.elf` linked and `firmware.bin` built |

The eight previously observed libnosys syscall-stub warnings and one RWX
load-segment warning remain. They did not prevent linking; this is not a
warning-free build. No warnings were hidden or represented as fixed.

This local build follows the [actual CubeMX generated-file review](2026-10-27_cubemx_generation_review.md).
Its flash size differs from the public-default review-environment build;
without binary hashes and inspected local configuration, the images are not
claimed identical. No private configuration or binary was requested or
published. The source-only milestone does not establish physical flashing.

## Next physical step

Use the active [PlatformIO ST-LINK programming workflow](../../firmware/README.md#program-the-local-build).
Its upload target builds the local firmware, then invokes OpenOCD programming,
verification and reset. The repository already sets `upload_protocol = stlink`.
The selected board must be powered and connected through its ST-LINK/SWD path;
UART serial monitoring alone is not a programmer.

Record successful programming/verification before interpreting runtime output.
Afterward open the actual enumerated serial port at 115200, reset once and
capture the coreMQTT 2.3.1 startup banner, sensor initialization, Wi-Fi/DHCP,
verified TLS, accepted MQTT connection and PUBACKs. A failure remains a failed
stage until actual output establishes recovery. Broker reception, new database
rows, Android display and a fresh labelled PyTorch trial follow separately.
