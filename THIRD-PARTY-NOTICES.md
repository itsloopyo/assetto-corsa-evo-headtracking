# Third-Party Notices

This mod itself is MIT licensed (copyright itsloopyo, see LICENSE). It bundles
or links the following third-party components.

## Ultimate ASI Loader

- **Version:** v9.7.2 (`ab722befd52581a34449b603926cfab476e66b05`)
- **License:** MIT
- **Upstream:** https://github.com/ThirteenAG/Ultimate-ASI-Loader
- **Usage:** Loads `AssettoCorsaEvoHeadTracking.asi` into the game as the
  `dinput8.dll` proxy.
- **Bundled:** yes. Shipped as `vendor/ultimate-asi-loader/dinput8.dll`, bundled
  in the release ZIP and used as the install-time source.

Copyright (c) 2023 ThirteenAG

---

## MinHook

- **Version:** v1.3.3 (`9fbd087432700d73fc571118d6a9697a36443d88`)
- **License:** BSD-2-Clause
- **Upstream:** https://github.com/TsudaKageyu/minhook
- **Usage:** Installs the camera function hooks at runtime.
- **Bundled:** yes. Statically linked into `AssettoCorsaEvoHeadTracking.asi`.

Copyright (c) 2009-2017 Tsuda Kageyu

---

## OpenTrack

- **Version:** n/a (wire protocol only)
- **License:** ISC
- **Upstream:** https://github.com/opentrack/opentrack
- **Usage:** Only the UDP wire format is implemented so OpenTrack and compatible
  phone apps can feed the mod; no OpenTrack code is included.
- **Bundled:** no.

---

## CameraUnlock Core

- **Version:** `64f8d685ed6398dddfda0e02f8395b57ed7aa20e`
- **License:** MIT
- **Upstream:** https://github.com/itsloopyo/cameraunlock-core
- **Usage:** Shared tracking pipeline, hooking and discovery helpers.
- **Bundled:** yes. Statically linked into `AssettoCorsaEvoHeadTracking.asi`.

Copyright (c) 2026 itsloopyo

---

Assetto Corsa EVO is a trademark of KUNOS Simulazioni S.r.l. This mod is an
unofficial, unaffiliated community project and contains no game code or assets.
