# Windows coreMQTT firmware build

The user uploaded `Pasted text(5).txt` after building in the integration
worktree. Its original bytes are preserved in
[the build transcript](2026-10-26_windows_coremqtt_build.txt), with its SHA-256
and transcribed metrics in [the evidence JSON](2026-10-26_windows_coremqtt_build.json).
This is reviewed user-computer output, not an independent build on that PC.

## Result

**SUCCESS in 16.75 seconds.** The transcript shows a fast-forward pull to
`ceb55b3`, a `True` result for the private-header existence check, and a passing
CubeMX source/USER CODE guard before the PlatformIO build. The existence check
does not validate credential values; no private header or compiled image is
included in this evidence.

| Item | Reported value |
| --- | --- |
| Board | black_f407zg, STM32F407ZGT6, 168 MHz |
| Platform/framework | ststm32 20.0.0, STM32Cube F4 1.28.3 |
| ARM compiler | GCC 12.3.1 |
| MQTT library | Actual coreMQTT 2.3.1 in dependency graph; its serializer compiled and archived |
| Static SRAM | 66,756 / 131,072 bytes (50.9%) |
| Flash | 162,788 / 1,048,576 bytes (15.5%) |
| Output | `firmware.elf` linked and `firmware.bin` built |
| Telemetry setting in supplied command | `PLATFORMIO_SETTING_ENABLE_TELEMETRY=No` |

The linker reports eight libnosys syscall-stub warnings (`_close`, `_fstat`,
`_getpid`, `_isatty`, `_kill`, `_lseek`, `_read`, `_write`) and one RWX load-segment
warning. They did not prevent a successful build; this is not a warning-free
result or proof that every runtime path works. No warning was hidden or fixed
as part of this evidence-only assessment.

## CubeMX review preparation

The new `firmware/scripts/prepare_cubemx_review.py` copies exactly six public
inputs into a fresh folder outside the repository: the `.ioc`, three active C
files and two headers under the corresponding `Core` paths. It records their
hashes, refuses to overwrite an existing review folder, and does not read the
ignored private header. It does **not** launch CubeMX or generate code.

The source guard now also checks the candidate project's `.ioc` when
`--generated` is supplied; previously the setting checks read only the source
configuration. Four local tests passed: public-only seed scope, refusing an
existing destination, rejecting a changed candidate PLLQ while source remains
valid, and rejecting a generated duplicate sensor-bus owner. The active source
guard also passed. These tests ran in temporary folders, not in CubeMX.

## Remaining work

Open the prepared project in the user's installed STM32CubeMX, preserve user
code, generate in the review folder and run the guard. Inspect `.ioc` and
non-USER peripheral changes; generation may add a conflicting I2C2 owner that
needs explicit review. No generated file is automatically copied into the
active PlatformIO sources.

Actual regeneration, firmware programming/readback, coreMQTT startup/TLS/PUBACK
logs and a new labelled PyTorch movement test remain pending. No firmware
binary hash was supplied, and this build command did not flash the board.
The earlier separate CCM measurement remains historical; this Windows log
reports only PlatformIO's static-SRAM and flash summaries.
