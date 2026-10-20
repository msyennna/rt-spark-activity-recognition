#!/usr/bin/env python3
"""Flash Yena's newly built Wi-Fi MQTT firmware with verified 16 KB writes.

Uses Python's standard library and the installed STM32CubeProgrammer CLI.
The BIN must match the ELF beside it and contain the new publisher banner.
Run --dry-run to validate files and see the plan without connecting to a board.
"""

from __future__ import annotations

import argparse
from datetime import datetime
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile


TARGET_SERIAL = "0669FF535548867187064052"
FLASH_BASE = 0x08000000
FLASH_SIZE = 1024 * 1024
BLOCK_SIZE = 16 * 1024
SECTOR_SIZES = [16 * 1024] * 4 + [64 * 1024] + [128 * 1024] * 7
BANNER = b"[RTSpark] Direct Wi-Fi MQTT TLS publisher v1.0"
BROKER = b"ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
DEFAULT_FIRMWARE = (Path.home() / "rt-spark-activity-recognition" / "firmware"
                    / "activity_recognition" / "Debug" / "rtthread.bin")
DEFAULT_PROGRAMMER = (Path(os.environ.get("ProgramFiles", r"C:\Program Files"))
                      / "STMicroelectronics" / "STM32Cube" / "STM32CubeProgrammer"
                      / "bin" / "STM32_Programmer_CLI.exe")


class FlashError(RuntimeError):
    pass


def elf_flash_image(path):
    """Reconstruct flash bytes from ELF32 ARM load segments, without extra packages."""
    elf = Path(path).read_bytes()
    if len(elf) < 52 or elf[:7] != b"\x7fELF\x01\x01\x01":
        raise FlashError("Expected a little-endian ELF32 firmware file.")
    header = struct.unpack_from("<HHIIIIIHHHHHH", elf, 16)
    if header[0] != 2 or header[1] != 40 or header[2] != 1:
        raise FlashError("ELF is not a linked ARM firmware image.")
    entry, phoff, phsize, phcount = header[3], header[4], header[8], header[9]
    if phsize < 32 or not 1 <= phcount <= 64 or phoff < 52 or phoff + phsize * phcount > len(elf):
        raise FlashError("ELF program headers are invalid or truncated.")
    segments = []
    for index in range(phcount):
        kind, offset, virtual, address, size, memory_size, flags, alignment = struct.unpack_from(
            "<IIIIIIII", elf, phoff + index * phsize)
        if kind != 1 or size == 0:
            continue
        if size > memory_size or offset + size > len(elf):
            raise FlashError("ELF load segment is truncated or invalid.")
        if not FLASH_BASE <= address < FLASH_BASE + FLASH_SIZE or address + size > FLASH_BASE + FLASH_SIZE:
            raise FlashError("ELF contains initialized data outside the expected flash region.")
        segments.append((address, elf[offset:offset + size]))
    segments.sort()
    if not segments or segments[0][0] != FLASH_BASE:
        raise FlashError("ELF does not begin at the board's flash address.")
    end = max(address + len(data) for address, data in segments)
    image = bytearray(end - FLASH_BASE)
    previous_end = FLASH_BASE
    for address, data in segments:
        if address < previous_end:
            raise FlashError("ELF contains overlapping flash segments.")
        image[address - FLASH_BASE:address - FLASH_BASE + len(data)] = data
        previous_end = address + len(data)
    return bytes(image), entry


def validated_image(binary_path, elf_path):
    data = Path(binary_path).read_bytes()
    if not 128 <= len(data) <= FLASH_SIZE:
        raise FlashError("The binary is empty, truncated, or larger than 1 MB.")
    expected, entry = elf_flash_image(elf_path)
    if data != expected:
        raise FlashError("BIN and ELF do not match. Clean and rebuild the project before flashing.")
    initial_sp, reset_handler = struct.unpack_from("<II", data)
    if not (0x20000000 < initial_sp <= 0x20020000 and initial_sp % 8 == 0):
        raise FlashError("Firmware has an unexpected initial stack pointer.")
    if not (reset_handler & 1 and FLASH_BASE <= (reset_handler & ~1) < FLASH_BASE + len(data)):
        raise FlashError("Firmware has an unexpected reset vector.")
    if (entry & ~1) != (reset_handler & ~1):
        raise FlashError("ELF entry point does not match the reset vector.")
    if BANNER not in data or BROKER not in data or b"activity/sensor" not in data:
        raise FlashError("This is not the new Wi-Fi MQTT publisher build. Check the Debug folder.")
    return data


def firmware_sectors(length):
    if not 0 < length <= FLASH_SIZE:
        raise FlashError("Firmware does not fit the board's flash region.")
    sectors, offset = [], 0
    for number, size in enumerate(SECTOR_SIZES):
        if offset < length:
            sectors.append(number)
        offset += size
    return sectors


def blocks(data):
    for offset in range(0, len(data), BLOCK_SIZE):
        yield FLASH_BASE + offset, data[offset:offset + BLOCK_SIZE]


def check_target(output):
    device = re.search(r"(?im)^\s*Device ID\s*:\s*0x([0-9a-f]+)", output)
    serial = re.search(r"(?im)^\s*ST-LINK SN\s*:\s*(\S+)", output)
    capacity = re.search(r"(?im)^\s*0x1FFF7A20\s*:\s*(?:0x)?([0-9a-f]{8})\b", output)
    if not device or int(device.group(1), 16) != 0x413:
        raise FlashError("The board did not identify as the expected STM32F407 family.")
    if not serial or serial.group(1) != TARGET_SERIAL:
        raise FlashError("ST-LINK serial does not match your previously verified board.")
    if not capacity or ((int(capacity.group(1), 16) >> 16) & 0xFFFF) != 1024:
        raise FlashError("The board did not report the expected 1024 KB flash capacity.")


