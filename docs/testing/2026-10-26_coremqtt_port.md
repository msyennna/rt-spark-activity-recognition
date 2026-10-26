# AWS coreMQTT integration: October 26, 2026

This source/build milestone follows Laboratory Activity 3 (3).docx. The earlier
physical PUBACK capture used the custom MQTT encoder; it is not runtime proof
for this new library integration.

## Change

The firmware now links the actual, unmodified AWS coreMQTT 2.3.1 low-level
serializer/deserializer. The adapter preserves MQTT 3.1.1, a clean session,
30-second keepalive, authenticated CONNECT, QoS 1, non-retained messages, strict
packet-ID acknowledgement and the existing `activity/sensor` JSON contract.
PINGREQ/PINGRESP also use coreMQTT. The handout's
`MQTT_DO_NOT_USE_CUSTOM_CONFIG` definition applies to the library build.

Release/archive/file hashes and the MIT license are preserved under
`firmware/lib/coreMQTT`. Native tests verify the upstream source hashes before
compiling. The new startup banner is:

```text
[RTSpark] AWS coreMQTT 2.3.1 (MQTT 3.1.1)
```

## Verification performed here

| Check | Actual result |
| --- | --- |
| `python tests/firmware_host/run.py` | Five native suites passed, compiling the real coreMQTT source for packet and MQTT-state suites. Tests cover packet bounds, strict acknowledgements, ping, partial I/O, TLS gates, failure/reconnect paths, platform transport, heap and actual sensor-driver scaling. |
| `python firmware/scripts/check_cubemx.py` | PASS for the checked-in configuration and active USER CODE blocks. This is not a CubeMX generation run. |
| ARM firmware build | PASS with PlatformIO 6.1.18, ststm32 20.0.0, STM32Cube F4 1.28.3, GCC 12.3.1; dependency graph lists coreMQTT 2.3.1. |
| Static SRAM `.data` + `.bss` | 66,756 / 131,072 bytes. The linker also reserves 16,384 bytes for heap/stack in this SRAM bank. |
| Separate CCM TLS arena | 57,344 / 65,536 bytes, confirmed by `arm-none-eabi-size -A`. |
| Flash | 162,476 / 1,048,576 bytes; 1,616 bytes above the previous custom-encoder build. |
| `python -B tests/firmware_tls/test_tls.py` on this Linux host | Five cases **skipped** because the socket harness currently targets Windows. No new TLS runtime pass is claimed. The previously recorded Windows TLS results remain historical evidence; TLS implementation files were not changed by this port. |

The ARM build used the supported `PLATFORMIO_SETTING_ENABLE_TELEMETRY=No`
setting. An initial attempt was stopped by automatic approval review for
optional telemetry; after verifying the supported opt-out, the build succeeded
with telemetry disabled. Dependency checksum verification stayed enabled.
Existing linker warnings about unused libnosys syscalls and an RWX load segment
remain; the build is successful, not warning-free.

The build used public defaults, without the user's ignored private header.
No generated binary, credentials, or build cache is published. This build
artifact must not be treated as a configured, working board image.

## Remaining checks and literal handout alignment

No board was flashed here. Build on the user's computer with its existing
private configuration, then check the coreMQTT banner, sensor readings,
Wi-Fi/DHCP, verified TLS, accepted CONNACK and new PUBACKs. Confirm new database
records and the physical Android display. Measure one-second publishing from a
timestamped capture; the earlier untimed PUBACK excerpt did not establish it.

Actual CubeMX regeneration and the handout's driver-pack/AC6 instructions remain
unverified. Page 1 names RT-Spark and page 2 names VSCode PlatformIO, while later
steps describe LSM6DSL, ISM43362 and SPI3 hardware plus an AC6 `solution.yml`.
The demonstrated board has ICM20608/RW007 on I2C2/SPI2. That conflict needs the
instructor's corrected board-specific instructions; selecting drivers for
absent chips is not a valid correction.
