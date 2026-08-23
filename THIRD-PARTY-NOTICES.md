# Third-Party Notices

## Scope of the MIT licence

The MIT licence in LICENSE covers this mod's own source and the binaries built
from it. It does not extend to the third-party components below, nor to the
Assetto Corsa EVO gameplay capture in `assets/`. Each of those keeps its own
licence and copyright, reproduced or attributed here.

## Ultimate ASI Loader

- **Version:** v9.7.2 (`ab722befd52581a34449b603926cfab476e66b05`)
- **License:** MIT
- **Upstream:** https://github.com/ThirteenAG/Ultimate-ASI-Loader
- **Usage:** Loads `AssettoCorsaEvoHeadTracking.asi` into the game as the
  `dinput8.dll` proxy.
- **Bundled:** yes. Shipped unmodified as `vendor/ultimate-asi-loader/dinput8.dll`,
  bundled in the release ZIP and used as the install-time source. The upstream
  licence text travels with it at `vendor/ultimate-asi-loader/LICENSE`.

Copyright (c) 2023 ThirteenAG

---

## MinHook

- **Version:** v1.3.3 (`9fbd087432700d73fc571118d6a9697a36443d88`)
- **License:** BSD-2-Clause
- **Upstream:** https://github.com/TsudaKageyu/minhook
- **Usage:** Installs the camera function hooks at runtime.
- **Bundled:** yes. Compiled from source and statically linked into
  `AssettoCorsaEvoHeadTracking.asi`, which is a binary redistribution, so the
  full licence text follows. It also carries Vyacheslav Patkov's Hacker
  Disassembler Engine, whose separate notice is reproduced below it.

```
MinHook - The Minimalistic API Hooking Library for x64/x86
Copyright (C) 2009-2017 Tsuda Kageyu.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

## Hacker Disassembler Engine 32 / 64 C

- **License:** BSD-2-Clause
- **Usage:** The length-disassembler MinHook uses to build its trampolines.
- **Bundled:** yes, as part of MinHook, statically linked into
  `AssettoCorsaEvoHeadTracking.asi`.

```
Hacker Disassembler Engine 32 C
Hacker Disassembler Engine 64 C
Copyright (c) 2008-2009, Vyacheslav Patkov.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

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

- **Version:** `3465659888b2270addac9de0b2a728f59a00360c`
- **License:** MIT
- **Upstream:** https://github.com/itsloopyo/cameraunlock-core
- **Usage:** Shared tracking pipeline, hooking and discovery helpers.
- **Bundled:** yes. Statically linked into `AssettoCorsaEvoHeadTracking.asi`.

Copyright (c) 2026 CameraUnlock

---

## Assetto Corsa EVO gameplay capture

- **File:** `assets/readme-clip.gif`
- **Rights holder:** KUNOS Simulazioni S.r.l., together with the rights holders
  of the vehicles, circuits and sponsor marks that appear in the frame.
- **Usage:** a recording of the game running with this mod, made on a
  legitimately purchased copy, shown in the README so a reader can see what the
  mod does before installing it.
- **Bundled:** no. It lives in this repository only; the packaging script ships
  no part of `assets/`, so it is in neither the installer ZIP nor anything the
  launcher deploys.
- **Licence:** none is granted or implied by this repository. It is not covered
  by the MIT licence in LICENSE, and nothing here permits reuse of it.
  Rights holders who would rather it were not published: open an issue or reach
  us on Discord and it comes down.

---

Assetto Corsa EVO is a trademark of KUNOS Simulazioni S.r.l. This mod is an
unofficial, unaffiliated community project. It contains no game code and
redistributes no game files: everything in the release ZIP is either this mod's
own build output or the Ultimate ASI Loader above. The gameplay capture is the
only game-derived material anywhere in this repository, and it is attributed
above rather than licensed.
