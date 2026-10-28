# Offline verification

Run commands from the repository root. The Python model/API tests require the
dependencies in `requirements.txt`; the native firmware suites require GCC or
Clang. These checks use mocks or temporary test data and do not program the
board or contact the deployed lab server.

## Python and CubeMX checks

```powershell
python -B firmware/scripts/check_cubemx.py
python -B -m unittest discover -s tests -v
python -B tools/evaluation/test_timed_walk_jog.py
```

| Suite | What it verifies |
| --- | --- |
| `test_temporal_model.py` | All-axis temporal learning, recording boundaries, date splits and checkpoint integrity |
| `test_cloud_temporal_contract.py` | Actual PyTorch inference through the MQTT callback, temporary SQLite and Flask responses |
| `test_cubemx_review.py` | Review-copy isolation, supplied generator fixture, peripheral ownership and retained application blocks |
| `test_model_update.py` | Service scope, rollback and fresh-record/matching-hash deployment gates using mocks |
| `tools/evaluation/test_timed_walk_jog.py` | Independent timing/ID selection, quality rules, errors and interrupted-session preservation |

The CubeMX guard validates the checked-in configuration and active source
blocks. It does not run the STM32CubeMX application; the actual generation
record is linked from the [evidence index](../docs/testing/README.md).

## Native firmware suites

```powershell
python -B tests/firmware_host/run.py
```

Five suites compile the actual C modules against host mocks: MQTT packets,
TCP/platform behavior, MQTT state transitions, bounded heap and ICM20608 driver.
See [firmware_host/README.md](firmware_host/README.md) for compiler setup and
coverage.

## Additional platform checks

The separate [TLS harness](firmware_tls/README.md) uses real Mbed TLS with a
local test server. Its five cases currently require Windows and are skipped
on other operating systems:

```powershell
python -B tests/firmware_tls/test_tls.py
```

An ARM build (`pio run -d firmware`) additionally requires PlatformIO and the
firmware dependencies. Android builds require Android Studio/SDK and the Gradle
toolchain. Actual sensor, network and phone observations remain in the
[dated evidence](../docs/testing/README.md); offline test success does not
replace those hardware results.
