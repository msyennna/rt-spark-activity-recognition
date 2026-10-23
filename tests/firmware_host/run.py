"""Compile and run the actual firmware C modules against host-side mocks.

No board, credentials, broker, database, or network access is used.
Set CC to a native GCC/Clang executable if one is not available on PATH.
"""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
compiler = os.environ.get("CC") or shutil.which("gcc") or shutil.which("clang")
if compiler is None and Path(r"C:\msys64\ucrt64\bin\gcc.exe").is_file():
    compiler = r"C:\msys64\ucrt64\bin\gcc.exe"
if compiler is None:
    raise SystemExit("Native C compiler required; set CC to GCC or Clang.")

environment = os.environ.copy()
environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
with tempfile.TemporaryDirectory(prefix="rtspark-host-tests-") as directory:
    for name, sources in (
        ("packets", [HERE / "test_packets.c", ROOT / "firmware/src/rtspark_mqtt_packet.c"]),
        ("platform", [HERE / "test_platform.c"]),
        ("mqtt_state", [HERE / "test_mqtt_state.c", ROOT / "firmware/src/rtspark_mqtt_packet.c"]),
        ("heap", [HERE / "test_heap.c"]),
        ("sensor", [HERE / "test_sensor.c", ROOT / "firmware/src/icm20608.c"]),
    ):
        executable = Path(directory) / (name + (".exe" if os.name == "nt" else ""))
        subprocess.run(
            [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
             "-I", str(HERE / "mocks"), "-I", str(ROOT / "firmware/include"),
             *map(str, sources), "-o", str(executable)],
            check=True, cwd=ROOT, env=environment,
        )
        subprocess.run([str(executable)], check=True, cwd=ROOT, env=environment)
