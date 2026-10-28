#!/usr/bin/env python3
"""Configure Yena's existing Mosquitto 2.0 server with authenticated TLS.

On the Ubuntu server, after creating ~/rtspark-mqtt-pki:
  sudo /home/ubuntu/rt-spark-activity-recognition/.venv/bin/python \
      /home/ubuntu/configure_rtspark_mqtt_tls.py

Uses the project's existing paho-mqtt 2.x dependency. The existing CA and
server keys are reused. A separate broker verifies TLS 1.2, authentication,
and local anonymous access before the service configuration is installed.
No sensor messages are published. If activation fails, the new configuration
is removed from conf.d and the original service is restarted.

--check-only validates the installed listener without modifying its files.
"""

import argparse
import getpass
import grp
import os
from pathlib import Path
import pwd
import re
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import uuid


BROKER_HOST = "ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
USERNAME = "yena_sensor"
LOCAL_PORT = 1883
TLS_PORT = 8883
BASE = Path("/etc/mosquitto")
TLS_DIR = BASE / "rtspark-tls"
CONFIG = BASE / "conf.d" / "20-rtspark-tls.conf"
MQTT = None


def run(command):
    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise RuntimeError("Command failed: {}\n{}".format(
            " ".join(map(str, command)), result.stderr.strip() or result.stdout.strip()))
    return result.stdout


def write_file(path, content, gid, mode=0o640):
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
        stream.write(content)
    os.chown(path, 0, gid)
    os.chmod(path, mode)


def configuration(directory, local_port, tls_port, public=True):
    address = "0.0.0.0" if public else "127.0.0.1"
    return """# RT-Spark MQTT: local classifier and authenticated sensor TLS.
per_listener_settings true

listener {local_port} 127.0.0.1
allow_anonymous true

listener {local_port} ::1
allow_anonymous true

listener {tls_port} {address}
allow_anonymous false
password_file {directory}/passwd
acl_file {directory}/acl
cafile {directory}/ca.crt
certfile {directory}/server.crt
keyfile {directory}/server.key
tls_version tlsv1.2
require_certificate false
""".format(directory=directory, local_port=local_port,
           tls_port=tls_port, address=address)


def tls_context(cafile):
    context = ssl.create_default_context(cafile=str(cafile))
    # Test the exact TLS generation supported by the board's current library.
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    return context


def check_connection(port, cafile=None, password=None, expected=True, host="127.0.0.1"):
    reply = threading.Event()
    accepted = []
    client = MQTT.Client(
        callback_api_version=MQTT.CallbackAPIVersion.VERSION2,
        client_id="rtspark-check-" + uuid.uuid4().hex[:12],
        protocol=MQTT.MQTTv311,
        reconnect_on_failure=False,
    )

    def connected(client, userdata, flags, reason_code, properties):
        accepted.append(not reason_code.is_failure)
        reply.set()

    client.on_connect = connected
    if password is not None:
        client.username_pw_set(USERNAME, password)
    if cafile is not None:
        client.tls_set_context(tls_context(cafile))
    started = False
    try:
        client.connect(host, port, keepalive=15)
        client.loop_start()
        started = True
        if not reply.wait(7):
            raise RuntimeError("No MQTT connection reply on port {}.".format(port))
        if accepted != [expected]:
            raise RuntimeError("Unexpected MQTT authentication result on port {}.".format(port))
    finally:
        client.disconnect()
        if started:
            client.loop_stop()


def unused_ports():
    sockets = []
    try:
        for _ in range(2):
            sock = socket.socket()
            sockets.append(sock)
            sock.bind(("127.0.0.1", 0))
        return [sock.getsockname()[1] for sock in sockets]
    finally:
        for sock in sockets:
            sock.close()


