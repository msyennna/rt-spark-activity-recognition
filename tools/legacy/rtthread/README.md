# Archived RT-Thread helpers

These files preserve the previously reviewed RT-Thread tooling unchanged:

- `firmware/apply_mbedtls_fixes.py`: patches the old RT-Thread mbedTLS 2.7.10.1
  package and installs the adjacent hardware entropy adapter.
- `firmware/tls_hardware.c`: adapter for that old package layout.
- `flashing/flash_rtspark_wifi.py`: validates and flashes the old RT-Thread
  `rtthread.elf`/`rtthread.bin` pair for the original lab ST-LINK serial.

They are retired from the active `integration/stm32cubemx` workflow. They do not
support the PlatformIO STM32Cube firmware. Their original default paths, firmware
banner, binary checks, and package assumptions are preserved as historical
source, including paths that no longer resolve from this archive location.

Do not execute these archived helpers against this integration checkout or use
them to flash its images. Historical use requires the matching baseline commit,
its original directory layout, and a deliberate review of the original target
and tool prerequisites. See baseline commit
`398c4bf26515e2d953827b4c4e16ea37ee1934ef` for that complete workflow.

The active build command, from the integration repository root, is:

```powershell
pio run -d firmware
```

It builds without flashing. Current setup and stage results are documented in
[`docs/RTSPARK_SETUP.md`](../../../docs/RTSPARK_SETUP.md) and
[the active firmware guide](../../../firmware/README.md).
