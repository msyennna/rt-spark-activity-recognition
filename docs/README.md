# Project documentation

The complete firmware, cloud and Android implementation is available on `main`.
The laboratory implementation and planned six-trial evaluation are complete.
The final audited live PyTorch score is **29/53 (54.7%)**, with weak jogging
recognition documented as a limitation.

## Read first

| Guide | Purpose |
| --- | --- |
| [Project overview](../README.md) | Features, app screenshots, repository layout and recorded results |
| [Completion review](LAB_COMPLETION.md) | Current completion status and the evidence supporting it |
| [Setup guide](RTSPARK_SETUP.md) | Local configuration, firmware build and device-to-phone setup |
| [Firmware guide](../firmware/README.md) | Active sources, CubeMX review and ST-LINK programming |
| [Cloud guide](../cloud/README.md) | MQTT subscriber, model, database and REST API |
| [Android guide](../android_app/README.md) | Android Studio setup, server hostname and trust configuration |

## Implementation and evaluation

| Reference | Purpose |
| --- | --- |
| [Model methodology](../cloud/classifier/README.md) | Dataset, units, temporal model, splits and checkpoint integrity |
| [Hardware/toolchain notes](LAB_COMPLETION.md#hardwaretoolchain-reference-differences) | Actual-board adaptation and reference-toolchain differences |
| [Firmware integration and verification](../firmware/README.md#integration-verification) | Publisher contract, firmware constraints and device-to-phone verification |
| [Deployment update runbook](../cloud/README.md#update-an-existing-server) | Existing server model update; firmware validation remains in the firmware guide |
| [Test guide](../tests/README.md) | Commands and prerequisites for offline verification |
| [Evidence index](testing/README.md) | Selected builds, uploads, cloud/phone checks and movement results |
| [Timed evaluation procedure](testing/TIMED_VALIDATION.md) | Independently timed movement-trial method |
| [Earlier report archive](reports/README.md) | Original Markdown/PDF reports preserved from the earlier `main` |

Dated testing files are records of individual checkpoints. Some correctly
describe work that was still pending at that time; use
[LAB_COMPLETION.md](LAB_COMPLETION.md) for the final completion boundary.
Additional engineering checks in the integration history are separate from
the completed core laboratory implementation and evaluation.
