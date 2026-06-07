#!/usr/bin/env python3
import json
import os
import pathlib
import secrets
import socket
import subprocess
import sys
import time

APP_PATH = pathlib.Path(os.environ.get("QTEASYTIER_APP", "Install/QtEasyTier.app"))
HELPER_PATH = APP_PATH / "Contents/MacOS/QtEasyTierHelper"
INSTANCE_NAME = "QtET-HelperSmoke-FixedIp"
UID = os.getuid()
RUNTIME_DIR = pathlib.Path("/tmp") / f"qteasytier-{UID}"
SOCKET_PATH = RUNTIME_DIR / "helper.sock"
TOKEN_PATH = RUNTIME_DIR / "helper.token"
LOG_PATH = pathlib.Path("/tmp/qteasytier-helper-ipc-smoke.log")
STRICT_RUNTIME_DIR = pathlib.Path("/tmp") / f"qteasytier-{UID}-strict-shutdown"
STRICT_SOCKET_PATH = STRICT_RUNTIME_DIR / "helper.sock"
STRICT_TOKEN_PATH = STRICT_RUNTIME_DIR / "helper.token"
STRICT_LOG_PATH = pathlib.Path("/tmp/qteasytier-helper-ipc-strict-shutdown.log")
PROTOTYPE_DISABLED_MARKER = "QTEASYTIER_PROTOTYPE_HELPER_DISABLED"

def request(token, payload, expect_success=True):
    payload = dict(payload)
    payload.setdefault("version", 1)
    payload.setdefault("token", token)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(8)
        client.connect(str(SOCKET_PATH))
        client.sendall((json.dumps(payload) + "\n").encode("utf-8"))
        data = b""
        while not data.endswith(b"\n"):
            chunk = client.recv(65536)
            if not chunk:
                break
            data += chunk
    response = json.loads(data.decode("utf-8"))
    if expect_success and response.get("success") is not True:
        raise AssertionError(response)
    return response

def request_to(socket_path, token, payload, expect_success=True):
    payload = dict(payload)
    payload.setdefault("version", 1)
    payload.setdefault("token", token)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(8)
        client.connect(str(socket_path))
        client.sendall((json.dumps(payload) + "\n").encode("utf-8"))
        data = b""
        while not data.endswith(b"\n"):
            chunk = client.recv(65536)
            if not chunk:
                break
            data += chunk
    response = json.loads(data.decode("utf-8"))
    if expect_success and response.get("success") is not True:
        raise AssertionError(response)
    return response

def sudo(args, **kwargs):
    return subprocess.run(["sudo"] + args, **kwargs)

def sudo_popen(args, **kwargs):
    return subprocess.Popen(["sudo"] + args, **kwargs)

