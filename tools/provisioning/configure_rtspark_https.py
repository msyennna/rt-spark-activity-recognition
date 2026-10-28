#!/usr/bin/env python3
"""Set up the RT-Spark HTTPS API on Ubuntu with the existing MQTT certificate.

Run on the Ubuntu VM: sudo python3 /home/ubuntu/configure_rtspark_https.py
Uses only Python's standard library. Installs Ubuntu's nginx package if needed.
Tests a separate loopback HTTPS proxy before activating the site on port 443.
The MQTT broker and Gunicorn application are not restarted or reconfigured.
"""

import argparse
import hashlib
import http.client
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import ssl
import subprocess
import sys
import tempfile
import time
import uuid


HOST = "ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
CA_FINGERPRINT = "8d43e9e5f7554ac78539b3fc5fe4ea05da1885ff616ec3c640d594cf82b1b202"
BASE = Path("/etc/nginx")
SOURCE = Path("/etc/mosquitto/rtspark-tls")
TLS_DIR = BASE / "rtspark-tls"
AVAILABLE = BASE / "sites-available/rtspark-api"
ENABLED = BASE / "sites-enabled/rtspark-api"
DEFAULT = BASE / "sites-enabled/default"
MAIN_CONFIG = BASE / "nginx.conf"
HTTPS_PORT = 443
UPSTREAM_PORT = 5000
NGINX = None


def run(command, timeout=30):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError("Command failed: {}\n{}".format(
            " ".join(map(str, command)), result.stderr.strip() or result.stdout.strip()))
    return result.stdout


def install_nginx():
    executable = shutil.which("nginx")
    if executable:
        return executable
    os_release = Path("/etc/os-release").read_text()
    if '\nID=ubuntu\n' not in '\n' + os_release.strip() + '\n':
        raise RuntimeError("Automatic installation is intended for your Ubuntu VM.")
    print("Installing Nginx from Ubuntu's package repositories...", flush=True)
    environment = dict(os.environ, DEBIAN_FRONTEND="noninteractive", NEEDRESTART_MODE="l")
    for command in (["apt-get", "update"], ["apt-get", "install", "-y", "nginx"]):
        subprocess.run(command, check=True, env=environment, timeout=600)
    executable = shutil.which("nginx")
    if not executable:
        raise RuntimeError("Nginx installation did not produce the expected executable.")
    return executable


def verify_certificates(source):
    for name in ("ca.crt", "server.crt", "server.key"):
        if not (source / name).is_file():
            raise RuntimeError("Missing certificate input: " + str(source / name))
    pem = (source / "ca.crt").read_text(encoding="ascii")
    if hashlib.sha256(ssl.PEM_cert_to_DER_cert(pem)).hexdigest() != CA_FINGERPRINT:
        raise RuntimeError("The CA differs from the one already used by your board. Stop for review.")
    run(["openssl", "verify", "-CAfile", source / "ca.crt", "-verify_hostname", HOST,
         "-purpose", "sslserver", source / "server.crt"])
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(str(source / "server.crt"), str(source / "server.key"))


def request_json(port, path, ca=None):
    connection = http.client.HTTPConnection(HOST if ca else "127.0.0.1", port, timeout=8)
    raw = None
    try:
        raw = socket.create_connection(("127.0.0.1", port), timeout=8)
        if ca:
            context = ssl.create_default_context(cafile=str(ca))
            context.minimum_version = ssl.TLSVersion.TLSv1_2
            connection.sock = context.wrap_socket(raw, server_hostname=HOST)
        else:
            connection.sock = raw
        connection.request("GET", path, headers={"Host": HOST, "Connection": "close"})
        response = connection.getresponse()
        if response.status != 200:
            raise RuntimeError("{} returned HTTP {}.".format(path, response.status))
        return json.loads(response.read(1024 * 1024).decode("utf-8"))
    finally:
        connection.close()
        if raw is not None:
            raw.close()


