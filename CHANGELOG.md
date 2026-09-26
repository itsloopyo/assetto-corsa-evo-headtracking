# Changelog

## [Unreleased]

### Added

- A setting set to `default` in `CameraUnlock.ini` takes its value from `Defaults.ini`, which every head tracking mod that keeps its settings in `CameraUnlock.ini` reads. Head tracking mods that keep their settings in another file do not read it, and neither do earlier versions of this mod. Writing a value in place of `default` changes that setting for this game only. When the mod saves a setting that a hotkey changed in game, it writes the new value in place of `default`, so that setting no longer follows `Defaults.ini` in this game until you set it to `default` again.
- `Defaults.ini` is `%AppData%\CameraUnlock\Defaults.ini` on Windows; `$XDG_CONFIG_HOME/CameraUnlock/Defaults.ini` on Linux, or `~/.config/CameraUnlock/Defaults.ini` where `XDG_CONFIG_HOME` is not set, under Wine and Proton too; and `~/Library/Application Support/CameraUnlock/Defaults.ini` on macOS. The mod's log, where it writes one, names the file it read.
- When the mod starts and finds no `Defaults.ini`, it creates one holding the built-in values, unless Windows runs the game as a packaged app. The mod never changes `Defaults.ini` after that.

### Changed

- Settings move to `CameraUnlock.ini`, next to `AssettoCorsaEVO.exe`. Earlier versions of the mod kept these settings in `HeadTracking.ini`, in the same folder. The first time this version starts and finds no `CameraUnlock.ini`, it reads your settings from `HeadTracking.ini` and writes them into `CameraUnlock.ini`. It never changes `HeadTracking.ini`, and does not read it again while `CameraUnlock.ini` exists.
- A first start with no `HeadTracking.ini` no longer writes one. It creates `CameraUnlock.ini` instead.
- A setting that the defaults the README shows set to `default` is written as `default` when the value imported for it equals its default at that start, which is the value `Defaults.ini` gives it, or the built-in value where `Defaults.ini` gives none. It then follows `Defaults.ini`. Every other setting is written with the value imported for it.
- `RotationEnabled` and `PositionEnabled` are one setting here, the tracking mode, so both are written as `default` or neither is.
- Comments, and keys the mod never read, are not carried over. Nor is this, where your old file had it:
  - A sensitivity or axis inversion you changed from its default. Set these in your tracker instead.
- An older version of the mod reads `HeadTracking.ini` and never reads `CameraUnlock.ini`, so a setting you change after updating is not in `HeadTracking.ini`.
- Deleting only `CameraUnlock.ini` makes the next start read `HeadTracking.ini` again. To go back to the defaults, replace everything in `CameraUnlock.ini` with the defaults the README shows. Every setting they set to `default` then follows `Defaults.ini`.
- Hotkeys are written as key names, and each hotkey lists every key that triggers it, the Ctrl+Shift chord included: `ToggleKey=End, Ctrl+Shift+Y`. `[Hotkeys] ToggleKey` and `ChordToggleKey`, virtual key codes, are imported together into `ToggleKey`, and `CycleModeKey` and `ChordCycleModeKey` into `CycleTrackingModeKey`, each as the plain key and Ctrl+Shift with the chord key.
- The tracking mode (Page Up / Ctrl+Shift+G) is saved to `CameraUnlock.ini` when you change it, and the game starts in the mode you left it in. Turning tracking on or off (End / Ctrl+Shift+Y) is still not saved; the game starts with head tracking on or off as `EnableOnStartup` says.
- The keys are renamed to the names every head tracking mod on `CameraUnlock.ini` uses: `LocalSmoothing` and `RemoteSmoothing` move from `[Rotation]` to `[Smoothing]`, and the lean limits are `PositionLimitX`, `PositionLimitZ` and `PositionLimitZBack`. `LimitY` set both vertical limits, so it becomes `PositionLimitY` (up) and `PositionLimitYDown` (down), both imported from it. `[Position] Enabled`, which chose the mode tracking started in, is imported as that mode: `Enabled=0` starts in rotation only, as it did.

### Removed

- The sensitivity and axis inversion settings, `[Rotation] YawSensitivity`, `PitchSensitivity`, `RollSensitivity`, `InvertYaw`, `InvertPitch` and `InvertRoll`, and `[Position] SensitivityX`, `SensitivityY`, `SensitivityZ`, `InvertX`, `InvertY` and `InvertZ`. Set these in your tracker app instead.
- With these settings at their shipped defaults the camera moves as it did before.

