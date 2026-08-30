# Assetto Corsa EVO Head Tracking

![Assetto Corsa EVO running with this mod](https://raw.githubusercontent.com/itsloopyo/assetto-corsa-evo-headtracking/main/assets/readme-clip.gif)

An unofficial head tracking mod for Assetto Corsa EVO that moves the camera with your head while your wheel or controller keeps steering, driven by OpenTrack over UDP, with no VR headset required.

## Features

- **Decoupled view and driving** - your head moves the camera; the car, the physics and every input stay untouched
- **6DOF positional tracking** - lean into an apex, peek round the A-pillar, check your mirrors
- **Horizon-locked look** - your head turns about the world's up axis, so a banked corner does not tilt the axis you turn about and looking down at the pedals then turning pans across the floor
- **Works in every driving view** - cockpit, dash, bonnet, fixed external and the chase cam

## Requirements

- [Assetto Corsa EVO](https://store.steampowered.com/app/3058630/) on Steam, a legitimately purchased copy. Note: The game's TrackIR tab in controller settings is not implemented and is unrelated to this mod.
- A tracker that sends OpenTrack UDP pose data to port `4242` (`[Network] UdpPort` in `HeadTracking.ini`): one 48-byte datagram of six little-endian 64-bit floats, `x, y, z, yaw, pitch, roll`. [OpenTrack](https://github.com/opentrack/opentrack) sends that from any of its inputs (webcam, TrackIR, Tobii, SteamVR). A phone app can send it straight to this PC if it has an OpenTrack or UDP output option; [Headcam](https://headcam.app) does, for free. See [Setting Up OpenTrack](#setting-up-opentrack).
- Windows 10 or 11, 64-bit.

## Installation

1. Download the installer ZIP from the [Releases](https://github.com/itsloopyo/assetto-corsa-evo-headtracking/releases) page.
2. Extract it anywhere.
3. Double-click `install.cmd`. It finds the game and drops the loader and the mod next to `AssettoCorsaEVO.exe`.
4. Configure OpenTrack (or your phone app) to output UDP to `127.0.0.1` port `4242`.
5. Launch the game. The mod writes a default `HeadTracking.ini` and a log next to the EXE on first run.

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

The mod writes `HeadTracking.ini` next to the EXE on first launch.

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

Every one of these keys is remappable through `[Hotkeys]` in `HeadTracking.ini`, both the nav-cluster key and the chord letter, which is worth doing if your button box or a wheel plugin already sits on one of them.

## Configuration

`HeadTracking.ini` sits next to `AssettoCorsaEVO.exe`. Edit it and restart the game to apply. The defaults, annotated with the accepted ranges:

```ini
[Network]
UdpPort=4242

[General]
EnableOnStartup=1

[Hotkeys]
; Windows virtual key codes, in hex. Each action has a nav-cluster key and a
; Ctrl+Shift+<key> chord, and both fire it - remap either or both.
ToggleKey=0x23
CycleModeKey=0x21
ChordToggleKey=0x59
ChordCycleModeKey=0x47

[Rotation]
; 0.1 - 3.0. Higher turns the view further for the same head movement.
YawSensitivity=1.0
PitchSensitivity=1.0
RollSensitivity=1.0
InvertYaw=0
InvertPitch=0
InvertRoll=0
; Smoothing covers rotation and position alike, and the value used is picked
; per connection from where the tracker sends from. 0.0 none .. 1.0 heavy.
; LocalSmoothing: tracker running on this machine (loopback). Nothing floors it,
; so 0.0 really is zero-latency tracking.
; RemoteSmoothing: tracker is a device on the network, e.g. a phone over WiFi.
LocalSmoothing=0.0
RemoteSmoothing=0.15

[Position]
Enabled=1
SensitivityX=1.0
SensitivityY=1.0
SensitivityZ=1.0
InvertX=0
InvertY=0
; InvertZ is for a tracker that sends depth backwards, not for a lean that
; feels reversed. It is applied before the LimitZ / LimitZBack clamp, so
; turning it on also swaps the travel budgets to 0.10m forward and 0.40m back.
InvertZ=0
; Travel limits in metres. Z is asymmetric: more room to lean forward
; toward the windscreen than back into the seat.
LimitX=0.30
LimitY=0.20
LimitZ=0.40
LimitZBack=0.10
```

Hotkeys are codes, not key names: `ToggleKey=Insert` is refused, `ToggleKey=0x2D` is the same key. They are read as hex, so a bare `24` is `0x24`. Common ones are `Home` `0x24`, `End` `0x23`, `Insert` `0x2D`, `Delete` `0x2E`, `Page Up` `0x21`, `Page Down` `0x22`, `F1`-`F12` `0x70`-`0x7B`, `A`-`Z` `0x41`-`0x5A`, numpad `0`-`9` `0x60`-`0x69`; the [full list](https://learn.microsoft.com/windows/win32/inputdev/virtual-key-codes) is Microsoft's. `Ctrl`, `Shift` and `Alt` cannot be bound - they are what the chord itself is made of. A code the mod refuses leaves that action on its previous key and says so in the log; the log also names every key it ended up bound to, so check there first if a remap did not take.

## Troubleshooting

**Mod not loading.**

- Check `HeadTracking.log` next to the game EXE. It records whether the loader attached, whether the build profile matched, and whether the camera hooks landed. Each launch starts a fresh log and moves the previous one to `HeadTracking.prev.log`, so it never grows without bound.
- No log file at all means the ASI loader is not attaching. Confirm `dinput8.dll` is in the same folder as `AssettoCorsaEVO.exe`.
- If the log says the build is newer than the mod knows about, Assetto Corsa EVO has patched and the mod has not been updated for that build yet. It stays dormant on purpose. Check the releases page for a newer release.

**No tracking response.**

- Confirm your tracker is running and its output is UDP to `127.0.0.1` port `4242`, matching `UdpPort` in `HeadTracking.ini`.
- Press `End` to make sure tracking is not toggled off.
- A phone app must target your PC's LAN IP, not `127.0.0.1`, and your firewall must allow inbound UDP on `4242`.
- Another game still running with a head tracking mod holds the port, and the log says `Failed to bind UDP port 4242`. The mod keeps retrying for as long as it is loaded, so close the other game and tracking starts within a second, logging `Bound UDP port 4242 after 12s of waiting - tracking is live`. There is no need to restart Assetto Corsa EVO.

**Jittery or unstable tracking.**

- Raise the smoothing value your tracker actually uses: `LocalSmoothing` if it runs on this PC, `RemoteSmoothing` if it is a phone or other device on the network. Start at `0.3`. The log line printed when a tracker connects says which of the two is in effect.
- Webcam trackers need even lighting and a clear view of your face; a dark room or a strong backlight makes the pose wander.
- On Wi-Fi, a phone app on the 5 GHz band is far steadier than 2.4 GHz.

**Wrong rotation axis or the view drifts off centre.**

- Centre in your tracker app while sitting in your normal driving position. The mod applies the pose it is sent as absolute and keeps no centre of its own.
- If an axis moves the wrong way, set the matching `InvertYaw`, `InvertPitch` or `InvertRoll` to `1`.

**The view keeps following my head in the pause menu.**

- The mod reads the session state from the game's own telemetry page. The log records every change (`[sim] session paused - head tracking held`). If the log says `head tracking will not pause with the game`, that page could not be mapped; report it with the log.

## Updating

Download the new release and run `install.cmd` again. Your `HeadTracking.ini` is preserved.

## Uninstalling

Run `uninstall.cmd`. This removes `AssettoCorsaEvoHeadTracking.asi` along with `HeadTracking.log` and `HeadTracking.prev.log`. The Ultimate ASI Loader is only removed if the installer put it there; use `uninstall.cmd /force` to remove it anyway. `HeadTracking.ini` is left in place, so delete it by hand if you want the folder completely clean.

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
