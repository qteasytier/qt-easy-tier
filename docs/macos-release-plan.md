# macOS Release Plan

This document tracks two macOS targets: the current community testing build
and the later Developer ID distribution build. The goal is to keep the macOS
work focused: package the existing Qt/EasyTier behavior correctly, avoid
changing core network semantics, and make every privileged operation explicit
and testable.

## Release Targets

### Community testing target

The community testing target is suitable for GitHub/Gitee release artifacts
used by technical users. It is not notarized and may require the user to open
the app explicitly or clear the quarantine attribute.

- `QtEasyTier.app` is produced by CMake install.
- `QtEasyTierHelper` is bundled inside `Contents/MacOS`.
- Network and OneClick can start TUN-backed EasyTier instances through the
  prototype helper after administrator authorization.
- The GUI can stop helper-owned instances and refresh state from helper
  `list` output.
- The app bundle passes local code-sign verification.
- A DMG can be created and verified locally.
- `QTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON` is used so the app remains usable
  without Apple Developer ID credentials.
- Validation follows `docs/macos-community-build.md`.

### Developer ID distribution target

The Developer ID distribution target is an optional later target suitable for
users who expect Gatekeeper to accept the app without manual workarounds.

- The app is signed with a Developer ID Application certificate.
- Hardened Runtime is enabled for the app, helper, bundled frameworks, and
  embedded executables.
- The final artifact is notarized with Apple's notary service and stapled.
- Gatekeeper accepts the distributed artifact on a clean machine.
- Privileged helper installation is explicit, revocable, and recoverable.
- The app can detect missing, outdated, stopped, or unauthorized helper states.
- Uninstall removes the privileged helper and helper runtime files.

## Current State

Implemented in the current macOS worktree:

- macOS app bundle packaging.
- Bundled helper executable target.
- Bundled LaunchDaemon plist scaffold in
  `Contents/Library/LaunchDaemons`.
- Client-side bundle-state probe for the future service-managed helper path.
- Service-management scaffold for querying, registering, and unregistering the
  bundled LaunchDaemon through `SMAppService` on macOS 13 or newer.
- Settings page helper status UI. Community testing builds hide the install,
  upgrade, and uninstall actions; those lifecycle actions are reserved for the
  Developer ID service-managed helper target.
- Service-managed helpers stay alive after the last network stops; the
  prototype helper is the only path that accepts the `shutdown` command.
- Helper uninstall uses a `prepare_uninstall` IPC command to verify that no
  helper-owned networks are running and remove helper runtime files before
  unregistering the service.
- Service-managed helper transport uses an XPC Mach service declared by the
  LaunchDaemon plist; prototype smoke keeps the local socket transport.
- The XPC helper applies a client peer code-signing requirement derived from
  the helper's Team ID and the main app bundle identifier.
- The XPC helper now fails closed if it is launched without a signing Team ID;
  ad-hoc builds must use the prototype socket smoke path.
- Service-managed helper IPC uses XPC peer code-signing requirements instead of
  a bearer token.
- Prototype helper IPC still uses a local socket with token authentication.
- Prototype helper sockets are restricted to the owning GUI user and reject
  peers with a different local uid.
- Prototype helper runtime paths are centralized under `/tmp/qteasytier-$uid`.
- Prototype helper fallback is controlled by the CMake option
  `QTEASYTIER_ENABLE_PROTOTYPE_HELPER`; community testing builds set it to
  `ON`, while Developer ID distribution builds should set it to `OFF`.
- Network TUN start and stop through the helper.
- OneClick host TUN start and stop through the helper.
- GUI state refresh using helper-owned instance information.
- Network and OneClick expose stable accessibility names for GUI smoke tests.
- OneClick layout adjusted so host/latency controls are visible in the default
  window size.
- Local `codesign --verify --deep --strict` passes after install.
- Release packaging uses an explicit nested-signing path and can create a
  locally verified DMG.
- App and helper entitlements files are present for Hardened Runtime signing.

Latest local validation before commit:

- `cmake --build` and `cmake --install` pass on the development Mac.
- The bundled LaunchDaemon plist passes `plutil -lint` after install.
- Direct helper IPC/TUN start, list, stop, and shutdown should be rerun with the
  current `package/mac/smoke_helper_ipc.py` before publishing.
- `package/mac/smoke_gui_ax.zsh` should be rerun with the current script before
  publishing. The community GUI smoke uses Accessibility element names and checks
  helper-owned Network and OneClick instances without depending on a public peer.
- `package/mac/build_mac.sh` creates
  `Install/QtEasyTier_v2.1.2_macos_arm64_community.dmg`; `hdiutil verify`
  passes.