def check_api(port, ca=None):
    latest = request_json(port, "/api/activity/latest", ca)
    history = request_json(port, "/api/activity/history?limit=2", ca)
    if not isinstance(latest, dict) or not all(
            key in latest for key in ("id", "activity_id", "activity", "confidence", "created_at")):
        raise RuntimeError("Latest-prediction response does not match the working API.")
    if not isinstance(history, dict) or not isinstance(history.get("records"), list):
        raise RuntimeError("History response does not match the working API.")
    if history.get("count") != len(history["records"]) or len(history["records"]) > 2:
        raise RuntimeError("History count or limit was not preserved through the proxy.")
    return latest, history


def server_config(certificates, port, loopback=False):
    listen = "127.0.0.1:{}".format(port) if loopback else str(port)
    return """# RT-Spark API: HTTPS to the existing local Gunicorn service.
server {{
    listen {listen} ssl;
    server_name {host};
    ssl_certificate {certificates}/server.crt;
    ssl_certificate_key {certificates}/server.key;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_session_cache shared:RTSparkAPI:1m;
    ssl_session_timeout 10m;
    server_tokens off;

    location / {{
        proxy_pass http://127.0.0.1:{upstream};
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_set_header X-Forwarded-Proto https;
        proxy_set_header Connection "";
        proxy_connect_timeout 5s;
        proxy_read_timeout 15s;
        proxy_send_timeout 15s;
        proxy_buffering off;
        add_header Cache-Control "no-store" always;
    }}
}}
""".format(listen=listen, host=HOST, certificates=certificates, upstream=UPSTREAM_PORT)


def free_port():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def preflight_config(stage, port):
    return """user www-data;
worker_processes 1;
daemon off;
pid {stage}/preflight.pid;
error_log {stage}/preflight.log warn;
events {{ worker_connections 128; }}
http {{
    access_log off;
    client_body_temp_path {stage}/client-body;
    proxy_temp_path {stage}/proxy;
    {server}
}}
""".format(stage=stage, server=server_config(stage / "certificates", port, loopback=True))


def wait_check(port, ca, process=None):
    deadline = time.monotonic() + 12
    last_error = None
    while time.monotonic() < deadline:
        if process is not None and process.poll() is not None:
            raise RuntimeError("The separate Nginx preflight process exited unexpectedly.")
        try:
            return check_api(port, ca)
        except (OSError, RuntimeError, ValueError, http.client.HTTPException) as error:
            last_error = error
            time.sleep(0.2)
    raise RuntimeError("HTTPS API check failed: {}".format(last_error))


def preflight(stage):
    port = free_port()
    configuration = stage / "preflight.conf"
    configuration.write_text(preflight_config(stage, port), encoding="utf-8")
    run([NGINX, "-t", "-p", str(stage) + "/", "-c", configuration])
    with (stage / "process.log").open("w") as log:
        process = subprocess.Popen([NGINX, "-p", str(stage) + "/", "-c", str(configuration)],
                                   stdout=log, stderr=subprocess.STDOUT)
        try:
            wait_check(port, stage / "certificates/ca.crt", process)
        except Exception:
            details = log.name
            print("Preflight failed. Nginx diagnostic output:", flush=True)
            for file in (stage / "preflight.log", Path(details)):
                if file.exists():
                    print(file.read_text(errors="replace")[-4000:], flush=True)
            raise
        finally:
            if process.poll() is None:
                process.send_signal(signal.SIGQUIT)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait(timeout=5)


def service(action):
    run(["systemctl", action, "nginx"])


def service_active():
    return subprocess.run(["systemctl", "is-active", "--quiet", "nginx"], timeout=15).returncode == 0


def nginx_test():
    run([NGINX, "-t", "-c", MAIN_CONFIG])