def preflight(stage, password, gid):
    local_port, tls_port = unused_ports()
    preview = stage / "preview.conf"
    write_file(preview, configuration(stage, local_port, tls_port, public=False), gid)
    with tempfile.TemporaryFile(mode="w+t") as log:
        broker = subprocess.Popen(["mosquitto", "-c", str(preview)],
                                  stdin=subprocess.DEVNULL, stdout=log, stderr=log)
        try:
            ready = False
            for _ in range(40):
                if broker.poll() is not None:
                    break
                try:
                    with socket.create_connection(("127.0.0.1", tls_port), timeout=0.2):
                        ready = True
                    break
                except OSError:
                    time.sleep(0.1)
            if not ready:
                raise RuntimeError("Temporary broker could not start.")
            ca = stage / "ca.crt"
            check_connection(tls_port, ca, password)
            print("  TLS 1.2 and MQTT account: PASS", flush=True)
            check_connection(tls_port, ca, expected=False)
            check_connection(tls_port, ca, password + "-incorrect", expected=False)
            print("  Anonymous access and incorrect password rejected: PASS", flush=True)
            check_connection(local_port)
            check_connection(local_port, host="::1")
            print("  Local IPv4 and IPv6 connections: PASS", flush=True)
        except Exception:
            log.flush()
            log.seek(0)
            print("Temporary broker log:\n" + log.read()[-6000:], file=sys.stderr)
            raise
        finally:
            if broker.poll() is None:
                broker.terminate()
                try:
                    broker.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    broker.kill()
                    broker.wait(timeout=5)
            preview.unlink(missing_ok=True)


def get_password(confirm):
    password = getpass.getpass("MQTT password for yena_sensor: ")
    if not (12 <= len(password) <= 64) or any(
        char == ":" or not (33 <= ord(char) <= 126) for char in password
    ):
        raise RuntimeError("Use 12-64 characters: letters, numbers and punctuation; no spaces or colon.")
    if confirm and password != getpass.getpass("Confirm MQTT password: "):
        raise RuntimeError("The passwords did not match.")
    return password


def check_installed(password):
    run(["systemctl", "is-active", "--quiet", "mosquitto"])
    check_connection(TLS_PORT, TLS_DIR / "ca.crt", password)
    check_connection(LOCAL_PORT)
    check_connection(LOCAL_PORT, host="::1")
    print("  Active TLS account and local IPv4/IPv6 connections: PASS", flush=True)


def setup(source, password):
    gid = grp.getgrnam("mosquitto").gr_gid
    stage = Path(tempfile.mkdtemp(prefix=".rtspark-tls-", dir=BASE))
    os.chown(stage, 0, gid)
    os.chmod(stage, 0o750)
    installed = False
    promoted = False
    try:
        for name in ("ca.crt", "server.crt", "server.key"):
            destination = stage / name
            shutil.copyfile(source / name, destination)
            os.chown(destination, 0, gid)
            os.chmod(destination, 0o640)
        # -U converts this newly created, root-only file to the 2.0-compatible
        # PBKDF2 format; the secret never appears in process arguments or logs.
        passwd = stage / "passwd"
        write_file(passwd, USERNAME + ":" + password + "\n", gid, mode=0o600)
        run(["mosquitto_passwd", "-H", "sha512-pbkdf2", "-U", str(passwd)])
        if not passwd.read_text().startswith(USERNAME + ":$7$"):
            raise RuntimeError("Password hashing did not produce the expected format.")
        os.chown(passwd, 0, gid)
        os.chmod(passwd, 0o640)
        write_file(stage / "acl", "user yena_sensor\ntopic write activity/sensor\n", gid)

        print("Testing a separate local broker before changing the service...", flush=True)
        preflight(stage, password, gid)
        write_file(stage / "listener.conf", configuration(TLS_DIR, LOCAL_PORT, TLS_PORT), gid, mode=0o644)
        stage.rename(TLS_DIR)
        promoted = True
        # An exclusive link publishes the complete config without replacing
        # any existing file. Both paths are on the same /etc filesystem.
        os.link(TLS_DIR / "listener.conf", CONFIG)
        installed = True
        print("Activating the verified configuration...", flush=True)
        run(["systemctl", "restart", "mosquitto"])
        check_installed(password)
    except BaseException:
        if installed:
            failed = CONFIG.with_name(CONFIG.name + ".failed-" + uuid.uuid4().hex[:8])
            CONFIG.rename(failed)
            try:
                run(["systemctl", "restart", "mosquitto"])
                run(["systemctl", "is-active", "--quiet", "mosquitto"])
                print("Original broker configuration restored and running.", file=sys.stderr)
            except Exception as restore_error:
                print("Broker restoration needs attention: " + str(restore_error), file=sys.stderr)
        if promoted:
            print("TLS files retained at {} for diagnosis.".format(TLS_DIR), file=sys.stderr)
        raise
    finally:
        if not promoted:
            shutil.rmtree(stage)


