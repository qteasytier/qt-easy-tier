#!/bin/zsh
set -u

APP_PATH="${QTEASYTIER_APP:-Install/QtEasyTier.app}"
APP_NAME="QtEasyTier"
if [[ -n "${QTEASYTIER_PEEK:-}" ]]; then
  PEEK="${QTEASYTIER_PEEK}"
elif command -v peekaboo >/dev/null 2>&1; then
  PEEK="$(command -v peekaboo)"
else
  PEEK=""
fi
RUN_ROOT="${QTEASYTIER_SMOKE_DIR:-/tmp/qteasytier-gui-ax-smoke-$(date +%Y%m%d-%H%M%S)}"
CONFIG="${HOME}/Library/Preferences/QtEasyTier"
BACKUP="${RUN_ROOT}/config-backup"
RESULTS="${RUN_ROOT}/results.jsonl"
SUMMARY="${RUN_ROOT}/summary.json"
HELPER_PID=""
RESTORED=0
CONFIG_BACKED_UP=0

mkdir -p "${RUN_ROOT}/json" "${RUN_ROOT}/screenshots" "${BACKUP}"
: > "${RESULTS}"

log_result() {
  local name="$1"
  local state="$2"
  local detail="$3"
  python3 - "$RESULTS" "$name" "$state" "$detail" <<'PY'
import json, sys, time
path, name, state, detail = sys.argv[1:5]
with open(path, "a", encoding="utf-8") as f:
    f.write(json.dumps({
        "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "name": name,
        "status": state,
        "detail": detail,
    }, ensure_ascii=False) + "\n")
PY
}

cleanup_helper() {
  if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" != "1" ]]; then
    rm -f "/tmp/qteasytier-$(id -u)/helper.sock" "/tmp/qteasytier-$(id -u)/helper.token" >/dev/null 2>&1 || true
    rmdir "/tmp/qteasytier-$(id -u)" >/dev/null 2>&1 || true
    return 0
  fi
  if [[ -n "$HELPER_PID" ]]; then
    sudo -n kill "$HELPER_PID" >/dev/null 2>&1 || true
  fi
  sudo -n pkill -x QtEasyTierHelper >/dev/null 2>&1 || true
  sudo -n rm -f "/tmp/qteasytier-$(id -u)/helper.sock" "/tmp/qteasytier-$(id -u)/helper.token" >/dev/null 2>&1 || true
  rmdir "/tmp/qteasytier-$(id -u)" >/dev/null 2>&1 || true
}

cleanup_app() {
  pkill -x "$APP_NAME" >/dev/null 2>&1 || true
  sleep 1
}

open_app() {
  if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" == "1" ]]; then
    /usr/bin/open -n \
      --env QTEASYTIER_FORCE_PROTOTYPE_HELPER=1 \
      --env QTEASYTIER_ALLOW_PROTOTYPE_HELPER=1 \
      "$APP_PATH" > "${RUN_ROOT}/open-app.out" 2> "${RUN_ROOT}/open-app.err" && return 0
    QTEASYTIER_FORCE_PROTOTYPE_HELPER=1 \
      QTEASYTIER_ALLOW_PROTOTYPE_HELPER=1 \
      "${APP_PATH}/Contents/MacOS/QtEasyTier" > "${RUN_ROOT}/direct-app.out" 2> "${RUN_ROOT}/direct-app.err" &
    return 0
  fi
  /usr/bin/open -n "$APP_PATH"
}

restore_config() {
  if [[ "$RESTORED" -eq 1 ]]; then
    return 0
  fi
  cleanup_app
  cleanup_helper
  if [[ "$CONFIG_BACKED_UP" -ne 1 ]]; then
    log_result "restore:config" "limited" "config backup was not created; leaving existing config untouched"
    RESTORED=1
    return 0
  fi
  if [[ "$CONFIG" != "${HOME}/Library/Preferences/QtEasyTier" ]]; then
    log_result "restore:path_guard" "fail" "$CONFIG"
    return 1
  fi
  rm -rf "$CONFIG"
  if [[ -d "$BACKUP/QtEasyTier" ]]; then
    mkdir -p "${HOME}/Library/Preferences"
    mv "$BACKUP/QtEasyTier" "$CONFIG"
    log_result "restore:config" "pass" "restored backup"
  else
    log_result "restore:config" "pass" "removed test config"
  fi
  RESTORED=1
}