## [1.1.3] - 2026-09-09

### Fixed

- add the 2026-09-07 Steam build profile

### Other

- ACE patch watch: record buildid 25170311 [skip ci]

## [1.1.2] - 2026-09-01

### Fixed

- record the cameraunlock-core commit that is actually built
- mirror the vertical limit and restore the MIT grant
- re-sync THIRD-PARTY-NOTICES.md before cutting the tag
- resolve the adversarial review findings
- add the 2026-08-27 Steam build profile

### Other

- ACE patch watch: record buildid 24989348 [skip ci]

## [1.1.1] - 2026-08-26

### Added

- add build profile for the 2026-08-25 Steam patch

### Fixed

- complete the patch-watch rederive checklist

### Other

- ACE patch watch: record buildid 24928989 [skip ci]

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-08-20

### Added

- A one-shot log line the first time a head pose reaches the camera hook. The
  per-frame diagnostic only covers the first three frames, which run before a
  tracker is usually connected, so nothing in the log confirmed the pose ever
  got that far. It is logged ahead of the tracking-enabled check, so toggling
  tracking off does not hide it.

### Changed

- The tracker owns the centre. The pose it sends is applied as absolute, so
  centre the view in your tracker app: OpenTrack's Center bind, SteamVR, or
  your phone app's CENTER button.

### Removed

- The recenter hotkeys (`Home` / `Ctrl+Shift+T`), the `RecenterKey` and
  `ChordRecenterKey` settings, and the mod-side centre capture behind them.
  Every tracker already centres itself, so the two centres drifted apart and a
  single recentre took a press on each side.

## [1.0.2] - 2026-08-17

### Added

- `[Rotation] LocalSmoothing` (default `0.0`) for a tracker running on this PC,
  and `[Rotation] RemoteSmoothing` (default `0.15`) for a phone or other device
  on the network. Which one applies is decided per connection from the packet's
  source address and re-evaluated when the source changes, so switching between
  a local OpenTrack instance and a phone takes effect without a restart. Both
  cover rotation and position alike.

### Removed

- `[Rotation] Smoothing` and `[Position] Smoothing`, replaced by the pair
  above. There is no separate position smoothing setting any more.
- The hidden 0.15 smoothing floor. It silently overrode whatever the user set,
  so a tracker on the same machine now gets zero-latency tracking by default.

## [1.0.1] - 2026-08-15

### Added

- `RecenterKey`, `ToggleKey` and `CycleModeKey` in `[Hotkeys]`, with a chord
  counterpart for each, so every action is remappable rather than just the yaw
  mode toggle. A code the mod cannot bind, whether a modifier, out of range, or
  a key name typed where a code belongs, leaves the action on its previous key
  and logs why, and the boot line names every key it ended up bound to.

### Changed

- Head yaw always turns the view about the world's up axis, so a banked corner
  never tilts the axis the head turns about, and looking down at the pedals
  then turning pans across the floor.

### Removed

- The camera-local yaw mode, its `Page Down` / `Ctrl+Shift+H` hotkey, and the
  `WorldSpaceYaw` and `YawModeKey` settings that selected it.

## [1.0.0] - 2026-08-12

### Added

- Initial head tracking support for Assetto Corsa EVO (Steam build 20260722).
- OpenTrack UDP receiver on port 4242 with interpolation, smoothing, per-axis
  sensitivity and inversion.
- Positional (6DOF) tracking with per-axis limits.
- Hotkeys: `Home` / `Ctrl+Shift+T` recenter, `End` / `Ctrl+Shift+Y` toggle
  tracking, `Page Up` / `Ctrl+Shift+G` cycle tracking mode (rotation and
  position / rotation only / position only).
- Head rotation relative to the car rather than the world, so the view leans
  with the car through a banked corner or over a crest, the way a driver's head
  does in the seat.
- Tracking holds the view while the session is paused or not running. The
  cockpit still renders behind the pause menu, so the view would otherwise keep
  following the player's head there. Session state is read from the game's own
  `acevo_pmf_graphics` telemetry page, not from a pinned address.
- Append-only build profile registry with a PE-fingerprint failsafe: on an
  unrecognised game build the mod stays fully dormant and the game runs vanilla.
- Validation of every `HeadTracking.ini` value that reaches the camera math.
  Non-finite or out-of-range smoothing, sensitivities, travel limits and UDP
  ports are reported in the log and replaced with the default instead of being
  carried into the view transform.
- `pixi run test` unit tests covering that config boundary, the camera
  transform composition, and the shipped configuration defaults.
