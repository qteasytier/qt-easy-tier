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
RUN_ROOT="${QTEASYTIER_SMAPP_SMOKE_DIR:-/tmp/qteasytier-smapp-smoke-$(date +%Y%m%d-%H%M%S)}"
CONFIG="${HOME}/Library/Preferences/QtEasyTier"
BACKUP="${RUN_ROOT}/config-backup"
RESULTS="${RUN_ROOT}/results.jsonl"
SUMMARY="${RUN_ROOT}/summary.json"
RESTORED=0
CONFIG_BACKED_UP=0

mkdir -p "${RUN_ROOT}/json" "${RUN_ROOT}/screenshots" "${BACKUP}"
: > "$RESULTS"

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
    "manual": [item for item in items if item["status"] == "manual"],
}
open(summary, "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, indent=2))
print(json.dumps(out, ensure_ascii=False, indent=2))
sys.exit(1 if out["failed"] else 0)
PY
}

cleanup_app() {
  pkill -x "$APP_NAME" >/dev/null 2>&1 || true
  sleep 1
}

restore_config() {
  if [[ "$RESTORED" -eq 1 ]]; then
    return 0
  fi
  cleanup_app
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

write_settings_config() {
  mkdir -p "$CONFIG"
  python3 - "$CONFIG" <<'PY'
import json, os, sys
cfg = sys.argv[1]
settings = {
    "hide_on_tray": False,
    "auto_reconnect": False,
    "auto_check_update": False,
    "auto_start": False,
    "last_page": "settings",
}
json.dump(settings, open(os.path.join(cfg, "settings2.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=2)
json.dump([], open(os.path.join(cfg, "servers.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=2)
PY
}

snapshot() {
  local name="$1"
  "$PEEK" see --app "$APP_NAME" --json --path "${RUN_ROOT}/screenshots/${name}.png" > "${RUN_ROOT}/json/${name}.json" 2> "${RUN_ROOT}/json/${name}.err"
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

click_named() {
  local shot="$1"
  local name="$2"
  local elem
  elem="$(find_element_id "${RUN_ROOT}/json/${shot}.json" "$name")" || {
    log_result "click:${name}" "fail" "element not found in ${shot}"
    return 1
  }
  "$PEEK" click --on "$elem" --snapshot latest --app "$APP_NAME" --foreground > "${RUN_ROOT}/click-${name}.out" 2> "${RUN_ROOT}/click-${name}.err"
  local rc=$?
  sleep 1
  [[ $rc -eq 0 ]] && log_result "click:${name}" "pass" "$elem" || log_result "click:${name}" "fail" "rc=$rc elem=$elem"
  return $rc
}

service_status() {
  local stage="${1:-unknown}"
  local launch_file="${RUN_ROOT}/launchctl-helper-${stage}.txt"
  local btm_file="${RUN_ROOT}/btm-${stage}.txt"

  if launchctl print system/io.github.qteasytier.QtEasyTier.Helper > "$launch_file" 2>&1; then
    log_result "service:${stage}:launchctl" "pass" "system/io.github.qteasytier.QtEasyTier.Helper present"
  else
    log_result "service:${stage}:launchctl" "limited" "$(head -n 1 "$launch_file" 2>/dev/null)"
  fi

  sfltool dumpbtm > "$btm_file" 2>&1 || true
  if grep -qiE 'qteasytier|io\.github\.qteasytier' "$btm_file"; then
    log_result "service:${stage}:btm" "pass" "QtEasyTier entry found in background items database"
  else
    log_result "service:${stage}:btm" "limited" "QtEasyTier entry not found in background items database"
  fi
}

record_element_detail() {
  local shot="$1"
  local name="$2"
  local result_name="$3"
  python3 - "${RUN_ROOT}/json/${shot}.json" "$name" "$RESULTS" "$result_name" <<'PY'
import json, sys, time
path, name, results, result_name = sys.argv[1:5]
data = json.load(open(path, encoding="utf-8"))
for item in data.get("data", {}).get("ui_elements", []):
    values = [str(item.get(k, "")) for k in ("label", "title", "identifier", "description")]
    if any(name == value or name in value for value in values):
        detail = {
            key: item.get(key, "")
            for key in ("label", "title", "description", "value", "role_description", "enabled")
            if item.get(key, "") not in ("", None)
        }
        with open(results, "a", encoding="utf-8") as f:
            f.write(json.dumps({
                "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
                "name": result_name,
                "status": "pass",
                "detail": json.dumps(detail, ensure_ascii=False),
            }, ensure_ascii=False) + "\n")
        sys.exit(0)
with open(results, "a", encoding="utf-8") as f:
    f.write(json.dumps({
        "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "name": result_name,
        "status": "fail",
        "detail": f"{name} missing in {path}",
    }, ensure_ascii=False) + "\n")
sys.exit(1)
PY
}

set_app_window_bounds() {
  osascript <<'APPLESCRIPT' >/dev/null 2>&1 || true
tell application "System Events"
  if exists process "QtEasyTier" then
    tell process "QtEasyTier"
      if exists window 1 then
        set position of window 1 to {80, 40}
        set size of window 1 to {900, 720}
      end if
    end tell
  end if
end tell
APPLESCRIPT
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
if /usr/libexec/PlistBuddy -c 'Print :MachServices:io.github.qteasytier.QtEasyTier.Helper' "$APP_PATH/Contents/Library/LaunchDaemons/io.github.qteasytier.QtEasyTier.Helper.plist" >/dev/null 2>&1; then
  log_result "preflight:mach_service" "pass" "io.github.qteasytier.QtEasyTier.Helper"
else
  log_result "preflight:mach_service" "fail" "missing MachServices entry"
fi

log_result "preflight:app" "pass" "$APP_PATH"
log_result "preflight:peekaboo" "pass" "$PEEK"

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

write_settings_config
/usr/bin/open -n "$APP_PATH"
sleep 3
set_app_window_bounds
snapshot settings_initial || log_result "snapshot:settings_initial" "fail" "peekaboo failed"
for element in settings.macHelperStateLabel settings.macHelperRefreshButton settings.macHelperInstallButton settings.macHelperUninstallButton; do
  if find_element_id "${RUN_ROOT}/json/settings_initial.json" "$element" >/dev/null; then
    log_result "settings:ax_visible:${element}" "pass" "$element"
  else
    log_result "settings:ax_visible:${element}" "fail" "$element missing"
  fi
done
record_element_detail settings_initial settings.macHelperStateLabel settings:initial_state || true
record_element_detail settings_initial settings.macHelperDetailLabel settings:initial_detail || true
service_status initial || true

click_named settings_initial settings.macHelperInstallButton || true
sleep 2
snapshot after_install_click || true
record_element_detail after_install_click settings.macHelperStateLabel settings:after_install_state || true
record_element_detail after_install_click settings.macHelperDetailLabel settings:after_install_detail || true
log_result "smapp:approval" "manual" "If macOS shows an approval prompt or System Settings requires approval, approve QtEasyTier, then rerun this script."
service_status after_install || true

click_named after_install_click settings.macHelperRefreshButton || true
sleep 1
snapshot after_refresh || true
record_element_detail after_refresh settings.macHelperStateLabel settings:after_refresh_state || true
record_element_detail after_refresh settings.macHelperDetailLabel settings:after_refresh_detail || true
service_status after_refresh || true

click_named after_refresh settings.macHelperUninstallButton || true
sleep 1
snapshot after_uninstall_click || true
record_element_detail after_uninstall_click settings.macHelperStateLabel settings:after_uninstall_click_state || true
record_element_detail after_uninstall_click settings.macHelperDetailLabel settings:after_uninstall_click_detail || true
log_result "smapp:uninstall_confirm" "manual" "If a confirmation dialog is shown, confirm uninstall and rerun refresh."
service_status after_uninstall_click || true

restore_config
trap - EXIT
summarize
