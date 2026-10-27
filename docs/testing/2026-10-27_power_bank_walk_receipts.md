# Follow-up broker receipt check

Reviewed October 27, 2026 (Asia/Manila). This note retains aggregate findings
only; raw terminal output and infrastructure identifiers are excluded.

## Context and method

The user supplied this capture in response to instructions to power the board
from its power bank, retain the same Wi-Fi, and walk normally near the access
point. Those physical conditions were requested but are not independently
measured by the receipt timestamps.

The broker subscriber requested 70 non-retained sensor messages, printed
fractional Unix receipt timestamps, and used a 90-second timeout. The pasted
output contains 70 timestamp rows followed by the shell prompt, with no timeout
or error shown. It follows the [extended diagnostic publishing
review](2026-10-27_mqtt_diagnostic_continuation.md).

## Results

Intervals were calculated from all consecutive fractional Unix timestamps.
Integer-nanosecond calculations and an independent decimal calculation agreed.

| Measurement | Result |
| --- | ---: |
| Requested / observed receipt rows | 70 / 70 |
| Consecutive intervals | 69 |
| First-to-last receipt span | 75.224140836 s |
| Minimum interval | 0.984525643 s |
| Median interval | 1.001840989 s |
| Maximum interval | 2.016112826 s |
| Intervals at most 1.5 s | 63 |
| Intervals greater than 1.5 s and at most 2.5 s | 6 |
| Intervals greater than 2.5 s | 0 |

**Outcome: count complete, with no observed broker receipt interval above
the classifier's 2.5-second gap threshold in this capture.** Six intervals
were approximately two seconds, so this is not a claim of uninterrupted
one-second delivery.

The span above is between the first and last receipt, not the full command
runtime. Receipt timestamps do not identify sensor acquisition times, exact
missing-sample counts or duplicate payloads. The classifier independently
checks its own monotonic arrival times; these subscriber timestamps alone
do not prove that it produced fresh predictions throughout the capture.

## Next step and remaining work

The result supports proceeding to the existing prediction-freshness preflight
and, if it passes, a new complete timed PyTorch evaluation. Retain the same
placement, power source and Wi-Fi throughout the session; follow all six
walking/jogging intervals and keep predictions hidden until completion.

The prior stopped trial remains invalid for scoring and is preserved. This
receipt check supplies no activity ground truth or accuracy score. Full valid
six-trial PyTorch movement evaluation remains pending, and the earlier
interruption's physical cause is not established by this short successful
capture.

No firmware, model, credentials, cloud configuration, temporal gap guard or
scoring rules were changed for this review. No rebuild or flash is required
to perform the next test.
