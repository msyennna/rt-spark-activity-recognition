"""Read-only TLS/API checks against the existing RT-Spark service.

Uses only Python's standard library and the unchanged Android CA. Sends a TLS
handshake to MQTT port 8883, but no MQTT CONNECT, credentials, subscribe or
publish. HTTPS uses GET only, follows no redirects and logs no activity labels,
confidence values or response bodies. This cannot prove new board DB updates.
"""

import argparse
from datetime import datetime, timezone
import http.client
import json
import math
from pathlib import Path
import re
import socket
import ssl
import sys


ROOT = Path(__file__).resolve().parents[2]
HOST = "ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
CA = ROOT / "android_app/app/src/main/res/raw/rtspark_ca.crt"
MAX_RESPONSE_BYTES = 65536


def tls_context():
    # Trust the lab CA only, matching Android and the firmware. No OS CA fallback.
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.load_verify_locations(cafile=str(CA))
    return context


def record_summary(record):
    if not isinstance(record, dict):
        raise ValueError("record must be an object")
    identifier = record.get("id")
    activity_id = record.get("activity_id")
    confidence = record.get("confidence")
    if type(identifier) is not int or identifier <= 0:
        raise ValueError("prediction ID must be a positive integer")
    if type(activity_id) is not int or (activity_id, record.get("activity")) not in (
        (0, "WALKING"), (1, "RUNNING")
    ):
        raise ValueError("activity ID/label does not match Android's contract")
    if type(confidence) not in (float, int) or not math.isfinite(confidence) or not 0 <= confidence <= 1:
        raise ValueError("confidence must be finite and between 0 and 1")
    timestamp = record.get("created_at")
    if not isinstance(timestamp, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}", timestamp):
        raise ValueError("created_at must use Android's SQLite UTC format")
    instant = datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S").replace(tzinfo=timezone.utc)
    return {"id": identifier, "created_at_utc": instant.isoformat().replace("+00:00", "Z")}


def mqtt_tls(timeout):
    with socket.create_connection((HOST, 8883), timeout=timeout) as connection:
        with tls_context().wrap_socket(connection, server_hostname=HOST) as secured:
            return {"status": "PASS", "tls": secured.version(), "cipher": secured.cipher()[0],
                    "mqtt_application_bytes_sent": 0}


def api_get(path, timeout):
    connection = http.client.HTTPSConnection(HOST, 443, timeout=timeout, context=tls_context())
    try:
        connection.request("GET", path, headers={"Accept": "application/json", "User-Agent": "RTSpark-ReadOnly-Check/1"})
        response = connection.getresponse()
        body = response.read(MAX_RESPONSE_BYTES + 1)
        if len(body) > MAX_RESPONSE_BYTES:
            raise ValueError("response exceeded the bounded JSON size")
        if response.status not in (200, 404):
            raise ValueError(f"unexpected HTTP status {response.status}")
        if response.getheader("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
            raise ValueError("API did not return application/json")
        value = json.loads(body)
        if not isinstance(value, dict):
            raise ValueError("API JSON must be an object")
        if path == "/api/activity/latest":
            if response.status == 404 and value.get("message") == "No activity predictions available.":
                return {"status": "PASS", "http_status": 404, "existing_records": False}
            if response.status != 200:
                raise ValueError("unexpected latest response")
            return {"status": "PASS", "http_status": 200, "schema_valid": True,
                    "record": record_summary(value)}
        records, count = value.get("records"), value.get("count")
        if response.status != 200 or not isinstance(records, list) or type(count) is not int or count != len(records) or not 0 <= count <= 1:
            raise ValueError("history count/records does not respect limit=1")
        return {"status": "PASS", "http_status": 200, "schema_valid": True,
                "count": count, "records": [record_summary(record) for record in records]}
    finally:
        connection.close()


def run_check(name, action):
    try:
        result = action()
    except ssl.SSLCertVerificationError as error:
        result = {"status": "FAIL", "reason": "certificate verification failed",
                  "verify_code": error.verify_code}
    except (ValueError, ssl.SSLError, http.client.HTTPException) as error:
        result = {"status": "FAIL", "reason": str(error)}
    except (TimeoutError, socket.gaierror, ConnectionError, OSError) as error:
        # A transport failure is not evidence that TLS/schema validation failed.
        # Includes sandbox/network denials; rerun with authorized network access.
        result = {"status": "UNREACHABLE", "reason": type(error).__name__}
    return {"check": name, **result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=int, default=8, help="per-operation timeout in seconds (1-30)")
    args = parser.parse_args()
    if not 1 <= args.timeout <= 30:
        parser.error("timeout must be between 1 and 30 seconds")
    # Catch endpoint drift without requesting any alternate host or reading secrets.
    api_source = (ROOT / "android_app/app/src/main/java/com/yena/rtsparkactivity/RtsparkApi.kt").read_text(encoding="utf-8")
    firmware_source = (ROOT / "firmware/include/rtspark_config.h").read_text(encoding="utf-8")
    if f'const val BASE_URL = "https://{HOST}"' not in api_source or f'#define RTSPARK_BROKER "{HOST}"' not in firmware_source:
        print("FAIL: checked host no longer matches the Android/firmware configuration", file=sys.stderr)
        return 1
    checks = [
        run_check("mqtt_tls_8883_no_mqtt_connect", lambda: mqtt_tls(args.timeout)),
        run_check("https_latest", lambda: api_get("/api/activity/latest", args.timeout)),
        run_check("https_history_limit_1", lambda: api_get("/api/activity/history?limit=1", args.timeout)),
    ]
    print(json.dumps({
        "checked_at_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "host": HOST, "ca_file": CA.relative_to(ROOT).as_posix(),
        "read_only": True, "firmware_database_updates_verified": False, "checks": checks,
    }, indent=2))
    if any(check["status"] == "FAIL" for check in checks):
        return 1
    return 2 if any(check["status"] == "UNREACHABLE" for check in checks) else 0


if __name__ == "__main__":
    sys.exit(main())
