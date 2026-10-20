"""Reapply the reviewed mbedTLS 2.7.10.1 port fixes after package download.

Does not read passwords, alter board applications, build, or flash firmware.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import tempfile

ORIGINAL_HARDWARE = "62d24b96d99f303afe56ebc713d3687d9489b7c96430987322266c4fbcfd7dbe"


def normalized(data: bytes) -> bytes:
    return data.replace(b"\r\n", b"\n")


def changes(project: Path) -> dict[Path, bytes]:
    tls = project / "packages/mbedtls-v2.7.10.1"
    replacement = Path(__file__).with_name("tls_hardware.c").read_bytes()
    hardware = tls / "ports/src/tls_hardware.c"
    if hashlib.sha256(normalized(hardware.read_bytes())).hexdigest() not in (
        ORIGINAL_HARDWARE, hashlib.sha256(normalized(replacement)).hexdigest()
    ):
        raise ValueError("The hardware adapter differs from both reviewed versions; it was preserved.")
    config_path = tls / "ports/inc/tls_config.h"
    config = config_path.read_text(encoding="utf-8-sig")
    for macro in ("MBEDTLS_HAVE_TIME_DATE", "MBEDTLS_PLATFORM_TIME_ALT"):
        pattern = rf"(?m)^[ \t]*(?://[ \t]*)?#define[ \t]+{macro}[ \t]*$"
        if len(re.findall(pattern, config)) != 1:
            raise ValueError("The TLS configuration has an unexpected definition of " + macro)
        config = re.sub(pattern, "#define " + macro, config)
    net_path = tls / "ports/src/tls_net.c"
    net = net_path.read_text(encoding="utf-8-sig")
    marker = "/* RTSpark: POSIX declarations required by the RT-Thread TLS port. */"
    if marker not in net:
        anchor = "#include <rtthread.h>"
        if anchor not in net:
            raise ValueError("Unexpected TLS network source; no files changed.")
        net = net.replace(anchor, anchor + "\n" + marker + "\n#include <errno.h>\n#include <fcntl.h>\n#include <sys/ioctl.h>\n", 1)
    return {
        hardware: replacement,
        config_path: config.encode("utf-8"),
        tls / "mbedtls/include/mbedtls/config.h": config.encode("utf-8"),
        net_path: net.encode("utf-8"),
    }


def apply(project: Path, updates: dict[Path, bytes]) -> Path:
    original = {path: path.read_bytes() if path.exists() else None for path in updates}
    backup = Path(tempfile.mkdtemp(prefix="rtspark-mbedtls-backup-"))
    written = []
    try:
        for path, content in updates.items():
            if original[path] == content:
                continue
            if original[path] is not None:
                copy = backup / path.relative_to(project)
                copy.parent.mkdir(parents=True, exist_ok=True)
                copy.write_bytes(original[path])
            path.parent.mkdir(parents=True, exist_ok=True)
            written.append(path)
            path.write_bytes(content)
    except Exception:
        for path in reversed(written):
            if original[path] is None:
                path.unlink(missing_ok=True)
            else:
                path.write_bytes(original[path])
        raise
    return backup


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[2] / "firmware/activity_recognition")
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    try:
        project = args.project.resolve()
        updates = changes(project)
        if args.check_only:
            pending = sum(path.read_bytes() != content for path, content in updates.items())
            print(f"PASS: reviewed patch anchors; {pending} file(s) need updating.")
        else:
            backup = apply(project, updates)
            print(f"PASS: mbedTLS port fixes applied. Original files: {backup}")
        return 0
    except (OSError, ValueError) as error:
        print("STOPPED: " + str(error))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
