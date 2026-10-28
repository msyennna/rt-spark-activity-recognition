# Main-branch integration and repository review

Reviewed October 28, 2026 (Asia/Manila).

The completed project from `integration/stm32cubemx` at
`001f9c949b830933126a331bef4f8fa3d4cc7f5e` was integrated with the existing
`main` at `f9e97fba74cef4adaba301ee1f48bdc2e631dcdc`. Both histories are retained.

## Repository organization

- The root README now describes the complete firmware, PyTorch cloud pipeline
  and Android app, with saved app screenshots and the audited live result.
- Component guides and documentation/evidence indexes provide setup and review
  paths without moving the active source trees.
- The earlier main-branch Markdown and PDF reports are preserved byte for byte
  in `docs/reports/archive/`, with their historical status explained in
  [the report index](../reports/README.md).
- Current setup/alignment notes and the model metadata now reflect the completed
  six-trial evaluation. The checkpoint weights and application algorithms are
  unchanged from the integration branch.
- The Android Gradle wrapper is executable on Linux/macOS. Local worktrees,
  capture CSVs and generated Android packages are excluded from Git alongside
  the existing private-header, signing-key, database and firmware-build rules.

## Checks run for this integration

| Check | Result |
| --- | --- |
| CubeMX active-source/configuration guard | Passed |
| Native firmware tests | All five suites passed: packets, platform, MQTT state, heap and sensor |
| CubeMX review tests | 12 passed |
| Deployment-helper tests | 4 passed |
| Timed-evaluation helper tests | 8 passed |
| Relative documentation links and README image paths | Checked; all targets exist |
| Python source syntax | All 30 files parsed |
| PyTorch checkpoint SHA-256 | Matches the model metadata |
| Archived reports | Both match the original main-branch blobs exactly |

The separate native TLS harness ran with all five cases **skipped** because it
currently targets Windows. PyTorch/cloud model tests were not rerun here:
PyTorch, Flask and Paho-MQTT are absent from this environment. Full ARM firmware
and Android builds, board programming, live cloud checks and movement trials
were not repeated during repository organization. Their existing dated evidence
remains in [the evidence index](README.md).

Existing Windows transcript line endings and other inherited evidence formatting
are retained. This review does not claim a new hardware test or a change to the
audited **29/53 (54.7%)** live PyTorch result.