def main():
    global MQTT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    if os.geteuid() != 0:
        raise RuntimeError("Run with sudo and the project's .venv/bin/python interpreter.")
    import paho.mqtt.client as mqtt
    MQTT = mqtt
    if not hasattr(MQTT, "CallbackAPIVersion"):
        raise RuntimeError("The project's paho-mqtt 2.x dependency is required.")
    for executable in ("mosquitto", "mosquitto_passwd", "openssl", "systemctl"):
        if not shutil.which(executable):
            raise RuntimeError("Missing command: " + executable)
    if args.check_only:
        if not CONFIG.is_file():
            raise RuntimeError("The TLS listener has not been installed.")
        check_installed(get_password(confirm=False))
        return
    if TLS_DIR.exists() or CONFIG.exists():
        raise RuntimeError("RT-Spark TLS files already exist. Use --check-only to test them.")
    if list((BASE / "conf.d").glob("*.conf")):
        raise RuntimeError("Custom .conf files now exist. Send their relevant settings before changing this setup.")
    text = (BASE / "mosquitto.conf").read_text()
    if not re.search(r"(?m)^\s*include_dir\s+/etc/mosquitto/conf\.d\s*$", text):
        raise RuntimeError("The expected include_dir is missing from mosquitto.conf.")
    protected = r"listener|port|bind_address|per_listener_settings|password_file|acl_file|allow_anonymous|plugin|auth_plugin|cafile|certfile|keyfile"
    if re.search(r"(?m)^\s*(?:" + protected + r")\s+", text):
        raise RuntimeError("The main config contains existing listener/security settings. Send those settings first.")
    run(["systemctl", "is-active", "--quiet", "mosquitto"])
    sudo_uid = os.environ.get("SUDO_UID")
    if not sudo_uid or int(sudo_uid) == 0:
        raise RuntimeError("Run this from ubuntu's SSH account using sudo.")
    source = Path(pwd.getpwuid(int(sudo_uid)).pw_dir) / "rtspark-mqtt-pki"
    for name in ("ca.crt", "server.crt", "server.key"):
        if not (source / name).is_file():
            raise RuntimeError("Missing certificate input: " + str(source / name))
    run(["openssl", "verify", "-CAfile", str(source / "ca.crt"),
         "-verify_hostname", BROKER_HOST, "-purpose", "sslserver", str(source / "server.crt")])
    certificate_key = run(["openssl", "x509", "-in", str(source / "server.crt"), "-pubkey", "-noout"])
    private_key_public_part = run(["openssl", "pkey", "-in", str(source / "server.key"), "-pubout"])
    if certificate_key.strip() != private_key_public_part.strip():
        raise RuntimeError("The server certificate and private key do not match.")
    print("Certificates verified. Creating MQTT account: yena_sensor", flush=True)
    setup(source, get_password(confirm=True))
    print("\nSUCCESS: authenticated MQTT TLS is ready on port 8883.")
    print("Local classifier access remains available on 127.0.0.1:1883 and [::1]:1883.")
    print("Broker hostname: " + BROKER_HOST)
    print("Topic allowed for yena_sensor: publish activity/sensor")
    print("CA certificate for the board: " + str(source / "ca.crt"))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nSetup interrupted.", file=sys.stderr)
        sys.exit(1)
    except Exception as error:
        print("STOPPED: " + str(error), file=sys.stderr)
        sys.exit(1)