def setup():
    for path in (AVAILABLE, ENABLED, TLS_DIR):
        if path.exists() or path.is_symlink():
            raise RuntimeError("HTTPS setup files already exist. Use --check-only, or send this message for review.")
    other_sites = [p for p in (BASE / "sites-enabled").iterdir() if p.name != "default"]
    if other_sites or list((BASE / "conf.d").glob("*.conf")):
        raise RuntimeError("Other Nginx site configuration exists. Stop for review before changing it.")
    if DEFAULT.exists() and (not DEFAULT.is_symlink() or DEFAULT.resolve() != BASE / "sites-available/default"):
        raise RuntimeError("The default site is customized. Stop for review before changing it.")
    nginx_test()
    was_active = service_active()
    previous_default = os.readlink(DEFAULT) if DEFAULT.is_symlink() else None
    # The certificate copies remain private to the Nginx master process.
    stage = Path(tempfile.mkdtemp(prefix=".rtspark-https-", dir=BASE))
    promoted = available_created = enabled_created = default_removed = False
    try:
        certificates = stage / "certificates"
        certificates.mkdir(mode=0o700)
        for name in ("ca.crt", "server.crt", "server.key"):
            path = certificates / name
            shutil.copyfile(SOURCE / name, path)
            path.chmod(0o600 if name.endswith(".key") else 0o644)
        print("Testing a separate local HTTPS proxy...", flush=True)
        preflight(stage)
        print("  CA, hostname verification, latest prediction and history: PASS", flush=True)
        certificates.rename(TLS_DIR)
        promoted = True
        with AVAILABLE.open("x", encoding="utf-8") as file:
            file.write(server_config(TLS_DIR, HTTPS_PORT))
        available_created = True
        AVAILABLE.chmod(0o644)
        ENABLED.symlink_to(AVAILABLE)
        enabled_created = True
        if previous_default is not None:
            DEFAULT.unlink()
            default_removed = True
        nginx_test()
        print("Activating the verified HTTPS configuration...", flush=True)
        service("reload" if was_active else "start")
        latest, history = wait_check(HTTPS_PORT, TLS_DIR / "ca.crt")
        service("enable")
        print("  Active HTTPS latest prediction and history: PASS", flush=True)
        print("\nSUCCESS: local HTTPS API checks passed on port 443.")
        print("Latest database ID: {}".format(latest["id"]))
        print("API base URL: https://{}/".format(HOST))
        print("Next: allow inbound HTTPS (443) from your current public IP in the EC2 security group.")
        print("The Android app will use your existing rtspark-ca.crt to trust this lab certificate.")
    except BaseException:
        if enabled_created:
            ENABLED.unlink(missing_ok=True)
        if default_removed:
            DEFAULT.symlink_to(previous_default)
        if available_created:
            AVAILABLE.unlink(missing_ok=True)
        if promoted:
            TLS_DIR.rename(BASE / ("rtspark-tls.failed-" + uuid.uuid4().hex[:8]))
        if enabled_created or default_removed:
            try:
                nginx_test()
                service("reload" if was_active else "stop")
                print("Previous Nginx site configuration restored.", flush=True)
            except Exception as rollback_error:
                print("Nginx recovery needs review: {}".format(rollback_error), file=sys.stderr)
        raise
    finally:
        shutil.rmtree(stage, ignore_errors=True)


def main():
    global NGINX
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    try:
        if os.geteuid() != 0:
            raise RuntimeError("Run this script with sudo on your Ubuntu server.")
        verify_certificates(SOURCE)
        print("Existing certificate, DNS name and private key: PASS", flush=True)
        check_api(UPSTREAM_PORT)
        print("Existing Gunicorn latest prediction and history: PASS", flush=True)
        if args.check_only:
            verify_certificates(TLS_DIR)
            latest, history = check_api(HTTPS_PORT, TLS_DIR / "ca.crt")
            print("PASS: verified HTTPS latest prediction and history; latest ID={}.".format(latest["id"]))
            return 0
        with socket.socket() as check:
            check.bind(("0.0.0.0", HTTPS_PORT))
        NGINX = install_nginx()
        setup()
        return 0
    except (RuntimeError, OSError, ValueError, http.client.HTTPException,
            subprocess.SubprocessError) as error:
        print("STOPPED: {}".format(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