def helper_prototype_mode_enabled():
    result = subprocess.run(
        ["strings", str(HELPER_PATH)],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    return PROTOTYPE_DISABLED_MARKER not in result.stdout

def cleanup(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
    sudo(["rm", "-f", str(SOCKET_PATH), str(TOKEN_PATH), str(LOG_PATH)],
         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["rmdir", str(RUNTIME_DIR)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def cleanup_strict(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
    sudo(["rm", "-f", str(STRICT_SOCKET_PATH), str(STRICT_TOKEN_PATH), str(STRICT_LOG_PATH)],
         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["rmdir", str(STRICT_RUNTIME_DIR)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def verify_shutdown_requires_explicit_allow():
    for path in (STRICT_SOCKET_PATH, STRICT_TOKEN_PATH, STRICT_LOG_PATH):
        try:
            path.unlink()
        except FileNotFoundError:
            pass
    STRICT_RUNTIME_DIR.mkdir(mode=0o700, exist_ok=True)
    token = secrets.token_hex(32)
    STRICT_TOKEN_PATH.write_text(token, encoding="utf-8")
    os.chmod(STRICT_TOKEN_PATH, 0o600)

    process = None
    try:
        with STRICT_LOG_PATH.open("ab") as log:
            process = sudo_popen([
                str(HELPER_PATH),
                "--daemon",
                "--socket", str(STRICT_SOCKET_PATH),
                "--token-file", str(STRICT_TOKEN_PATH),
                "--owner-uid", str(UID),
            ], stdout=log, stderr=subprocess.STDOUT)

        deadline = time.time() + 10
        while time.time() < deadline and not STRICT_SOCKET_PATH.exists():
            time.sleep(0.1)
        if not STRICT_SOCKET_PATH.exists():
            raise AssertionError("strict helper socket did not appear")

        denied = request_to(
            STRICT_SOCKET_PATH,
            token,
            {"command": "shutdown"},
            expect_success=False,
        )
        if denied.get("success") is True or "not allowed" not in denied.get("error", ""):
            raise AssertionError(denied)

        allowed = request_to(STRICT_SOCKET_PATH, token, {"command": "prepare_uninstall"})
        time.sleep(0.8)
        if process.poll() is None:
            raise AssertionError("strict helper did not exit after prepare_uninstall")
        return {
            "shutdown_without_allow_rejected": denied.get("success") is not True,
            "prepare_uninstall_exited": allowed.get("success") is True and process.poll() is not None,
        }
    finally:
        cleanup_strict(process)

def main():
    if not HELPER_PATH.exists():
        raise SystemExit(f"missing helper: {HELPER_PATH}")
    if not helper_prototype_mode_enabled():
        raise SystemExit(
            "prototype helper smoke requires a development build configured "
            "with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON"
        )

    # Prompt once through sudo itself; no password files or command-line password.
    sudo(["-v"], check=True)

    strict_result = verify_shutdown_requires_explicit_allow()

    for path in (SOCKET_PATH, TOKEN_PATH, LOG_PATH):
        try:
            path.unlink()
        except FileNotFoundError:
            pass
    RUNTIME_DIR.mkdir(mode=0o700, exist_ok=True)
    token = secrets.token_hex(32)
    TOKEN_PATH.write_text(token, encoding="utf-8")
    os.chmod(TOKEN_PATH, 0o600)

    process = None
    try:
        with LOG_PATH.open("ab") as log:
            process = sudo_popen([
                str(HELPER_PATH),
                "--daemon",
                "--socket", str(SOCKET_PATH),
                "--token-file", str(TOKEN_PATH),
                "--owner-uid", str(UID),
                "--allow-shutdown",
            ], stdout=log, stderr=subprocess.STDOUT)

        deadline = time.time() + 10
        while time.time() < deadline and not SOCKET_PATH.exists():
            time.sleep(0.1)
        if not SOCKET_PATH.exists():
            raise AssertionError("helper socket did not appear")

        status = request(token, {"command": "status"})
        if status.get("client_identity_required") is not False:
            raise AssertionError(status)
        bad_token = request("bad-token", {"command": "status"}, expect_success=False)
        if bad_token.get("success") is True:
            raise AssertionError("bad token was accepted")

        toml = "\n".join([
            f'instance_name = "{INSTANCE_NAME}"',
            'hostname = "helper-smoke-fixed-ip"',
            "dhcp = false",
            'ipv4 = "11.45.14.9/24"',
            "",
            "[network_identity]",
            'network_name = "qtet-helper-smoke-fixed-ip"',
            'network_secret = "qtet-helper-smoke-secret"',
            "",
            "[flags]",
            "latency_first = true",
            "private_mode = true",
            "no_tun = false",
            "",
        ])
        request(token, {"command": "start", "instance": INSTANCE_NAME, "toml": toml})

        duplicate = request(
            token,
            {"command": "start", "instance": INSTANCE_NAME, "toml": toml},
            expect_success=False,
        )
        if duplicate.get("success") is True or "already owned" not in duplicate.get("error", ""):
            raise AssertionError(duplicate)

        last_list = None
        tun_seen = False
        for _ in range(35):
            last_list = request(token, {"command": "list"})
            text = json.dumps(last_list, ensure_ascii=False)
            if "TunDeviceReady" in text or "utun" in text or "11.45.14.9" in text:
                tun_seen = True
                break
            time.sleep(1)
        if not tun_seen:
            raise AssertionError({"missing_tun_signal": last_list})

        unknown_stop = request(
            token, {"command": "stop", "instance": "QtET-Unknown"}, expect_success=False)
        if unknown_stop.get("success") is True or "not owned" not in unknown_stop.get("error", ""):
            raise AssertionError(unknown_stop)

        running_shutdown = request(token, {"command": "shutdown"}, expect_success=False)
        if running_shutdown.get("success") is True or "running" not in running_shutdown.get("error", ""):
            raise AssertionError(running_shutdown)

        request(token, {"command": "stop", "instance": INSTANCE_NAME})
        for _ in range(12):
            listed = request(token, {"command": "list"})
            if not listed.get("infos"):
                break
            time.sleep(0.5)
        else:
            raise AssertionError("helper still lists instances after stop")

        request(token, {"command": "shutdown"})
        time.sleep(0.8)
        if process.poll() is None:
            raise AssertionError("helper did not exit after shutdown")

        print(json.dumps({
            **strict_result,
            "status_success": status.get("success") is True,
            "prototype_identity_bypass_reported": status.get("client_identity_required") is False,
            "bad_token_rejected": bad_token.get("success") is not True,
            "duplicate_rejected": duplicate.get("success") is not True,
            "tun_seen": tun_seen,
            "unknown_stop_rejected": unknown_stop.get("success") is not True,
            "running_shutdown_rejected": running_shutdown.get("success") is not True,
            "shutdown_exited": process.poll() is not None,
        }, ensure_ascii=False, indent=2))
        return 0
    finally:
        cleanup(process)

if __name__ == "__main__":
    raise SystemExit(main())
