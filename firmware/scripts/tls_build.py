"""Acquire the pinned complete Mbed TLS release and compile its bare-metal client.

Only the official release archive is accepted, checked before extraction. The
cache is ignored; a clean clone downloads it once during `pio run -d firmware`.
"""

import hashlib
import tarfile
import urllib.request
from pathlib import Path, PurePosixPath

Import("env")

VERSION = "3.6.7"
SHA256 = "a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6"
URL = (
    "https://github.com/Mbed-TLS/mbedtls/releases/download/"
    f"mbedtls-{VERSION}/mbedtls-{VERSION}.tar.bz2"
)
project = Path(env.subst("$PROJECT_DIR"))
cache = project / ".deps"
archive = cache / f"mbedtls-{VERSION}.tar.bz2"
source = cache / f"mbedtls-{VERSION}"
marker = source / ".rtspark-verified-sha256"
cache.mkdir(exist_ok=True)


def digest(path):
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


if not archive.is_file():
    print(f"Downloading pinned Mbed TLS {VERSION} release...")
    pending = archive.with_suffix(".download")
    request = urllib.request.Request(URL, headers={"User-Agent": "RTSpark-PlatformIO/1"})
    with urllib.request.urlopen(request, timeout=60) as response, pending.open("wb") as output:
        while True:
            block = response.read(1024 * 1024)
            if not block:
                break
            output.write(block)
    if digest(pending) != SHA256:
        pending.unlink()
        raise RuntimeError("Mbed TLS download SHA-256 mismatch; refusing to build")
    pending.replace(archive)

if digest(archive) != SHA256:
    raise RuntimeError(f"Mbed TLS archive SHA-256 mismatch: remove {archive} and retry")

if not marker.is_file() or marker.read_text(encoding="ascii").strip() != SHA256:
    # Do not trust host-dependent tar extraction rules, links, devices or paths.
    with tarfile.open(archive, "r:bz2") as release:
        members = release.getmembers()
        for member in members:
            parts = PurePosixPath(member.name).parts
            if (
                not parts
                or parts[0] != source.name
                or ".." in parts
                or "\\" in member.name
                or ":" in member.name
                or not (member.isdir() or member.isfile())
            ):
                raise RuntimeError(f"Unsafe path or entry in Mbed TLS archive: {member.name}")
            target = cache.joinpath(*parts).resolve()
            if not target.is_relative_to(source.resolve()):
                raise RuntimeError("Mbed TLS extraction escaped its cache directory")
        # Every entry is now a regular file or directory under the checked root.
        release.extractall(cache, members=members)
    marker.write_text(SHA256 + "\n", encoding="ascii")

build_info = source / "include" / "mbedtls" / "build_info.h"
if not build_info.is_file() or "0x03060700" not in build_info.read_text(encoding="utf-8"):
    raise RuntimeError("Mbed TLS cache version mismatch")
if not (source / "library" / "psa_crypto_driver_wrappers_no_static.c").is_file():
    raise RuntimeError("Mbed TLS archive lacks generated sources; use the complete release")

# Upstream restarts ECDH only for ECDHE_ECDSA, although the ECDH continuation
# paths are also usable after RSA signature verification. Enable those paths
# for ECDHE_RSA so the foreground SPI/lwIP poll runs between ECC slices. RSA
# public-key signature checks remain synchronous. See tests/firmware_tls/README.md.
# Match whole-file hashes before/after; never silently patch a different release.
client = source / "library" / "ssl_tls12_client.c"
original_hash = "eb169eb2c8fbacdd13833a69fb87cb150fd5b548079b50fd86d5e79245768341"
patched_hash = "95b35175ab531907482926725a0b10a1ab79a5d7469cd580ae1e41d9bd2e2054"
client_hash = digest(client)
if client_hash == original_hash:
    old = b"if (suite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA &&"
    new = (
        b"if ((suite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA ||\n"
        b"         suite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_RSA) &&"
    )
    upstream = client.read_bytes()
    if upstream.count(old) != 1:
        raise RuntimeError("Mbed TLS ECDHE_RSA continuation patch did not match exactly")
    patched = upstream.replace(old, new)
    if hashlib.sha256(patched).hexdigest() != patched_hash:
        raise RuntimeError("Mbed TLS ECDHE_RSA continuation patch hash mismatch")
    client.write_bytes(patched)
elif client_hash != patched_hash:
    raise RuntimeError("Mbed TLS TLS1.2 client source changed; refusing unverified patch")

env.Append(
    CPPPATH=[str(source / "include"), str(source / "library")],
    CPPDEFINES=[("MBEDTLS_CONFIG_FILE", '\\"rtspark_mbedtls_config.h\\"')],
)
env.BuildSources("$BUILD_DIR/mbedTLS", str(source / "library"), src_filter=["+<*.c>"])
