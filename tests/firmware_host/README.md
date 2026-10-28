# Firmware host verification

From the repository root:

```powershell
python -B tests/firmware_host/run.py
```

The runner uses native GCC or Clang, compiling with warnings treated as errors.
Set the `CC` environment variable to the compiler executable if it is not on
PATH. On the integration workstation, it also detects
`C:\msys64\ucrt64\bin\gcc.exe`. Executables live only in a temporary directory.

These tests compile the actual firmware modules. HAL, lwIP and Mbed TLS calls
are mocked to drive failure and partial-I/O conditions reproducibly:

- `test_packets.c`: exact authenticated clean CONNECT flags and keepalive;
  QoS 1/non-retained PUBLISH, multi-byte remaining length, output bounds,
  rejected wildcard topics, strict CONNACK and matching packet-ID PUBACK.
- `test_platform.c`: receive-ring wraparound and TCP window backpressure;
  accepted TCP writes are not replayed after output failure; EOF, stale DNS
  callbacks, connection timeout, network loss; rejected NTP source/port/token,
  unsynchronized/invalid replies and old dates; NTP era rollover in 2036,
  HAL tick rollover and UTC expiry; RNG repeated-word, hardware-status,
  timeout and PLL-clock faults fail closed.
- `test_mqtt_state.c`: credentials/DHCP/clock/RNG prerequisites; required
  certificate/hostname/TLS configuration; failed verification prevents CONNECT;
  one TLS read/write/handshake step per poll; unchanged buffer retries,
  fragmented acknowledgements, delivery serialization, packet-ID rollover,
  refusal/mismatch/timeout cleanup, no offline sample queue, idle keepalive,
  and network-loss handling.
- `test_heap.c`: actual bounded 8 KiB newlib heap, exact exhaustion, negative
  shrink, `PTRDIFF_MIN`/`PTRDIFF_MAX`, and unchanged allocation cursor on failure.
- `test_sensor.c`: actual ICM20608 driver, identity/range register setup,
  propagated I2C failures, signed extreme raw readings, temperature-byte
  separation, and the documented acceleration/gyro scales.

The mocks do not validate the cryptographic implementation, actual radio/DHCP
traffic, sensor values, hardware RNG quality, timing on the F407, live broker
interoperability, database changes or Android rendering. Those remain separate
checks in the [firmware verification guide](../../firmware/README.md#integration-verification).

Tests supply deliberately fake public credentials. They never include the local
private header, open a network connection, access a deployed database or flash
a board.
