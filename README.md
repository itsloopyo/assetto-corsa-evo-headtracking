# Assetto Corsa EVO Head Tracking

![Assetto Corsa EVO running with this mod](https://raw.githubusercontent.com/itsloopyo/assetto-corsa-evo-headtracking/main/assets/readme-clip.gif)

An unofficial head tracking mod for Assetto Corsa EVO that moves the camera with your head while your wheel or controller keeps steering, driven by OpenTrack over UDP, with no VR headset required.

## Features

- **Decoupled view and driving** - your head moves the camera; the car, the physics and every input stay untouched
- **6DOF positional tracking** - lean into an apex, peek round the A-pillar, check your mirrors
- **Works with any OpenTrack compatible tracker** - free options available for PC, iOS and Android
- **Horizon-locked look** - your head turns about the world's up axis, so a banked corner does not tilt the axis you turn about and looking down at the pedals then turning pans across the floor
- **Works in every driving view** - cockpit, dash, bonnet, fixed external and the chase cam

## Requirements

- [Assetto Corsa EVO](https://store.steampowered.com/app/3058630/) on Steam, a legitimately purchased copy. Note: The game's TrackIR tab in controller settings is not implemented and is unrelated to this mod.
- A tracker that sends OpenTrack UDP pose data to port `4242` (`[Network] UdpPort` in `CameraUnlock.ini`): one 48-byte datagram of six little-endian 64-bit floats, `x, y, z, yaw, pitch, roll`. [OpenTrack](https://github.com/opentrack/opentrack) sends that from any of its inputs (webcam, TrackIR, Tobii, SteamVR). A phone app can send it straight to this PC if it has an OpenTrack or UDP output option; [Headcam](https://headcam.app) does, for free. See [Setting Up OpenTrack](#setting-up-opentrack).
- Windows 10 or 11, 64-bit.

## Installation

### Lopari

Download [Lopari](https://lopari.app), choose **Assetto Corsa EVO**, and click
**Play with head tracking**.

### Standalone Installer

1. Download the installer ZIP from the [Releases](https://github.com/itsloopyo/assetto-corsa-evo-headtracking/releases) page.
2. Extract it anywhere.
3. Double-click `install.cmd`. It finds the game and drops the loader and the mod next to `AssettoCorsaEVO.exe`.
4. Configure OpenTrack (or your phone app) to output UDP to `127.0.0.1` port `4242`.
5. Launch the game. The mod creates `CameraUnlock.ini`, its settings file, and a log next to the EXE on first run.

If the installer cannot find your game, point it at the install folder yourself. Either set the environment variable:

```powershell
$env:ASSETTO_CORSA_EVO_PATH = "D:\Games\Assetto Corsa EVO"
```

or pass the path as the first argument:

```powershell
install.cmd "D:\Games\Assetto Corsa EVO"
```

### Manual Installation

Copy two files into the Assetto Corsa EVO folder, the one containing `AssettoCorsaEVO.exe`:

1. `vendor/ultimate-asi-loader/dinput8.dll` to `dinput8.dll`. This is the Ultimate ASI Loader; skip this step if you already run an ASI loader for this game.
2. `plugins/AssettoCorsaEvoHeadTracking.asi` to `AssettoCorsaEvoHeadTracking.asi`.

The mod creates `CameraUnlock.ini` next to the EXE on first launch.

## Setting Up OpenTrack

The mod listens for OpenTrack pose data on UDP port `4242`, on every network
interface. One datagram is six little-endian 64-bit floats in the order
`x, y, z, yaw, pitch, roll`: position in centimetres, rotation in degrees, 48
bytes in total. Anything that sends that to that port drives the view.
OpenTrack's **UDP over network** output sends exactly this, and the steps below
set it up.

1. Install [OpenTrack](https://github.com/opentrack/opentrack/releases).
2. Pick a tracker under **Input**, using the notes below.
3. Set **Output** to **UDP over network**, host `127.0.0.1`, port `4242`.
4. Press **Start**. Tracking and the game can start in either order.

### Webcam

OpenTrack ships a `neuralnet tracker` input that reads a plain webcam. Select it
under **Input**, pick your camera in its settings, and use the output settings
above. How well it tracks depends on your camera and your lighting, so try it
before buying anything.

### Phone

A phone app can reach the mod directly, with no OpenTrack on the PC, if it sends
the datagram described above. Point it at this PC's IP address (run `ipconfig`
to find it) on port `4242`. Not every phone tracker speaks this protocol, so
check yours for an OpenTrack or UDP output option first. [Headcam](https://headcam.app)
sends it, and I wrote it so decent tracking is free for anyone who already owns
a phone.

Sending direct works when the app filters its own signal on the device. The
mod's smoothing is sized to take the edge off a clean signal rather than to
rescue a noisy one, so a raw feed sent direct will jitter. If it does, point the
app at OpenTrack's **UDP over network** *input* on some other port, say 5252,
and let OpenTrack's filters and curves clean it up before its output forwards to
`127.0.0.1:4242`.

Anything arriving from outside `127.0.0.0/8` counts as a remote connection and
is smoothed with `RemoteSmoothing` rather than `LocalSmoothing`. That includes a
tracker on this very PC that sends to the machine's own LAN address, because the
mod reads the source address and not the machine.

### Headset or other hardware

If your device has an OpenTrack input driver, select it under **Input** and use
the same output settings. OpenTrack's own **Input** list is the authority on
what it can read; the mod only ever sees what OpenTrack sends.

### Centring

Centring belongs to your tracker. The mod subtracts no centre of its own: it
applies the pose it receives exactly as it arrives, so a stream of zeros holds
the view where the game itself puts it. Press the centre control in your tracker
(OpenTrack's **Center** bind, or the CENTER button in Headcam) and the tracker
zeroes its own output, which leaves the view centred with the mod doing nothing.

That is why there is no centre hotkey here and nothing to re-centre in game. Two
centres in series would drift apart, because each side re-centres at moments the
other cannot see, and you would end up pressing twice to centre once. If the
view sits off to one side, centre it in the tracker.

## Controls

Two equivalent binding sets, use whichever your keyboard has:

| Action              | Nav-cluster | Chord           |
|---------------------|-------------|-----------------|
| Toggle tracking     | `End`       | `Ctrl+Shift+Y`  |
| Cycle tracking mode | `Page Up`   | `Ctrl+Shift+G`  |

`Page Up` / `Ctrl+Shift+G` cycles tracking mode:

1. Normal head-tracked gameplay
2. Positional tracking disabled, rotational tracking enabled
3. Rotational tracking disabled, positional tracking enabled
4. Back to normal

The mode you pick is saved to `CameraUnlock.ini`, and the game starts in it next time. `End` / `Ctrl+Shift+Y` changes the current session only; whether tracking is on when the game starts is `EnableOnStartup`.

Each action's keys are one list in `[Hotkeys]` in `CameraUnlock.ini`, `ToggleKey` and `CycleTrackingModeKey`, the chord included, so any of them can be changed or removed. That is worth doing if your button box or a wheel plugin already sits on one of them.

## Configuration

<!-- cameraunlock:config -->
The mod reads its settings from `CameraUnlock.ini` in the game folder, and creates the file when it starts and finds none. Edit it with any text editor.

A setting set to `default` takes its value from `Defaults.ini`, which every head tracking mod that keeps its settings in `CameraUnlock.ini` reads. Head tracking mods that keep their settings in another file do not read it, and neither do earlier versions of this mod. Writing a value in place of `default` changes that setting for this game only. When the mod saves a setting that a hotkey changed in game, it writes the new value in place of `default`, so that setting no longer follows `Defaults.ini` in this game until you set it to `default` again.

`Defaults.ini` is `%AppData%\CameraUnlock\Defaults.ini` on Windows; `$XDG_CONFIG_HOME/CameraUnlock/Defaults.ini` on Linux, or `~/.config/CameraUnlock/Defaults.ini` where `XDG_CONFIG_HOME` is not set, under Wine and Proton too; and `~/Library/Application Support/CameraUnlock/Defaults.ini` on macOS. The mod's log, where it writes one, names the file it read.

When the mod starts and finds no `Defaults.ini`, it creates one holding the built-in values, unless Windows runs the game as a packaged app. The mod never changes `Defaults.ini` after that. Edit it with any text editor.

Earlier versions of the mod kept these settings in `HeadTracking.ini`, in the same folder. The first time this version starts and finds no `CameraUnlock.ini`, it reads your settings from `HeadTracking.ini` and writes them into `CameraUnlock.ini`. It never changes `HeadTracking.ini`, and does not read it again while `CameraUnlock.ini` exists.

A setting that the defaults below set to `default` is written as `default` when the value imported for it equals its default at that start, which is the value `Defaults.ini` gives it, or the built-in value where `Defaults.ini` gives none. It then follows `Defaults.ini`. Every other setting is written with the value imported for it. `RotationEnabled` and `PositionEnabled` are one setting here, the tracking mode, so both are written as `default` or neither is.

Comments, and keys the mod never read, are not carried over. Nor are these, where your old file had them:

- Reticle settings, and a key that toggled the reticle.
- A sensitivity, scale, deadzone, response curve or axis inversion you changed from its default. Set these in your tracker instead.
- The setting for a feature that earlier versions shipped switched off while it was untested. It now follows the mod's default.

An older version of the mod reads `HeadTracking.ini` and never reads `CameraUnlock.ini`, so a setting you change after updating is not in `HeadTracking.ini`.

Deleting only `CameraUnlock.ini` makes the next start read `HeadTracking.ini` again. To go back to the defaults, replace everything in `CameraUnlock.ini` with the defaults below. Every setting they set to `default` then follows `Defaults.ini`.

The built-in value of each setting set to `default` below:

- `UdpPort=4242`
- `EnableOnStartup=true`
- `RotationEnabled=true`
- `LocalSmoothing=0.0`
- `RemoteSmoothing=0.15`
- `PositionEnabled=true`
- `PositionLimitX=0.3`
- `PositionLimitY=0.2`
- `PositionLimitYDown=0.2`
- `PositionLimitZ=0.4`
- `PositionLimitZBack=0.1`
- `ToggleKey=End, Ctrl+Shift+Y`
- `CycleTrackingModeKey=PageUp, Ctrl+Shift+G`

With every setting at its default, the file reads:

```ini
; Assetto Corsa EVO head tracking settings.
; Comments start with ; and go on their own line. Text after a value is part of the value.
; Hotkeys are key names such as End, PageUp or Ctrl+Shift+Y. Separate several with commas; leave empty for none.
; A setting set to default takes its value from Defaults.ini, which every head tracking mod
; that keeps its settings in CameraUnlock.ini reads: %AppData%\CameraUnlock\Defaults.ini on
; Windows, $XDG_CONFIG_HOME/CameraUnlock/Defaults.ini (normally ~/.config/CameraUnlock) on
; Linux, under Wine and Proton too, and ~/Library/Application Support/CameraUnlock/Defaults.ini
; on macOS. The log names the file it read. Write a value instead of default to change that
; setting for this game only.

[CameraUnlock]
; Written by the mod. Leave this section in place.
ConfigFormat=1

[Network]
; UDP port the mod receives tracker data on (OpenTrack protocol).
UdpPort=default

[General]
; true: head tracking is on when the game starts. ToggleKey turns it on and off.
EnableOnStartup=default
; true: turning your head turns the view.
; Tracking mode at startup, with PositionEnabled. The mode hotkey changes both.
RotationEnabled=default

[Smoothing]
; Smoothing when the tracker runs on this PC. 0 is the least, 1 the most.
LocalSmoothing=default
; Smoothing when the tracker is another device on the network, such as a phone.
; 0 is the least, 1 the most.
RemoteSmoothing=default

[Position]
; true: moving your head moves the view.
; Tracking mode at startup, with RotationEnabled. The mode hotkey changes both.
PositionEnabled=default
; How far, in metres, leaning left or right can move the view.
PositionLimitX=default
; How far, in metres, raising your head can move the view.
PositionLimitY=default
; How far, in metres, lowering your head can move the view.
PositionLimitYDown=default
; How far, in metres, leaning forward can move the view.
PositionLimitZ=default
; How far, in metres, leaning back can move the view.
PositionLimitZBack=default

[Hotkeys]
; Turns head tracking on and off.
ToggleKey=default
; Changes the tracking mode: rotation and position, rotation only, position only.
CycleTrackingModeKey=default
```
<!-- /cameraunlock:config -->

Changes take effect the next time the game starts.

Hotkeys are written as key names, such as `End`, `PageUp`, `F9` or `Ctrl+Shift+Y`, separated by commas. A key with no name can be written as its Windows virtual key code, `0x` and two hex digits, such as `0xBA`. A value the mod cannot read leaves that setting at its default and is named in `HeadTracking.log`.

## Troubleshooting

**Mod not loading.**

- Check `HeadTracking.log` next to the game EXE. It records whether the loader attached, whether the build profile matched, and whether the camera hooks landed. Each launch starts a fresh log and moves the previous one to `HeadTracking.prev.log`, so it never grows without bound.
- No log file at all means the ASI loader is not attaching. Confirm `dinput8.dll` is in the same folder as `AssettoCorsaEVO.exe`.
- If the log says the build is newer than the mod knows about, Assetto Corsa EVO has patched and the mod has not been updated for that build yet. It stays dormant on purpose. Check the releases page for a newer release.

**No tracking response.**

- Confirm your tracker is running and its output is UDP to `127.0.0.1` port `4242`, matching `UdpPort` in `CameraUnlock.ini`.
- Press `End` to make sure tracking is not toggled off.
- A phone app must target your PC's LAN IP, not `127.0.0.1`, and your firewall must allow inbound UDP on `4242`.
- Another game still running with a head tracking mod holds the port, and the log says `Failed to bind UDP port 4242`. The mod keeps retrying for as long as it is loaded, so close the other game and tracking starts within a second, logging `Bound UDP port 4242 after 12s of waiting - tracking is live`. There is no need to restart Assetto Corsa EVO.

**Jittery or unstable tracking.**

- Raise the smoothing value your tracker actually uses, in `[Smoothing]` in `CameraUnlock.ini`: `LocalSmoothing` if it runs on this PC, `RemoteSmoothing` if it is a phone or other device on the network. Start at `0.3`. The log line printed when a tracker connects says which of the two is in effect.
- Webcam trackers need even lighting and a clear view of your face; a dark room or a strong backlight makes the pose wander.
- On Wi-Fi, a phone app on the 5 GHz band is far steadier than 2.4 GHz.

**Wrong rotation axis or the view drifts off centre.**

- Centre in your tracker app while sitting in your normal driving position. The mod applies the pose it is sent as absolute and keeps no centre of its own.
- The mod applies the pose as your tracker sends it. If an axis moves the wrong way, invert that axis in your tracker's settings.

**The view keeps following my head in the pause menu.**

- The mod reads the session state from the game's own telemetry page. The log records every change (`[sim] session paused - head tracking held`). If the log says `head tracking will not pause with the game`, that page could not be mapped; report it with the log.

## Updating

Download the new release and run `install.cmd` again. Your `CameraUnlock.ini` is kept. Updating from v1.1.3 or earlier, the first start reads your settings from `HeadTracking.ini` into a new `CameraUnlock.ini`; see [Configuration](#configuration).

## Uninstalling

Run `uninstall.cmd`. This removes `AssettoCorsaEvoHeadTracking.asi` along with `HeadTracking.log` and `HeadTracking.prev.log`. The Ultimate ASI Loader is only removed if the installer put it there; use `uninstall.cmd /force` to remove it anyway. `CameraUnlock.ini` and `HeadTracking.ini` are left in place, so your settings are still there if you install again.

## Building from Source

Needs CMake and a Visual Studio C++ toolchain. The build never touches the game install and never needs the game to be present.

```powershell
git clone --recursive https://github.com/itsloopyo/assetto-corsa-evo-headtracking
cd assetto-corsa-evo-headtracking
pixi run build
pixi run test
pixi run package
```

## License

MIT License - see [LICENSE](LICENSE) for details. It covers this mod's own
source and the binaries built from it. The bundled Ultimate ASI Loader and the
statically linked MinHook keep their own licences, and the gameplay clip at the
top of this page is Assetto Corsa EVO footage that belongs to KUNOS Simulazioni
and the rights holders of the cars, circuits and sponsor marks in it, shown here
to demonstrate the mod and licensed to nobody. All of it is set out in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## Credits

- KUNOS Simulazioni for Assetto Corsa EVO.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) by ThirteenAG.
- [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.
- [OpenTrack](https://github.com/opentrack/opentrack) for the tracking protocol.

Full attribution in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## Disclaimer

This mod is not affiliated with, endorsed by, or supported by KUNOS Simulazioni. Use at your own risk.

## Community & Support

- Discord: [Loop's Head Tracking Hangout](https://discord.com/invite/dxyZdyFNT9) - setup help, bug reports, and new-release announcements
- [Lopari](https://lopari.app) - free Windows launcher with one-click install and launch for the released head-tracking mods
- [Headcam](https://headcam.app) - free app that turns your iPhone or Android phone into the head tracker
