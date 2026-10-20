"""Compile the installed STM32CubeF4 lwIP core without its RTOS port."""

from pathlib import Path

Import("env")

framework = env.PioPlatform().get_package_dir("framework-stm32cubef4")
if not framework:
    raise RuntimeError("Phase 1 requires the installed framework-stm32cubef4 package")

lwip_src = Path(framework) / "Middlewares" / "Third_Party" / "LwIP" / "src"
if not (lwip_src / "include" / "lwip" / "init.h").is_file():
    raise RuntimeError("The installed STM32CubeF4 package does not contain lwIP")

env.Append(CPPPATH=[str(lwip_src / "include")])
env.BuildSources(
    "$BUILD_DIR/LwIP",
    str(lwip_src),
    src_filter=[
        "-<*>",
        "+<core/*.c>",
        "+<core/ipv4/*.c>",
        "+<netif/ethernet.c>",
    ],
)
