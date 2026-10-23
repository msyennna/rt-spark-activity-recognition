# Native TLS integration checks

Run `python tests/firmware_tls/test_tls.py` after the firmware dependency setup.
The runner uses a native GCC compiler (`CC` or MSYS2 UCRT64 GCC on Windows),
OpenSSL (`OPENSSL` or Git for Windows' OpenSSL), and Python's standard library.
It generates temporary RSA test certificates and keys, starts a TLS 1.2 server
on localhost, and compiles the actual pinned Mbed TLS 3.6.7 release with the
firmware's TLS configuration and 56 KiB allocator. No project credentials,
cloud data, physical board or firmware flashing are involved.

The tests require a verified ECDHE_RSA/AES-GCM handshake, at least one
`MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS`, and at least one network `WANT_*` return.
The test foreground counter advances between every handshake call. Separate
handshakes must reject an untrusted CA, a wrong hostname, a date before the
certificate's validity and a date after expiry with the corresponding X.509
verification flags.

The host supplies Windows system entropy and a test UTC clock. These checks
exercise the real TLS library and continuation behavior, but do not validate
the STM32 RNG peripheral, SNTP, RW007 transport, board RAM high-water marks or
physical polling latency. Those remain board acceptance checks.

## Local upstream adaptation

Mbed TLS 3.6.7 `library/ssl_tls12_client.c` enables the existing restartable
ECDH state paths only for `MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA`. The build script
also enables those paths for `MBEDTLS_KEY_EXCHANGE_ECDHE_RSA` in TLS 1.2. It
changes one condition; the ECDH implementation, peer verification, signature
algorithms and cipher operations remain upstream. RSA public-key signature
verification remains synchronous, while the expensive ephemeral EC operations
yield according to `mbedtls_ecp_set_max_ops(128)`.

The script verifies the official complete release archive SHA-256 before
extraction, then requires these exact whole-file hashes for the adaptation:

- Upstream: `eb169eb2c8fbacdd13833a69fb87cb150fd5b548079b50fd86d5e79245768341`
- Adapted: `95b35175ab531907482926725a0b10a1ab79a5d7469cd580ae1e41d9bd2e2054`

It rejects any other client source rather than applying a fuzzy patch. The
upstream source and its SPDX/license notices are preserved in the ignored
dependency cache. The release is licensed Apache-2.0 OR GPL-2.0-or-later; this
integration uses the Apache-2.0 option. Recheck this adaptation and the native
handshake tests when updating Mbed TLS.