Known community testing limits:

- The service-managed helper lifecycle still needs a real SMAppService
  install/approval/uninstall pass on a clean machine before it can be exposed in
  a Developer ID distribution build.
- Service-managed helper IPC now targets an XPC Mach service with a Team ID and
  app identifier peer code-signing requirement. The helper refuses XPC service
  startup when no signing Team ID is present. The Unix socket transport remains
  for prototype smoke only.
- Community testing builds use the local helper path when the service-managed
  helper is not enabled or is still waiting for System Settings approval.
- `QTEASYTIER_DISABLE_PROTOTYPE_HELPER=1` can disable the local helper fallback
  for diagnostics.
- Developer ID signing, notarization, stapling, service approval after signing,
  and clean-machine Gatekeeper checks are optional future distribution work.
- Packaging now has an explicit nested-signing script path with optional
  notarization, but it still needs a Developer ID identity and notary profile.

## Architecture Direction

The final app should keep the GUI unprivileged. Only the helper should run with
elevated privileges, and only for operations that require TUN access.

Preferred helper lifecycle:

1. The GUI checks whether the bundled helper is installed and current.
2. If missing or outdated, the GUI asks the user to install or upgrade it.
3. The system launches the helper as a privileged service after admin approval.
4. The GUI connects to the helper and sends structured commands.
5. The helper starts, stops, and lists only instances it owns.
6. The GUI can request helper uninstall from the settings or diagnostics flow.

The production helper mechanism should be based on Apple's service management
APIs where possible. On macOS 13 and newer, `SMAppService` supports registering
LaunchAgents and LaunchDaemons, and LaunchDaemons require administrator
approval before the system bootstraps them.

## IPC Contract

The helper IPC should remain small and versioned.

Required request fields:

- `version`
- `command`
- `instance_name` for per-instance operations
- `toml_config` for `start`

Required commands:

- `status`
- `start`
- `stop`
- `list`
- `shutdown`
- `uninstall` or `prepare_uninstall`

Response shape:

```json
{
  "success": true,
  "error": "",
  "version": 1
}
```

Rules:

- Unknown commands fail closed.
- Malformed JSON fails closed.
- Helper rejects empty instance names.
- Helper never stops instances it did not start.
- Helper never accepts arbitrary filesystem paths from the GUI for execution.
- Helper logs enough detail for diagnostics without logging secrets.

## Implementation Phases

### Phase 1: Stabilize the current helper path

- Keep the current local socket helper path working for prototype smoke.
- Add a `status` command.
- Make helper startup, timeout, and error messages deterministic.
- Centralize helper runtime paths.
- Add debug logs that are useful but do not expose secrets.
- Preserve existing Network and OneClick behavior.

Acceptance:

- Network start, list, stop passes on the development Mac.
- OneClick host start, list, stop passes on the development Mac.
- Repeated start/stop cycles do not leave helper processes behind.
- GUI restart while a helper instance is running can show and stop it.

Current phase-1 status command:

```json
{
  "version": 1,
  "command": "status",
  "token": "runtime-token"
}
```

Expected response:

```json
{
  "success": true,
  "version": 1,
  "helper_version": "<project version>",
  "owned_instance_count": 0
}
```

### Phase 2: Production helper lifecycle

- Add helper install detection.
- Add helper version detection.
- Add helper upgrade path.
- Add helper uninstall path.
- Replace ad hoc administrator startup with a service-managed helper lifecycle.
- Keep the GUI unprivileged.

Acceptance:

- Fresh install asks for admin approval once.
- Subsequent launches do not ask for the password on every start.
- Helper can be upgraded when the bundled helper version changes.
- Helper can be removed.
- Failed install leaves the app usable and explains the failure.

### Phase 3: Packaging and signing

- Add entitlements files for app and helper if needed.
- Sign every Mach-O in the bundle with the correct identity.
- Enable Hardened Runtime for distribution signing.
- Verify no Homebrew or user-local absolute library paths remain.
- Create the DMG after signing.
- Notarize and staple the final artifact.

Acceptance:

- `codesign --verify --deep --strict` passes.
- `spctl --assess` passes on the final artifact.
- `xcrun stapler validate` passes after notarization.
- A downloaded copy opens without Gatekeeper warnings on a clean Mac.

### Phase 4: Smoke and regression matrix

- Clean user account test.
- Reboot test.
- Missing helper test.
- Outdated helper test.
- Network TUN start/stop test.
- Network `no_tun = true` test.
- OneClick host start/stop test.
- OneClick guest invalid-code error test.
- App quit while network is running.
- Helper crash/restart behavior.

Acceptance:

