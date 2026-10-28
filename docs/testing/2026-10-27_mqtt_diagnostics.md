# MQTT disconnect and skipped-reading diagnostics

Reviewed October 27, 2026 (Asia/Manila). Raw broker logs, connection identifiers,
addresses and exact infrastructure event timestamps are excluded from this note.

## Findings

The user-supplied broker excerpt contains repeated connection closures and
reconnections. One previously observed long receipt pause overlaps a reconnect.
The more recent short receipt gap has no logged disconnect during its interval.
That gap therefore cannot be attributed to a reconnect from this evidence alone.

The precise broker wording matters. For the last reported Mosquitto 2.0.18,
the colon-form disconnect notice formats `strerror(errno)`. Its TLS handling
can set `EPROTO` on a failed SSL operation. The reported words alone therefore
do not establish a malformed MQTT packet or a fault in the coreMQTT serializer.
See the upstream [disconnect logging](https://github.com/eclipse-mosquitto/mosquitto/blob/v2.0.18/src/loop.c)
and [TLS/socket handling](https://github.com/eclipse-mosquitto/mosquitto/blob/v2.0.18/lib/net_mosq.c).

Older entries also replace an existing connection using the same client
identifier. A reconnect can overlap an old connection that the broker has not
yet removed; separate simultaneous publishers can also cause replacement.
These entries alone do not establish which situation occurred.

## Diagnostic change

The firmware previously collapsed terminal `mbedtls_ssl_read/write` errors to
`-1`. This erased the original TLS error code before the failure was logged.
Its sample-offer function also returned silently while MQTT was busy.

The updated foreground MQTT module now:

- Preserves terminal TLS read/write error codes in failure reports.
- Treats a zero-byte TLS read as EOF and closes that attempt immediately,
  following the pinned Mbed TLS read contract, rather than waiting for the
  MQTT reply timeout. A zero-progress write with nonempty pending data also
  fails as EOF.
- Adds the MQTT state and time spent in that state to failure reports.
- Reports the first skipped sample offer during a busy interval, then suppresses
  further skipped-offer reports until MQTT returns ONLINE. The cumulative
  `busy_offers` counter counts rejected local offers, not measured packet loss.
- Keeps pending packet bytes intact when a new offer is rejected, and prints
  neither sensor payloads nor credentials in these diagnostics.

This supplies missing evidence; it does not establish that the intermittent
network problem is repaired. No offline queue, catch-up burst, packet-format,
credential, trust-verification, classifier, model or scoring change is included.
The existing one-TLS-operation-per-poll design remains in place.

## Verification

The new regression first failed on the preceding firmware because a terminal
TLS error was not reported with its original code. After the change,
`python tests/firmware_host/run.py` passes all five native C suites. New cases
cover terminal read/write codes, EOF handling, bounded skipped-offer reporting,
preserved pending bytes, reporting after recovery, and absence of sample or
credential values from those reports. Existing verification gates, WANT retries,
partial I/O, acknowledgements, refusal, timeout and ping tests also pass.

`python firmware/scripts/check_cubemx.py` passes. This is a source guard,
not a new CubeMX generation or physical test.

The ARM build also passes for `black_f407zg`, using the pinned STM32 platform
20.0.0, STM32CubeF4 1.28.3, GCC 12.3.1 and actual coreMQTT 2.3.1. PlatformIO
reports SUCCESS in 471.68 seconds, including restoration of build dependencies.
Static SRAM is 66,764 / 131,072 bytes and flash is 162,900 / 1,048,576 bytes.
This is a local build with public defaults; private configuration is absent
and the image was not flashed or published. Existing libc syscall-stub and
RWX load-segment linker warnings remain. Rejected package-mirror downloads
were replaced by verified alternatives; no checksum verification was bypassed.

No board programming has occurred in this review. A user-computer build with
the local private configuration and subsequent serial evidence are next.

## User-computer verification

Use the board/laptop programming connection that previously completed ST-LINK
verification. Disconnect the power bank while using that laptop power setup.
From the integration worktree in Windows PowerShell:

```powershell
& {
    git pull --ff-only origin integration/stm32cubemx
    if ($LASTEXITCODE -ne 0) { return }
    python firmware\scripts\check_cubemx.py
    if ($LASTEXITCODE -ne 0) { return }
    $env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = "No"
    & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -d firmware -e black_f407zg -t upload
}
```

Inspect the build/programming verification result before checking runtime.
Enumerate serial ports, then open the actual board port at 115200 baud.
If it is still COM3:

```powershell
python -m serial.tools.list_ports
python -m serial.tools.miniterm COM3 115200 --eol CR
```

Capture a `sample skipped` report or the first `[RTSpark] ... failed` line
with nearby PUBACK/network messages. Use a single intended board publisher
during this check. Keep any raw logs local; publish only anonymized findings.
The next physical check should identify the state/error before another full
six-trial PyTorch evaluation, which remains pending.
