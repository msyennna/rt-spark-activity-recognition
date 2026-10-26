# CubeMX 6.18.1 generated-file review

Reviewed October 27, 2026 (Asia/Manila). The user generated a separate review
project and supplied `rtspark-cubemx-generated-review.zip` after the old guard
reported EXTI and duplicate-I2C2 errors. This review inspects that actual output;
the reviewer did not run the CubeMX GUI or flash a board.

## Provenance and result

The original public archive is preserved byte-for-byte as
[`tests/fixtures/cubemx_6_18_1_review.zip`](../../tests/fixtures/cubemx_6_18_1_review.zip).
Its SHA-256 is
`fd6e2664658a419215e7fd823d55d2c46adba9280136a8eae8fae3e8cfa75340`.
The `.ioc` identifies CubeMX **6.18.1**, database **DB.6.0.181**, and
STM32Cube F4 **V1.28.3**. The eleven files comprise the `.ioc`, seed manifest,
and nine public Core C/header files. No private header or firmware image was
included. Archive paths were checked before extraction.

The six seed-file hashes match baseline `02bf2e1` after accounting for the
Windows copies' CRLF line endings. All **17 nonempty baseline USER CODE
blocks** are retained in the generated output and accepted sources. The original
seed manifest still correctly says its preparation helper did not generate;
that field describes the seed step, not the later user-run generation.
[The audit JSON](2026-10-27_cubemx_generation_review.json) records raw archive,
seed, baseline and accepted-file hashes.

**Source reconciliation is complete.** The accepted `.ioc`, `main.c`, `main.h`
and interrupt-source changes are now in PlatformIO's active tree. Reconciled
MSP and interrupt header match their previous active contents. No generated
support files or private configuration were imported.

## Findings and accepted differences

| Area | Actual generated difference | Resolution |
| --- | --- | --- |
| EXTI15_10 | CubeMX omitted the previously explicit disabled NVIC key, removed the unused handler/alias, and emitted no NVIC-enable call. | This was a false positive in the old guard, not evidence that CubeMX enabled the interrupt. Accept omission or an explicit disabled value; reject enabled/malformed values and actual enable calls in main/MSP. Foreground polling is retained. |
| Sensor bus | Added `hi2c2`, `MX_I2C2_Init(void)` declaration, call and body in main, plus HAL I2C MSP init/deinit functions. | Remove all those generated duplicates. Retain `custom_bus.c/h` byte-for-byte, including its differently typed initializer and BSP-owned PF0/PF1 setup at 100 kHz. |
| GPIO output ordering | Generated HIGH latch writes before switching RW007 CS/reset and shared-flash CS to output mode. | Accept this ordering. PF10 and PG15 remain low-speed push-pull, PB12 high-speed push-pull, all with no pull. PB12 uses the new generated `SPI_FLASH_CS` symbols. Electrical behavior still needs the physical regression check. |
| Clock, SPI and UART | Formatting/order changes, with matching hardware values. | Retain 8 MHz HSE, PLLM4/PLLN168/PLLP2/PLLQ7, 168 MHz core/48 MHz RNG domain, SPI2 prescaler8, SPI2/SPI3/UART settings and their MSP setup. |
| GPIO input/interrupt | PG11 remains falling-edge EXTI with pulldown and no enabled NVIC. SPI3 and SysTick handlers remain. | Accept disabled-EXTI cleanup; the existing RW007 reset/poll logic remains in preserved application blocks. |
| `.ioc` serialization | Reordered pin indices and GPIO parameter lists; recorded the generated I2C2 function. | Accept serialization changes without assigning new pin signals. I2C2 remains configured for documentation but BSP-owned in the compiled source. |
| Framework support | Generated HAL configuration, syscalls, sysmem and system source. | Keep these as review evidence only. PlatformIO/framework retain those responsibilities; do not copy them into the active source tree. |

The raw external review directory and immutable test ZIP intentionally retain
CubeMX's I2C2 additions. The updated guard therefore still rejects that raw
copy until it is reconciled; pulling this commit updates the accepted active
firmware rather than rewriting a separate folder in Downloads.

## Validation

- Active configuration/source guard: PASS.
- Guard on the separately reconciled Core review copy: PASS.
- Twelve review regression tests: PASS, including actual archived output,
  application preservation, enabled-NVIC rejection, code-level NVIC checks,
  duplicate main/MSP I2C ownership, lost application code and conflicting clocks.
- Five actual native firmware suites: PASS (packets, platform, MQTT state, heap,
  sensor). These do not execute physical GPIO initialization.
- Incremental ARM firmware build: **PASS**, static SRAM **66,756 / 131,072**
  bytes, separate CCM **57,344** bytes, flash **162,468 / 1,048,576** bytes.
  The complete [build log](2026-10-27_cubemx_arm_build.txt) is retained. The
  493.00-second elapsed time includes restoring pinned dependencies. This is
  a build in the review environment, not a new build on the user's PC.

The build used GCC 12.3.1, STM32Cube F4 1.28.3 and coreMQTT 2.3.1. PlatformIO
rejected checksum-mismatched package downloads and one timed-out mirror, then
installed verified packages from alternate mirrors; no integrity check was
bypassed. The eight existing libnosys syscall-stub warnings and RWX load-segment
warning remain. They did not prevent linking, and are not represented as fixed.
No new TLS cryptographic or live-service tests were run for this GPIO/source
reconciliation; those modules and the cloud/Android/model files are unchanged.

Optional PlatformIO telemetry is disabled for the ARM build. Public-default
configuration is used in the review environment; a local configured rebuild is
required before programming. No compiled image is published.

## Next steps on the user's Windows computer

In the integration worktree, pull `integration/stm32cubemx`, run the guard
**without `--generated`** against the accepted active sources, and rebuild with
the existing ignored configuration. No additional CubeMX run is needed for
this reviewed output. Once that build succeeds, program the resulting local
image using the reviewed board workflow. Capture the actual coreMQTT startup
banner, verified TLS and PUBACKs, fresh database/API rows and the phone view.
Then perform a new independently timed walking/jogging session for PyTorch.
Earlier Random Forest trials remain historical evidence.

For a future generation, use a new seeded review directory and repeat the diff.
STM32CubeMX's **Project Manager → Advanced Settings → Driver selector → Generate
code** option can disable generation for an application-owned peripheral. The
separate option to omit a function call alone does not remove all definitions.
See [ST's CubeMX 6.18.1 manual, Advanced Settings](https://dev.st.com/stm32cube-docs/stm32cubemx/6.18.1/en/docs/markup/CubeMX_UserManual/chapters/04_4_stm32cubemx_user_interface.html).
Any changed generator settings still require reviewing the next actual output.
