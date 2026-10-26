"""Read-only guard for the reviewed CubeMX configuration and active sources.

CubeMX's ignored Core/ output is never copied into src/ or include/. Optionally
check a regenerated project before manually accepting its peripheral changes.
No source contents or configuration secrets are printed.
"""

import argparse
from pathlib import Path
import re
import sys


FIRMWARE = Path(__file__).resolve().parents[1]
CORE_FILES = {
    "src/main.c": "Core/Src/main.c",
    "src/stm32f4xx_hal_msp.c": "Core/Src/stm32f4xx_hal_msp.c",
    "src/stm32f4xx_it.c": "Core/Src/stm32f4xx_it.c",
    "include/main.h": "Core/Inc/main.h",
    "include/stm32f4xx_it.h": "Core/Inc/stm32f4xx_it.h",
}
MARKER = re.compile(r"/\* USER CODE (BEGIN|END) ([^*]+?) \*/")


def user_blocks(text, label):
    blocks = {}
    opened = None
    for match in MARKER.finditer(text):
        direction, name = match.group(1), match.group(2)
        if direction == "BEGIN":
            if opened or name in blocks:
                raise ValueError(f"{label}: nested/duplicate USER CODE block {name}")
            opened = (name, match.end())
        else:
            if not opened or opened[0] != name:
                raise ValueError(f"{label}: unmatched USER CODE END {name}")
            blocks[name] = text[opened[1]:match.start()].strip()
            opened = None
    if opened:
        raise ValueError(f"{label}: missing USER CODE END {opened[0]}")
    return blocks


def check(generated=None):
    errors = []
    expected = {
        "Mcu.CPN": "STM32F407ZGT6",
        "Mcu.ThirdPartyNb": "0",
        "PF0.Signal": "I2C2_SDA",
        "PF1.Signal": "I2C2_SCL",
        "I2C2.ClockSpeed": "100000",
        "RCC.HSE_VALUE": "8000000",
        "RCC.PLLM": "4",
        "RCC.PLLN": "168",
        "RCC.PLLQ": "7",
        "RCC.48MHZClocksFreq_Value": "48000000",
        "RCC.PLLQCLKFreq_Value": "48000000",
        "SPI2.BaudRatePrescaler": "SPI_BAUDRATEPRESCALER_8",
        "PB12.Signal": "GPIO_Output",
        "PB12.PinState": "GPIO_PIN_SET",
        "PF10.PinState": "GPIO_PIN_SET",
        "PG15.PinState": "GPIO_PIN_SET",
        "PG11.GPIO_ModeDefaultEXTI": "GPIO_MODE_IT_FALLING",
        "ProjectManager.KeepUserCode": "true",
    }
    configurations = [(FIRMWARE / "activity_recognition/activity_recognition.ioc", ".ioc")]
    if generated is not None:
        configurations.append((generated / "activity_recognition.ioc", "generated .ioc"))
    for ioc_path, label in configurations:
        try:
            ioc_text = ioc_path.read_text()
        except OSError as exc:
            errors.append(f"{label}: could not read configuration: {exc}")
            continue
        config = dict(line.split("=", 1) for line in ioc_text.splitlines()
                      if "=" in line and not line.startswith("#"))
        for key, value in expected.items():
            if config.get(key) != value:
                errors.append(f"{label}: {key} must be {value}")
        if not config.get("NVIC.EXTI15_10_IRQn", "").startswith("false\\:"):
            errors.append(f"{label}: EXTI15_10 NVIC must stay disabled for foreground polling")
        if "LSM6DSL" in ioc_text or "X-CUBE-MEMS1" in ioc_text:
            errors.append(f"{label}: stale LSM6DSL/MEMS pack selection must be removed")

    main = (FIRMWARE / "src/main.c").read_text()
    for name, expected_value in (("PLLM", "4"), ("PLLN", "168"), ("PLLQ", "7")):
        if not re.search(rf"PLL\.{name}\s*=\s*{expected_value}\s*;", main):
            errors.append(f"src/main.c: compiled {name} differs from .ioc")
    if not re.search(r"hspi2\.Init\.BaudRatePrescaler\s*=\s*SPI_BAUDRATEPRESCALER_8\s*;", main):
        errors.append("src/main.c: compiled SPI2 prescaler differs from .ioc")
    bus = (FIRMWARE / "src/custom_bus.c").read_text()
    if not re.search(r"hi2c->Init\.ClockSpeed\s*=\s*100000\s*;", bus):
        errors.append("src/custom_bus.c: reviewed I2C2 speed must stay 100 kHz")
    if re.search(r"\bMX_I2C2_Init\s*\(", main) or re.search(r"I2C_HandleTypeDef\s+hi2c2\s*;", main):
        errors.append("src/main.c: custom_bus.c must retain sole ownership of I2C2 initialization")

    for active_relative, generated_relative in CORE_FILES.items():
        active = FIRMWARE / active_relative
        try:
            active_blocks = user_blocks(active.read_text(), active_relative)
            if generated is not None:
                candidate = generated / generated_relative
                if not candidate.is_file():
                    errors.append(f"generated project is missing {generated_relative}")
                    continue
                candidate_text = candidate.read_text()
                candidate_blocks = user_blocks(candidate_text, generated_relative)
                for name, body in active_blocks.items():
                    if body and candidate_blocks.get(name) != body:
                        errors.append(f"{generated_relative}: active USER CODE {name} was changed/lost")
                if active_relative == "src/main.c" and (
                    re.search(r"\bMX_I2C2_Init\s*\(", candidate_text) or
                    re.search(r"I2C_HandleTypeDef\s+hi2c2\s*;", candidate_text)
                ):
                    errors.append("generated main.c: remove Cube's duplicate I2C2 handle/init; retain custom_bus.c")
        except ValueError as exc:
            errors.append(str(exc))
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generated", type=Path,
                        help="review a CubeMX project directory containing Core/Src and Core/Inc")
    args = parser.parse_args()
    try:
        errors = check(args.generated.resolve() if args.generated else None)
    except (OSError, ValueError) as exc:
        print(f"CubeMX review failed: {exc}", file=sys.stderr)
        return 1
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print("CubeMX configuration and active USER CODE blocks: OK")
    if args.generated:
        print("Candidate USER CODE retained. Review non-user peripheral changes manually; no files were copied.")
    else:
        print("PlatformIO compiles src/ and include/; ignored CubeMX Core/ output is not built.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
