# Changelog

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