trap restore_config EXIT

write_config() {
  local page="$1"
  mkdir -p "$CONFIG"
  python3 - "$CONFIG" "$page" <<'PY'
import json, os, sys, time
cfg, page = sys.argv[1:3]
ts = int(time.time())
settings = {
    "hide_on_tray": False,
    "auto_reconnect": False,
    "auto_check_update": False,
    "auto_start": False,
    "last_page": page,
}
network = {
    "instance_name": "QtET-GuiAxSmoke",
    "network_label": "Gui AX Smoke",
    "is_running": False,
    "hostname": "gui-ax-smoke",
    "network_name": f"qtet-gui-ax-smoke-{ts}",
    "network_secret": "qtet-gui-ax-secret",
    "dhcp": False,
    "ipv4": "11.45.14.10/24",
    "latency_first": True,
    "private_mode": True,
    "servers": [],
    "enable_kcp_proxy": True,
    "disable_kcp_input": False,
    "no_tun": False,
    "enable_quic_proxy": False,
    "disable_quic_input": False,
    "disable_relay_kcp": False,
    "disable_relay_quic": False,
    "enable_relay_foreign_network_kcp": False,
    "enable_relay_foreign_network_quic": False,
    "disable_udp_hole_punching": False,
    "disable_tcp_hole_punching": False,
    "disable_upnp": False,
    "need_p2p": False,
    "lazy_p2p": False,
    "p2p_only": False,
    "multi_thread": True,
    "use_smoltcp": False,
    "bind_device": True,
    "disable_p2p": False,
    "enable_exit_node": False,
    "system_forwarding": False,
    "disable_sym_hole_punching": False,
    "disable_ipv6": False,
    "relay_all_peer_rpc": False,
    "enable_encryption": True,
    "accept_dns": False,
    "dev_name": "",
    "mtu": 0,
    "default_protocol": "",
    "encryption_algorithm": "aes-gcm",
    "foreign_network_whitelist_enabled": False,
    "foreign_network_whitelist": [],
    "listen_addresses": [],
    "proxy_networks": [],
    "custom_routes": [],
    "exit_nodes": [],
}
json.dump(settings, open(os.path.join(cfg, "settings2.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=2)
json.dump([network], open(os.path.join(cfg, "network2.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=2)
json.dump([], open(os.path.join(cfg, "servers.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=2)
PY
}

snapshot() {
  local name="$1"
  "$PEEK" see --app "$APP_NAME" --json --path "${RUN_ROOT}/screenshots/${name}.png" > "${RUN_ROOT}/json/${name}.json" 2> "${RUN_ROOT}/json/${name}.err"
}

peekaboo_health_check() {
  local out="${RUN_ROOT}/json/peekaboo-health.json"
  local err="${RUN_ROOT}/json/peekaboo-health.err"
  "$PEEK" see --json --timeout-seconds 8 > "$out" 2> "$err" || return 1
  python3 - "$out" <<'PY'
import json, sys
try:
    data = json.load(open(sys.argv[1], encoding="utf-8"))
except Exception:
    sys.exit(1)
sys.exit(0 if data.get("success") is True else 1)
PY
}

find_element_id() {
  python3 - "$1" "$2" <<'PY'
import json, sys
path, name = sys.argv[1:3]
data = json.load(open(path, encoding="utf-8"))
for item in data.get("data", {}).get("ui_elements", []):
    values = [str(item.get(k, "")) for k in ("label", "title", "identifier", "description")]
    if any(name == value or name in value for value in values):
        print(item["id"])
        sys.exit(0)
sys.exit(1)
PY
}

prototype_helper_enabled() {
  local helper="${APP_PATH}/Contents/MacOS/QtEasyTierHelper"
  [[ -x "$helper" ]] && ! strings "$helper" | grep -q "QTEASYTIER_PROTOTYPE_HELPER_DISABLED"
}

expect_element_visible() {
  local shot="$1"
  local name="$2"
  if find_element_id "${RUN_ROOT}/json/${shot}.json" "$name" >/dev/null; then
    log_result "${shot}:ax_visible:${name}" "pass" "$name"
  else
    log_result "${shot}:ax_visible:${name}" "fail" "$name missing"
    return 1
  fi
}

expect_element_absent() {
  local shot="$1"
  local name="$2"
  if find_element_id "${RUN_ROOT}/json/${shot}.json" "$name" >/dev/null; then
    log_result "${shot}:ax_hidden:${name}" "fail" "$name should be hidden"
    return 1
  fi
  log_result "${shot}:ax_hidden:${name}" "pass" "$name hidden"
}

click_named() {
  local shot="$1"
  local name="$2"
  local elem
  local log_prefix="${RUN_ROOT}/click-${shot}-${name}"
  elem="$(find_element_id "${RUN_ROOT}/json/${shot}.json" "$name")" || {
    log_result "click:${name}" "fail" "element not found in ${shot}"
    return 1
  }
  "$PEEK" click --on "$elem" --snapshot latest --app "$APP_NAME" --foreground --wait-for 5000 > "${log_prefix}.out" 2> "${log_prefix}.err"
  local rc=$?
  sleep 0.7
  if [[ $rc -ne 0 ]]; then
    "$PEEK" click "$name" --app "$APP_NAME" --foreground --wait-for 5000 > "${log_prefix}.retry.out" 2> "${log_prefix}.retry.err"
    rc=$?
    sleep 0.7
  fi
  if [[ $rc -eq 0 ]]; then
    log_result "click:${name}" "pass" "$elem"
  else
    log_result "click:${name}" "fail" "rc=$rc elem=$elem"
  fi
  return $rc
}

paste_named() {
  local shot="$1"
  local name="$2"
  local text="$3"
  click_named "$shot" "$name" || return 1
  "$PEEK" hotkey "cmd,a" --app "$APP_NAME" --foreground >/dev/null 2> "${RUN_ROOT}/hotkey-${name}.err" || true
  "$PEEK" paste --text "$text" --app "$APP_NAME" --foreground > "${RUN_ROOT}/paste-${name}.out" 2> "${RUN_ROOT}/paste-${name}.err"
  local rc=$?
  [[ $rc -eq 0 ]] && log_result "paste:${name}" "pass" "len=${#text}" || log_result "paste:${name}" "fail" "rc=$rc"
  return $rc
}

helper_request() {
  python3 - "$@" <<'PY'
import json, os, pathlib, socket, sys
command = sys.argv[1]
instance = sys.argv[2] if len(sys.argv) > 2 else ""
runtime = pathlib.Path("/tmp") / f"qteasytier-{os.getuid()}"
token = (runtime / "helper.token").read_text(encoding="utf-8").strip()
payload = {"version": 1, "token": token, "command": command}
if instance:
    payload["instance"] = instance
with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
    client.settimeout(8)
    client.connect(str(runtime / "helper.sock"))
    client.sendall((json.dumps(payload) + "\n").encode("utf-8"))
    data = b""
    while not data.endswith(b"\n"):
        chunk = client.recv(65536)
        if not chunk:
            break
        data += chunk
print(data.decode("utf-8").strip())
PY
}

wait_helper_list_contains() {
  local needle="$1"
  local out="${RUN_ROOT}/helper-list.json"
  local i
  for i in {1..35}; do
    helper_request list > "$out" 2> "${RUN_ROOT}/helper-list.err" || true
    if python3 - "$out" "$needle" <<'PY'
import json, sys
path, needle = sys.argv[1:3]
try:
    response = json.load(open(path, encoding="utf-8"))
except Exception:
    sys.exit(1)
for item in response.get("infos", []):
    if item.get("key") == needle:
        sys.exit(0)
sys.exit(1)
PY
    then
      log_result "helper:list_contains" "pass" "$needle"
      return 0
    fi
    sleep 1
  done
  log_result "helper:list_contains" "fail" "$needle"
  return 1
}

start_helper() {
  local helper="${APP_PATH}/Contents/MacOS/QtEasyTierHelper"
  local runtime="/tmp/qteasytier-$(id -u)"
  if ! prototype_helper_enabled; then
    log_result "preflight:prototype_helper" "fail" "rebuild with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON for prototype GUI smoke"
    return 1
  fi
  sudo -v || return 1
  cleanup_helper
  mkdir -p "$runtime"
  chmod 700 "$runtime"
  python3 - "$runtime/helper.token" <<'PY'
import pathlib, secrets, sys, os
path = pathlib.Path(sys.argv[1])
path.write_text(secrets.token_hex(32), encoding="utf-8")
os.chmod(path, 0o600)
PY
  sudo -n "$helper" --daemon --socket "$runtime/helper.sock" --token-file "$runtime/helper.token" --owner-uid "$(id -u)" --allow-shutdown > "${RUN_ROOT}/helper-gui-smoke.log" 2>&1 &
  HELPER_PID="$!"
  local i
  for i in {1..50}; do
    [[ -S "$runtime/helper.sock" ]] && return 0
    sleep 0.2
  done
  return 1
}

summarize() {
  python3 - "$RESULTS" "$SUMMARY" "$RUN_ROOT" <<'PY'
import json, sys
results, summary, run = sys.argv[1:4]
items = [json.loads(line) for line in open(results, encoding="utf-8") if line.strip()]
counts = {}
for item in items:
    counts[item["status"]] = counts.get(item["status"], 0) + 1
out = {
    "run": run,
    "counts": counts,
    "failed": [item for item in items if item["status"] == "fail"],
    "limited": [item for item in items if item["status"] == "limited"],
}
open(summary, "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, indent=2))
print(json.dumps(out, ensure_ascii=False, indent=2))
sys.exit(1 if out["failed"] else 0)
PY
}

if [[ ! -d "$APP_PATH" ]]; then
  log_result "preflight:app" "fail" "$APP_PATH"
  summarize
  exit 2
fi
if [[ -z "$PEEK" || ! -x "$PEEK" ]]; then
  log_result "preflight:peekaboo" "fail" "peekaboo not found; install it or set QTEASYTIER_PEEK=/path/to/peekaboo"
  summarize
  exit 2
fi
log_result "preflight:app" "pass" "$APP_PATH"
log_result "preflight:peekaboo" "pass" "$PEEK"
if peekaboo_health_check; then
  log_result "preflight:peekaboo_health" "pass" "desktop capture"
else
  log_result "preflight:peekaboo_health" "fail" "peekaboo cannot capture the desktop; check display capture and Accessibility permissions"
  summarize
  exit 2
fi

cleanup_app
if [[ -d "$CONFIG" ]]; then
  if mv "$CONFIG" "$BACKUP/QtEasyTier"; then
    log_result "preflight:backup_config" "pass" "$BACKUP/QtEasyTier"
  else
    log_result "preflight:backup_config" "fail" "$CONFIG"
    summarize
    exit 2
  fi
else
  log_result "preflight:backup_config" "pass" "no existing config"
fi
CONFIG_BACKED_UP=1

if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" == "1" ]]; then
  if start_helper; then
    log_result "preflight:helper" "pass" "prototype helper started"
  else
    log_result "preflight:helper" "fail" "prototype helper did not start"
    summarize
    exit 2
  fi
else
  log_result "preflight:helper" "limited" "set QTEASYTIER_GUI_SMOKE_HELPER=1 for real TUN helper click smoke"
fi

write_config settings
open_app
sleep 3
snapshot settings || log_result "snapshot:settings" "fail" "peekaboo failed"
expect_element_visible settings "settings.macHelperStateLabel"
expect_element_visible settings "settings.macHelperDetailLabel"
expect_element_visible settings "settings.macHelperRefreshButton"
if prototype_helper_enabled; then
  expect_element_absent settings "settings.macHelperInstallButton"
  expect_element_absent settings "settings.macHelperUninstallButton"
else
  expect_element_visible settings "settings.macHelperInstallButton"
  expect_element_visible settings "settings.macHelperUninstallButton"
fi

cleanup_app
write_config network
open_app
sleep 3
snapshot network || log_result "snapshot:network" "fail" "peekaboo failed"
if find_element_id "${RUN_ROOT}/json/network.json" "network.runButton" >/dev/null; then
  log_result "network:ax_visible" "pass" "network.runButton"
else
  log_result "network:ax_visible" "fail" "network.runButton missing"
fi

if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" == "1" ]]; then
  click_named network "network.runButton"
  wait_helper_list_contains "QtET-GuiAxSmoke"
  snapshot network_running || true
  click_named network_running "network.runButton"
  sleep 3
  if [[ ! -S "/tmp/qteasytier-$(id -u)/helper.sock" ]]; then
    log_result "network:helper_stop_empty" "pass" "helper socket removed"
  else
    helper_request list > "${RUN_ROOT}/helper-after-network-stop.json" 2> "${RUN_ROOT}/helper-after-network-stop.err" || true
    if grep -q '"infos":\[\]' "${RUN_ROOT}/helper-after-network-stop.json"; then
      log_result "network:helper_stop_empty" "pass" "helper list empty"
    else
      log_result "network:helper_stop_empty" "fail" "helper still has instances"
    fi
  fi
else
  log_result "network:real_tun_click" "limited" "set QTEASYTIER_GUI_SMOKE_HELPER=1 for real helper start/stop"
fi

cleanup_app
write_config oneclick
if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" == "1" ]]; then
  if start_helper; then
    log_result "preflight:helper_restart" "pass" "prototype helper restarted for OneClick"
  else
    log_result "preflight:helper_restart" "fail" "prototype helper did not restart"
  fi
fi
open_app
sleep 3
snapshot oneclick || log_result "snapshot:oneclick" "fail" "peekaboo failed"
if find_element_id "${RUN_ROOT}/json/oneclick.json" "oneclick.startStopButton" >/dev/null; then
  log_result "oneclick:ax_visible" "pass" "oneclick.startStopButton"
else
  log_result "oneclick:ax_visible" "fail" "oneclick.startStopButton missing"
fi
click_named oneclick "oneclick.hostModeCheckBox"
sleep 0.7
snapshot oneclick_host || true

if [[ "${QTEASYTIER_GUI_SMOKE_HELPER:-0}" == "1" ]]; then
  click_named oneclick_host "oneclick.startStopButton"
  wait_helper_list_contains "QtET-OneClick"
  snapshot oneclick_running || true
  if helper_request stop "QtET-OneClick" > "${RUN_ROOT}/helper-stop-oneclick.json" 2> "${RUN_ROOT}/helper-stop-oneclick.err"; then
    log_result "oneclick:helper_stop" "pass" "QtET-OneClick"
  else
    log_result "oneclick:helper_stop" "fail" "QtET-OneClick"
  fi
  sleep 3
  if [[ ! -S "/tmp/qteasytier-$(id -u)/helper.sock" ]]; then
    log_result "oneclick:helper_stop_empty" "pass" "helper socket removed"
  else
    helper_request list > "${RUN_ROOT}/helper-after-oneclick-stop.json" 2> "${RUN_ROOT}/helper-after-oneclick-stop.err" || true
    if grep -q '"infos":\[\]' "${RUN_ROOT}/helper-after-oneclick-stop.json"; then
      log_result "oneclick:helper_stop_empty" "pass" "helper list empty"
    else
      log_result "oneclick:helper_stop_empty" "fail" "helper still has instances"
    fi
  fi
else
  log_result "oneclick:real_tun_click" "limited" "set QTEASYTIER_GUI_SMOKE_HELPER=1 for real helper start/stop"
fi

restore_config
trap - EXIT
summarize
