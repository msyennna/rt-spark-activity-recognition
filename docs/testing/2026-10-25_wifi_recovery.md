# October 25 cloud reachability and Wi-Fi recovery checkpoint

## Evidence and scope

This report records user-supplied Windows, Ubuntu and serial-monitor output
from October 25, 2026. The reviewer did not operate the board or rerun the
physical checks. The public checker output includes its UTC timestamp; the
later serial excerpts have no wall-clock timestamp.

The repository source baseline inspected for this checkpoint is
`02fbbbd793f7f4ab13bb85c4cfa93a2367f5a543`. The flashed binary hash, private
configuration and a complete build/upload transcript were not supplied.
No firmware source, service configuration or credentials are changed by this
documentation update. SSIDs, access-point identifiers, local addresses and
passwords are omitted.

## Public cloud checks

The supplied read-only checker ran at `2026-10-25T14:55:32.608188Z`.

| Check | Supplied result | Limit |
| --- | --- | --- |
| MQTT TLS 8883 | PASS; TLSv1.2; ECDHE-RSA-AES256-GCM-SHA384 | MQTT application bytes sent: 0; no publisher login or sensor delivery tested |
| HTTPS latest | PASS; HTTP 200; schema valid; record ID 3987 | Record timestamp was `2026-10-23T09:50:54Z`, older than the check |
| HTTPS history, limit 1 | PASS; HTTP 200; schema valid; one record, ID 3987 | Same stale record; no current board-originated database update demonstrated |

The checker explicitly returned `firmware_database_updates_verified: false`.
Reachability and schema passes do not establish prediction freshness.

## Restart and data-flow observations

The user transferred the existing timed helper to Ubuntu and reconnected by SSH.
Both the preflight and a subsequent attempt to start the timer stopped with:

```text
STOPPED: Latest prediction is older than 15 seconds. Power the board and wait for fresh predictions.
```

Those attempts stopped before a timed trial began. They produce no movement
accuracy result.

A local broker subscription also returned `Timed out`:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
```

An earlier board reset log completed a scan of three access points but reported
`RW007 JOIN: TARGET SSID NOT FOUND`, followed by JOIN TEST FAILED.
ICM20608 initialization and six-field sensor JSON output continued.
This locates the observed board failure at Wi-Fi discovery/association;
it does not demonstrate a password rejection or an AWS failure.

## Latest supplied board excerpt

After further troubleshooting, the user supplied:

```text
RW007 WIFI CONNECTED
LWIP: LINK UP
LWIP: DHCP START
LWIP: DHCP START result=0
RW007: SPI TEST FINISHED
WHO_AM_I: 0xAF
ICM20608 INITIALIZED SUCCESSFULLY!
Reading accelerometer + gyroscope...
LWIP TX: ETH len=350
```

Wi-Fi association, link-up and sensor startup are now confirmed by this excerpt.
DHCP has started; `result=0` does not establish that an address was assigned.
The Ethernet transmit line alone does not establish MQTT sensor publication.

The exact local change that preceded this successful association was not
independently verified. This report does not infer a radio band solely from
the earlier target network's name.

## Next acceptance evidence

1. Capture current `LWIP: DHCP BOUND` and subsequent board TLS/MQTT readiness.
2. Capture packet-ID PUBACK lines and receive five actual sensor messages on
   the Ubuntu broker.
3. Confirm advancing prediction IDs/current UTC timestamps and a PASS from
   `timed_walk_jog.py --check-only`.
4. Run the [independently timed movement protocol](TIMED_VALIDATION.md), retain
   raw rows, quality exclusions and measured results, then commit that evidence.
5. Complete the remaining Android physical-phone, CubeMX regeneration and
   physical regression checks in the [firmware verification guide](../../firmware/README.md#integration-verification).

Earlier October 23 success remains historical evidence in
[the live pipeline report](2026-10-23_cubemx_live_pipeline.md). It is not a
substitute for current delivery after this restart.

## Subsequent timed movement result

Later in the same troubleshooting sequence, the user supplied a COMPLETE
six-trial output for `timed_walk_jog_20261025T153506Z_0abffb`: 51/52 eligible
windows correct (98.1%), with all trials reported Valid. This supports resumed
database prediction flow after the earlier stale-data stop. The original
`session.json` and `summary.md` have since been supplied and are preserved
unchanged. Independent raw-data selection and recorded quality checks agree;
helper replay regenerates both originals byte-for-byte.

See the [movement trial report](2026-10-25_cubemx_walk_jog_trial.md) for the
matrix, raw audit, file hashes and limits. Detailed DHCP/PUBACK capture and
current Android record matching remain separate checks.
