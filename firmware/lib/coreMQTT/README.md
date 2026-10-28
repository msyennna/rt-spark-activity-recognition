# AWS coreMQTT 2.3.1

This directory contains the **unmodified upstream serializer/deserializer**
implementation from [FreeRTOS/coreMQTT v2.3.1](https://github.com/FreeRTOS/coreMQTT/tree/v2.3.1).
The MIT license is included. `UPSTREAM.json` records the release archive and
each selected file's SHA-256 and original path. The native test runner checks
those hashes before compiling.

Version 2.3.1 implements MQTT 3.1.1, the protocol used by the existing broker
contract. Later coreMQTT 5.x releases implement MQTT 5; this port deliberately
pins the MQTT 3.1.1 release.

The [AWS library documentation](https://docs.aws.amazon.com/freertos/latest/userguide/coremqtt.html)
supports using its composable low-level API directly. The application adapter
in `firmware/src/rtspark_mqtt_packet.c` calls the actual library for CONNECT,
QoS 1 PUBLISH, CONNACK/PUBACK, and PINGREQ/PINGRESP. The foreground application
continues to own TCP/TLS partial I/O, deadlines, reconnects and pending packet
IDs. It does not use coreMQTT's high-level connection/state API, subscriptions,
or an RTOS thread.

`firmware/platformio.ini` defines `MQTT_DO_NOT_USE_CUSTOM_CONFIG`, as requested
by the handout. Both ARM and native test builds compile the upstream source
with its default configuration. The local `library.json` is a PlatformIO build
descriptor, not a modified implementation of coreMQTT.

This completes the library-source integration. It does not execute the
handout's separate `cpackget`/component-editor/AC6 steps or resolve its
LSM6DSL/ISM43362/SPI3 conflict with the demonstrated RT-Spark hardware. See
[the hardware/toolchain notes](../../../docs/LAB_COMPLETION.md#hardwaretoolchain-reference-differences).
