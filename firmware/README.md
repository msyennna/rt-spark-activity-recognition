# STM32CubeMX firmware

PlatformIO builds the reviewed `src/` and `include/` trees for the STM32F407ZG.
The source started from Shanice's DHCP firmware at
`a224cfa45ecfe4a846c470a3db167b3e3fbbbffe`. The integration keeps the original
RW007 SPI reset/handshake, command framing, foreground Ethernet service, and
ICM20608 measurement driver. RT-Thread is not part of this build; names of wire
protocol constants retain the RW007 firmware's ABI.

From the repository root:

```powershell
python firmware/scripts/check_cubemx.py
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = "No"
pio run -d firmware
```

The build command does not flash the board. See
[`docs/RTSPARK_SETUP.md`](../docs/RTSPARK_SETUP.md) for local configuration and
separate board/cloud/Android verification steps. Keep Wi-Fi and MQTT credentials
in ignored local headers. Never copy those headers into CubeMX output or commits.

## MQTT library and handout alignment

The active firmware links AWS coreMQTT 2.3.1, using its upstream low-level
serializer/deserializer for MQTT 3.1.1 packets and acknowledgements. The build
defines `MQTT_DO_NOT_USE_CUSTOM_CONFIG`. See the
[library provenance](lib/coreMQTT/README.md) and the
[October 26 build/test record](../docs/testing/2026-10-26_coremqtt_port.md).
The foreground application owns transport, timing and reconnects. No RT-Thread
or FreeRTOS kernel is introduced by the library integration.

The handout names RT-Spark and PlatformIO, but later describes different
LSM6DSL/ISM43362/SPI3 hardware and an AC6/component-pack workflow. The actual
board uses the settings below. On October 26 the user chose to continue with
the actual hardware while instructor clarification remains unavailable. Keep
that distinction in the completion record; literal compliance with different
chip/pack steps is not established. See
[HANDOUT_ALIGNMENT.md](../docs/HANDOUT_ALIGNMENT.md).

## Hardware settings

- The actual sensor is **ICM20608**, on hardware **I2C2**, PF0 SDA / PF1 SCL,
  at 100 kHz. `custom_bus.c` owns its handle and initialization; `icm20608.c`
  validates WHO_AM_I before configuring the sensor. The stale LSM6DSL
  X-CUBE-MEMS1 selection was removed from the `.ioc`.
- RW007 uses SPI2 PC2 MISO / PC3 MOSI / PB13 SCK, software CS PF10, reset PG15,
  and INT/BUSY PG11. SPI2 retains the board-tested prescaler 8 (5.25 MHz).
  PB12 stays high to deselect the onboard flash. PG11 falling edges are latched
  by EXTI and polled; its NVIC interrupt stays disabled.
- The netif now uses the station MAC returned by `CMD_MAC_GET`, before enabling
  link/DHCP. Zero and multicast addresses are rejected. Missing or invalid MAC
  leaves networking disabled while sensor diagnostics continue. There is no
  fallback shared MAC address.
- HSE is 8 MHz, PLLM 4, PLLN 168, PLLP 2: CPU 168 MHz and VCO 336 MHz.
  PLLQ **7** supplies 48 MHz for the hardware RNG; SPI and peripheral bus clocks
  retain their original rates. The application platform module owns RNG.

## Regeneration is a reviewed operation

`activity_recognition/activity_recognition.ioc` is a peripheral configuration
reference. CubeMX generates ignored `activity_recognition/Core/`, `Drivers/`,
and Makefile files. Those files are not PlatformIO's active sources. Regenerating
the `.ioc` alone therefore does not update the compiled firmware.

Before regeneration, preserve a clean commit. The preparation helper creates
a **new** review directory outside the repository, copies the public `.ioc`,
seeds `Core/Src` with active `main.c`, `stm32f4xx_hal_msp.c` and
`stm32f4xx_it.c`, and seeds `Core/Inc` with `main.h` and `stm32f4xx_it.h`.
It records source hashes and includes no credentials. In PowerShell:

```powershell
$rtsparkReview = Join-Path $env:USERPROFILE ("Downloads\rtspark-cubemx-review-" + (Get-Date -Format "yyyyMMdd-HHmmss") + "\activity_recognition")
python firmware/scripts/prepare_cubemx_review.py --output "$rtsparkReview"
```

Open `$rtsparkReview\activity_recognition.ioc` in STM32CubeMX, keep user-code
preservation enabled, and generate into that review project. Preparation is
not generation. Keep this PowerShell window open so the variable is available.
After actual generation, run:

```powershell
python firmware/scripts/check_cubemx.py --generated "$rtsparkReview"
```

The read-only guard rejects malformed/duplicate USER CODE markers and any lost
nonempty active USER CODE block. It also checks the reviewed clock, bus and
interrupt settings. It never copies generated sources into the build.
With `--generated`, both source and candidate `.ioc` settings are checked.
The helper's four temporary-directory tests run with
`python -m unittest discover -s tests -p test_cubemx_review.py -v`.

Review generated code outside USER CODE blocks before accepting changes:

1. Keep `custom_bus.c` / `custom_bus.h` and `icm20608.c` / `icm20608.h` under
   application ownership. With the MEMS pack removed, CubeMX may generate a
   second `hi2c2` and `MX_I2C2_Init(void)`. Remove those generated main.c
   declarations, definition and call; the BSP initializes I2C2 when the sensor
   starts. Its `MX_I2C2_Init(I2C_HandleTypeDef *)` has a different signature.
2. Retain the RW007 boot-pin transitions, shared-flash deselection, foreground
   polling, and disabled PG11 NVIC. Generated GPIO defaults do not replace the
   reset routine's deliberate input/alternate-function changes.
3. Retain platform RNG/UTC/network initialization and the MQTT/IMU application
   hooks. Do not add a second Cube HAL RNG owner.
4. Manually copy only reviewed peripheral changes into `src/` / `include/`.
   Run the guard and `pio run -d firmware`, inspect the diff, then repeat the
   physical sensor/Wi-Fi/DHCP/TLS checks before accepting the regeneration.

The corrected USER CODE boundaries preserve the application loop and RW007
helpers during a seeded regeneration. Passing the guard verifies preservation,
not CubeMX compatibility or hardware behavior; CubeMX regeneration itself must
still be exercised with the installed CubeMX version.