- Every test has a reproducible script or a documented manual checklist.
- Screenshots and IPC logs are kept for failing cases.
- No privileged helper processes remain after stop/uninstall tests.

## Estimated Remaining Time

For a high-quality beta on the current development Mac:

- 1 to 2 focused days.
- Main work: helper status/error hardening, repeated smoke tests, cleanup.

For a real distribution release:

- 5 to 8 focused days if Developer ID credentials are ready.
- Main work: production helper lifecycle, signing/notarization, clean-machine
  testing, and release documentation.

Risk factors:

- Apple service-management behavior differs by macOS version.
- Developer ID / notarization credentials may not be ready.
- Hardened Runtime can expose missing entitlements or unsigned nested binaries.
- Clean-machine testing may uncover dependency path or helper-install issues.

## Detailed Roadmap

### Track A: Helper correctness

Goal: make the helper path deterministic before changing the installation
mechanism.

Tasks:

- Keep all responses versioned, including errors.
- Frame all IPC requests by newline and wait for a complete request before
  parsing JSON.
- Check helper protocol compatibility with `status` before reusing a running
  helper.
- Keep helper-owned instance state unchanged when `retain_network_instance`
  fails.
- Add negative tests for malformed JSON, missing token, wrong token, unsupported
  version, empty instance name, and unknown command.
- Add repeated start/stop smoke loops for Network and OneClick.

Done when:

- Prototype helper can handle partial socket writes.
- Helper can reject malformed or incompatible requests without crashing.
- Repeated start/stop cycles leave no helper process and no TUN instance.

### Track B: Helper lifecycle

Goal: stop prompting for the administrator password on every app launch.

Tasks:

- Define helper state values: missing, installed, outdated, running, stopped,
  unauthorized, incompatible, and unknown.
- Add a client-side helper state query that does not prompt for authorization.
- Add a user-triggered install/upgrade operation.
- Add uninstall operation.
- Move from ad hoc administrator startup toward a service-managed helper.
- Document fallback behavior for macOS versions that cannot use the preferred
  helper registration API.

Done when:

- Fresh install prompts once for helper installation.
- Reopening the app does not prompt for a password just to start a network.
- Helper upgrade is detected when the bundled helper version changes.
- Uninstall removes the privileged helper.

### Track C: GUI integration

Goal: make helper state understandable to users without changing core behavior.

Tasks:

- Add clear errors for helper missing, install cancelled, incompatible helper,
  helper startup timeout, and helper communication failure.
- Ensure Network and OneClick both refresh from helper `list` output.
- Ensure GUI restart while a helper-owned network is running can show and stop
  that instance.
- Keep `no_tun = true` on the in-process FFI path.
- Keep Windows and Linux behavior unchanged.

Done when:

- Network and OneClick TUN paths run through helper.
- Network `no_tun = true` still runs in-process.
- GUI never appears stuck after helper failure.

### Track D: Signing, notarization, and packaging

Goal: produce a downloadable macOS artifact accepted by Gatekeeper.

Tasks:

- Add app/helper entitlements if required.
- Sign nested frameworks, helper, CIDRCalculator, EasyTier dylib, and the app.
- Use Developer ID signing with Hardened Runtime for distribution builds.
- Create DMG after signing.
- Notarize with Apple's notary service.
- Staple notarization ticket.
- Validate with `spctl` and `stapler`.

Done when:

- `codesign --verify --deep --strict` passes.
- `spctl --assess` passes on the final artifact.
- `xcrun stapler validate` passes.
- Downloaded artifact launches on a clean Mac without Gatekeeper rejection.

### Track E: Release tests

Goal: prove the app works outside the development setup.

Tasks:

- Run tests on a clean user account.
- Run install/start/stop after reboot.
- Test missing helper and outdated helper flows.
- Test Network TUN start/stop.
- Test Network `no_tun = true`.
- Test OneClick host start/stop.
- Test OneClick guest invalid-code error.
- Test quitting the app while helper-owned instances are running.
- Test uninstall cleanup.

Done when:

- Each scenario has a command script or written manual checklist.
- Failure artifacts include screenshots and helper IPC logs.
- No root helper remains after stop or uninstall tests.

## PR Strategy

The work does not need many PRs. A small number of high-quality PRs is better:

1. macOS app bundle, helper, and packaging support.
2. Production helper lifecycle and release hardening.

If upstream prefers smaller changes, split by review boundary rather than by
implementation convenience.

## Do Not Change

- EasyTier FFI semantics.
- TOML generation semantics except where needed to route TUN mode through the
  helper.
- Existing Windows behavior.
- Existing Linux behavior.
- UI style system or custom control rendering outside the scoped macOS fixes.
