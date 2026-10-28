"""Compile the real firmware TLS library and verify it against a local server."""

import os
from pathlib import Path
import re
import runpy
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
FIRMWARE = ROOT / "firmware"


class BuildEnvironment:
    """Run the same verified dependency bootstrap without invoking SCons."""

    def subst(self, name):
        assert name == "$PROJECT_DIR"
        return str(FIRMWARE)

    def Append(self, **kwargs):
        pass

    def BuildSources(self, *args, **kwargs):
        pass


@unittest.skipUnless(sys.platform == "win32", "The native socket harness currently targets Windows")
class TLSIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.gcc = os.environ.get("CC") or shutil.which("gcc") or r"C:\msys64\ucrt64\bin\gcc.exe"
        cls.openssl = os.environ.get("OPENSSL") or shutil.which("openssl") or r"C:\Program Files\Git\usr\bin\openssl.exe"
        for program in (cls.gcc, cls.openssl):
            if not Path(program).is_file():
                raise RuntimeError(f"Native test prerequisite missing: {program}")
        cls.temp = tempfile.TemporaryDirectory(prefix="rtspark-tls-")
        cls.work = Path(cls.temp.name)
        cls.process_env = os.environ.copy()
        cls.process_env["PATH"] = str(Path(cls.gcc).parent) + os.pathsep + cls.process_env.get("PATH", "")
        runpy.run_path(str(FIRMWARE / "scripts" / "tls_build.py"), init_globals={
            "Import": lambda *args: None, "env": BuildEnvironment(),
        })
        source = FIRMWARE / ".deps" / "mbedtls-3.6.7"
        cls.client = cls.work / "client.exe"
        command = [cls.gcc, "-std=c99", "-O2", "-DMBEDTLS_CONFIG_FILE=\"rtspark_mbedtls_config.h\"",
                   "-I" + str(FIRMWARE / "include"), "-I" + str(source / "include"),
                   "-I" + str(source / "library"), str(Path(__file__).with_name("client.c"))]
        command += [str(path) for path in sorted((source / "library").glob("*.c"))]
        command += ["-o", str(cls.client), "-lws2_32", "-lbcrypt"]
        build = subprocess.run(command, capture_output=True, text=True, env=cls.process_env, timeout=120)
        if build.returncode:
            raise RuntimeError("Native Mbed TLS compile failed:\n" + build.stdout + build.stderr)

        def openssl(*arguments):
            result = subprocess.run([cls.openssl, *arguments], cwd=cls.work,
                                    capture_output=True, text=True, timeout=30)
            if result.returncode:
                raise RuntimeError("Test certificate generation failed:\n" + result.stderr)

        openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-sha256", "-days", "30",
                "-keyout", "ca.key", "-out", "ca.pem", "-subj", "/CN=RTSpark Native Test CA",
                "-addext", "basicConstraints=critical,CA:TRUE",
                "-addext", "keyUsage=critical,keyCertSign,cRLSign")
        openssl("req", "-new", "-newkey", "rsa:2048", "-nodes", "-sha256",
                "-keyout", "server.key", "-out", "server.csr", "-subj", "/CN=localhost")
        (cls.work / "server.ext").write_text(
            "subjectAltName=DNS:localhost\nbasicConstraints=critical,CA:FALSE\n"
            "keyUsage=critical,digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n",
            encoding="ascii")
        openssl("x509", "-req", "-in", "server.csr", "-CA", "ca.pem", "-CAkey", "ca.key",
                "-CAcreateserial", "-out", "server.pem", "-days", "7", "-sha256", "-extfile", "server.ext")
        openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-sha256", "-days", "30",
                "-keyout", "other.key", "-out", "other.pem", "-subj", "/CN=Unrelated Native CA",
                "-addext", "basicConstraints=critical,CA:TRUE")
        cls.context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        cls.context.minimum_version = ssl.TLSVersion.TLSv1_2
        cls.context.maximum_version = ssl.TLSVersion.TLSv1_2
        cls.context.set_ciphers("ECDHE-RSA-AES128-GCM-SHA256")
        cls.context.load_cert_chain(cls.work / "server.pem", cls.work / "server.key")

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def handshake(self, ca="ca.pem", hostname="localhost", timestamp=None):
        listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(20)
        port = listener.getsockname()[1]
        errors = []

        def serve():
            try:
                connection, _ = listener.accept()
                connection.settimeout(20)
                with connection:
                    # Force a real WANT_READ before ServerHello becomes available.
                    time.sleep(0.05)
                    with self.context.wrap_socket(connection, server_side=True):
                        pass
            except ssl.SSLError:
                pass  # Expected when the client rejects the test certificate.
            except Exception as error:
                errors.append(error)
            finally:
                listener.close()

        server = threading.Thread(target=serve, daemon=True)
        server.start()
        result = subprocess.run([str(self.client), str(port), str(self.work / ca), hostname,
                                 str(int(time.time()) if timestamp is None else timestamp)],
                                capture_output=True, text=True, timeout=20, env=self.process_env)
        server.join(timeout=3)
        self.assertFalse(server.is_alive(), "Local TLS server did not finish")
        self.assertFalse(errors, str(errors))
        fields = dict(re.findall(r"(rc|verify|want|crypto|polls)=(-?\d+)", result.stdout))
        self.assertEqual(len(fields), 5, result.stdout + result.stderr)
        print("  " + result.stdout.strip())
        return result.returncode, {key: int(value) for key, value in fields.items()}, result.stdout

    def test_verified_rsa_handshake_yields_to_foreground(self):
        result, fields, output = self.handshake()
        self.assertEqual(result, 0, output)
        self.assertEqual(fields["verify"], 0)
        self.assertGreater(fields["want"], 0)
        self.assertGreater(fields["crypto"], 0)
        self.assertGreater(fields["polls"], fields["crypto"])
        self.assertIn("TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256", output)

    def test_rejects_wrong_ca(self):
        result, fields, output = self.handshake(ca="other.pem")
        self.assertNotEqual(result, 0, output)
        self.assertTrue(fields["verify"] & 0x08, output)

    def test_rejects_wrong_hostname(self):
        result, fields, output = self.handshake(hostname="wrong.invalid")
        self.assertNotEqual(result, 0, output)
        self.assertTrue(fields["verify"] & 0x04, output)

    def test_rejects_clock_before_validity(self):
        result, fields, output = self.handshake(timestamp=1)
        self.assertNotEqual(result, 0, output)
        self.assertTrue(fields["verify"] & 0x200, output)

    def test_rejects_clock_after_expiry(self):
        result, fields, output = self.handshake(timestamp=int(time.time()) + 365 * 86400)
        self.assertNotEqual(result, 0, output)
        self.assertTrue(fields["verify"] & 0x01, output)


if __name__ == "__main__":
    unittest.main(verbosity=2)