class Programmer:
    def __init__(self, executable, frequency, log):
        self.executable, self.frequency, self.log = str(executable), frequency, log

    def run(self, stage, operation):
        command = [self.executable, "-c", "port=SWD", "mode=UR", "reset=HWrst",
                   "freq={}".format(self.frequency), "sn=" + TARGET_SERIAL] + list(operation)
        self.log.write("\n=== {} ===\n{}\n".format(stage, subprocess.list2cmdline(command)))
        self.log.flush()
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, encoding="utf-8", errors="replace", timeout=120,
                                    shell=False)
        except subprocess.TimeoutExpired as error:
            output = error.stdout or b""
            if isinstance(output, bytes):
                output = output.decode("utf-8", errors="replace")
            self.log.write(output + "\nTIMEOUT\n")
            self.log.flush()
            raise FlashError(stage + " timed out; no later steps were run.") from error
        self.log.write(result.stdout + "\nExit code: {}\n".format(result.returncode))
        self.log.flush()
        if result.returncode != 0 or "error:" in result.stdout.lower():
            print("\n" + "\n".join(result.stdout.splitlines()[-24:]), flush=True)
            raise FlashError(stage + " failed; no later steps were run.")
        return result.stdout


def execute(args, data, logfile):
    programmer = Programmer(args.programmer, args.frequency, logfile)
    print("Checking your board...", flush=True)
    check_target(programmer.run("Check target", ["-r32", "0x1FFF7A20", "0x4"]))
    sectors = firmware_sectors(len(data))
    print("Board matched. Erasing sectors {} once...".format(
        ", ".join(map(str, sectors))), flush=True)
    programmer.run("Erase firmware sectors", ["-e"] + list(map(str, sectors)))
    all_blocks = list(blocks(data))
    with tempfile.TemporaryDirectory(prefix="rtspark_wifi_flash_") as folder:
        folder = Path(folder)
        for index, (address, block) in enumerate(all_blocks, 1):
            print("Block {}/{} at 0x{:08X} ({} bytes)...".format(
                index, len(all_blocks), address, len(block)), flush=True)
            part = folder / "block_{:02d}.bin".format(index)
            part.write_bytes(block)
            programmer.run("Program and verify block {}/{}".format(index, len(all_blocks)),
                           ["--skipErase", "-d", str(part), "0x{:08X}".format(address), "-v"])
            print("  Verified.", flush=True)
        print("Reading back the entire firmware for an independent byte comparison...", flush=True)
        readback = folder / "readback.bin"
        programmer.run("Read back complete firmware",
                       ["--upload", "0x{:08X}".format(FLASH_BASE), str(len(data)), str(readback)])
        if not readback.is_file():
            raise FlashError("The programmer did not create the readback file.")
        actual = readback.read_bytes()
        if actual != data:
            mismatch = next((i for i, (a, b) in enumerate(zip(actual, data)) if a != b),
                            min(len(actual), len(data)))
            raise FlashError("Full readback differs near 0x{:08X}.".format(FLASH_BASE + mismatch))
    logfile.write("\nSUCCESS: full firmware readback MATCHES.\n")
    print("\nSUCCESS: all {} blocks programmed; full firmware readback MATCHES.".format(
        len(all_blocks)), flush=True)
    print("Open miniterm on COM3, then press the board's RESET button once.", flush=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", type=Path, default=DEFAULT_FIRMWARE)
    parser.add_argument("--elf", type=Path, help="Default: the ELF beside the binary.")
    parser.add_argument("--programmer", type=Path, default=DEFAULT_PROGRAMMER)
    parser.add_argument("--frequency", type=int, choices=[125, 950], default=950)
    parser.add_argument("--log", type=Path, default=Path(__file__).resolve().with_name("rtspark-wifi-flash.log"))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)
    try:
        data = validated_image(args.firmware, args.elf or args.firmware.with_suffix(".elf"))
        digest = hashlib.sha256(data).hexdigest()
        print("Firmware: {}".format(args.firmware.resolve()), flush=True)
        print("BIN/ELF match; Wi-Fi publisher found. {} bytes; SHA-256 {}".format(len(data), digest), flush=True)
        print("Target ST-LINK: {}; speed: {} kHz; block size: 16 KB.".format(
            TARGET_SERIAL, args.frequency), flush=True)
        if args.dry_run:
            print("DRY RUN: no programmer connection or flash writes.")
            print("Erase sectors:", ", ".join(map(str, firmware_sectors(len(data)))))
            print("Write {} verified blocks; compare every firmware byte afterward.".format(
                len(list(blocks(data)))))
            return 0
        if not args.programmer.is_file():
            raise FlashError("STM32CubeProgrammer was not found at {}.".format(args.programmer))
        print("Keep the board connected; do not press RESET while programming.", flush=True)
        with args.log.open("w", encoding="utf-8") as logfile:
            logfile.write("RT-Spark Wi-Fi publisher flash\nStarted: {}\nFirmware SHA-256: {}\n".format(
                datetime.now().astimezone().isoformat(), digest))
            print("Diagnostic log: {}".format(args.log.resolve()), flush=True)
            try:
                execute(args, data, logfile)
            except (FlashError, OSError, KeyboardInterrupt) as error:
                message = "Interrupted by Ctrl+C." if isinstance(error, KeyboardInterrupt) else str(error)
                logfile.write("\nSTOPPED: {}\n".format(message))
                print("\nSTOPPED: {}".format(message), file=sys.stderr, flush=True)
                print("Send the terminal output or rtspark-wifi-flash.log for review.", file=sys.stderr, flush=True)
                return 1
        return 0
    except (FlashError, OSError) as error:
        print("STOPPED before programming: {}".format(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
