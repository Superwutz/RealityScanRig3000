# Release Process

This repository does not auto-release by default.
Use the release script to produce a consistent OTA/web-installer release.

## When To Release

Create a release when one of these is true:

- You finished a user-visible feature set on `main`.
- You fixed a production bug that should reach devices quickly.
- You changed firmware behavior that needs a traceable version.

## Go/No-Go Gate

Release only if all checks pass:

1. `main` is green in your local test flow (build + smoke test on hardware).
2. OTA path is validated once against the previous version.
3. No known blocker bugs for scan/trigger/update core flow.
4. Working tree is clean.

## Versioning Rule

Use SemVer tags with `v` prefix:

- `vX.Y.Z`
- `Z` = bugfix only
- `Y` = backward-compatible feature
- `X` = breaking or migration-heavy changes

Firmware/UI compile-time version uses the same number without `v`.

## Standard Release Command

From repo root on `main`:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\release.ps1 -Version v0.4.3 -Commit -Tag -Push
```

By default, the release script now runs:
- UI smoke tests against locally rendered embedded UI
- UI device tests against `http://scanrig.local`

Useful switches:
- `-UiDeviceUrl http://your-rig-hostname-or-ip`
- `-SkipDeviceTests` (keeps local UI smoke tests)
- `-SkipUiTests` (skip all UI tests)

What it does:

1. Validates branch, clean worktree, and version bump.
2. Runs UI smoke tests and device-backed UI tests (unless skipped).
3. Updates `SCANRIG_FW_VERSION` and `SCANRIG_UI_VERSION` in `src/main.cpp`.
4. Builds firmware and stages installer bundle.
5. Updates release artifacts in `docs/` (`manifest.json`, binaries, installer page).
6. Creates commit `release: vX.Y.Z`.
7. Creates annotated git tag `vX.Y.Z`.
8. Pushes `main` and tag.

## Dry Run / Prepare Only

If you want to generate artifacts first without committing:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\release.ps1 -Version v0.4.3
```

Then inspect changes with `git status` and continue manually.
