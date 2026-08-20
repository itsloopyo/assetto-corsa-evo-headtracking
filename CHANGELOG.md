# Changelog

## [1.0.2] - 2026-08-17

### Added

- split smoothing into LocalSmoothing and RemoteSmoothing

## [Unreleased]

### Added

- Log a one-shot line the first time a head pose reaches the camera hook. The
  existing per-frame diagnostic only covers the first three frames, which run
  before a tracker is usually connected, so nothing in the log confirmed the
  pose ever got that far. It is logged ahead of the tracking-enabled check, so
  toggling tracking off does not hide it.

### Changed

- Smoothing is now two settings instead of one: `[Rotation] LocalSmoothing`
  (default `0.0`) applies when the tracker runs on this PC, `[Rotation]
  RemoteSmoothing` (default `0.15`) applies when it is a phone or other device
  on the network. Which one is used is decided per connection from the packet's
  source address and is re-evaluated when the source changes, so switching
  between a local OpenTrack instance and a phone takes effect without a restart.
- Removed `[Rotation] Smoothing` and `[Position] Smoothing`. Both new values
  cover rotation and position alike, so there is no separate position smoothing
  setting.
- Removed the hidden 0.15 smoothing floor. It silently overrode whatever the
  user set, so a tracker on the same machine now gets zero-latency tracking by
  default.
- The tracker owns the centre. The recenter hotkeys (`Home` / `Ctrl+Shift+T`)
  and their `RecenterKey` / `ChordRecenterKey` settings are gone, along with the
  mod-side centre capture; the tracker pose is applied as absolute. Centre the
  view in your tracker app instead.

## [1.0.1] - 2026-08-15

### Added

- make every hotkey remappable through [Hotkeys]
- lock head yaw to the world's up axis

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
