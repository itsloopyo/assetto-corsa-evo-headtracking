#!/usr/bin/env python3
"""Verify a build profile's pinned camera values against an AssettoCorsaEVO.exe.

check-fingerprint.ps1 answers "does this EXE have a profile"; this answers the
harder half of a post-patch rederive: "do slot and offset still hold on it".

It walks MSVC RTTI in the file on disk - mangled name -> TypeDescriptor ->
RTTICompleteObjectLocator -> vtable - so it needs no Ghidra project and no
running game. For each concrete DrivableCamera it reports the compute slot
target and every access the compute body makes through the `out` pointer, then
compares both against the values the profile pins.

Usage:
    python scripts/verify-camera-profile.py
    python scripts/verify-camera-profile.py --exe "D:\\...\\AssettoCorsaEVO.exe"
    python scripts/verify-camera-profile.py --slot 2 --transform-offset 0x2C

Exit codes: 0 verified, 1 mismatch or could not verify.

Disassembly needs capstone (`pip install capstone`). Without it the RTTI walk
still runs and reports the vtable and slot targets, but the transform offset
cannot be checked and the exit code is 1 - a partial answer must not read as a
pass.
"""

import argparse
import struct
import sys

# The concrete driving views. The abstract DrivableCamera base is included
# because its compute slot must be _purecall - the same slot index resolving to
# a real body on the base would mean the interface had been reshaped.
CONCRETE_CLASSES = [
    "CockpitCamera",
    "DashCamera",
    "BonnetCamera",
    "ExternalFixedCamera",
    "ExternalDampedCamera",
]
BASE_CLASS = "DrivableCamera"

# The compute bodies are split into non-contiguous chunks by hot/cold
# splitting, so the scan follows .pdata unwind chains to collect every chunk
# belonging to the function. A fixed byte window instead of this reads straight
# past the end of the body into unrelated functions and finds their accesses -
# which made an earlier version of this script report a wrong slot as verified.
UNW_FLAG_CHAININFO = 0x4


class Image:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        if d[:2] != b"MZ":
            raise SystemExit("not a PE image: %s" % path)
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        if d[pe:pe + 4] != b"PE\0\0":
            raise SystemExit("not a PE image: %s" % path)
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = pe + 24
        if struct.unpack_from("<H", d, opt)[0] != 0x20B:
            raise SystemExit("not a 64-bit PE: %s" % path)
        # COFF header: +0 signature, +4 Machine, +6 NumberOfSections, +8 TimeDateStamp.
        self.timestamp = struct.unpack_from("<I", d, pe + 8)[0]
        self.image_base = struct.unpack_from("<Q", d, opt + 24)[0]
        self.size_of_image = struct.unpack_from("<I", d, opt + 56)[0]
        self.checksum = struct.unpack_from("<I", d, opt + 64)[0]
        sec = opt + struct.unpack_from("<H", d, pe + 20)[0]
        self.sections = []
        for i in range(nsec):
            o = sec + i * 40
            name = d[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
            vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", d, o + 8)
            self.sections.append((name, vaddr, vsize, raddr, rsize))

    def rva_to_off(self, rva):
        for _, vaddr, vsize, raddr, rsize in self.sections:
            if vaddr <= rva < vaddr + max(vsize, rsize):
                delta = rva - vaddr
                if delta < rsize:
                    return raddr + delta
        return None

    def off_to_rva(self, off):
        for _, vaddr, _, raddr, rsize in self.sections:
            if raddr <= off < raddr + rsize:
                return vaddr + (off - raddr)
        return None

    def u32(self, rva):
        off = self.rva_to_off(rva)
        return struct.unpack_from("<I", self.data, off)[0] if off is not None else None

    def u64(self, rva):
        off = self.rva_to_off(rva)
        return struct.unpack_from("<Q", self.data, off)[0] if off is not None else None

    def pdata(self):
        """Every RUNTIME_FUNCTION, as (begin_rva, end_rva, unwind_rva)."""
        if getattr(self, "_pdata", None) is None:
            self._pdata = []
            for name, _, _, raddr, rsize in self.sections:
                if name != ".pdata":
                    continue
                for i in range(rsize // 12):
                    b, e, u = struct.unpack_from("<III", self.data, raddr + i * 12)
                    if b or e:
                        self._pdata.append((b, e, u))
            self._pdata.sort()
        return self._pdata

    def _primary_of(self, entry):
        """Follow UNW_FLAG_CHAININFO to the chunk that owns the function."""
        seen = set()
        while entry and entry[0] not in seen:
            seen.add(entry[0])
            off = self.rva_to_off(entry[2])
            if off is None:
                return entry
            flags = self.data[off] >> 3
            if not flags & UNW_FLAG_CHAININFO:
                return entry
            count = self.data[off + 2]
            chain = off + 4 + ((count + 1) & ~1) * 2
            b, e, u = struct.unpack_from("<III", self.data, chain)
            entry = (b, e, u)
        return entry

    def function_chunks(self, rva):
        """Every address range belonging to the function containing `rva`."""
        entries = self.pdata()
        owner = None
        for entry in entries:
            if entry[0] <= rva < entry[1]:
                owner = self._primary_of(entry)
                break
        if owner is None:
            return []
        chunks = [(e[0], e[1]) for e in entries if self._primary_of(e)[0] == owner[0]]
        return sorted(chunks)

    def find_all(self, needle):
        out = []
        i = self.data.find(needle)
        while i != -1:
            out.append(i)
            i = self.data.find(needle, i + 1)
        return out


def find_vtable(img, class_name):
    """MSVC RTTI walk. Returns (vtable_rva, typedesc_rva) or None."""
    for name_off in img.find_all((".?AV%s@@" % class_name).encode("ascii")):
        name_rva = img.off_to_rva(name_off)
        if name_rva is None:
            continue
        # x64 TypeDescriptor: { void* pVFTable; void* spare; char name[]; }
        td_rva = name_rva - 0x10
        for ref_off in img.find_all(struct.pack("<I", td_rva)):
            ref_rva = img.off_to_rva(ref_off)
            if ref_rva is None:
                continue
            # COL.pTypeDescriptor is at COL+0x0C; signature is 1 on x64 and
            # COL.pSelf at +0x14 points back at the COL.
            col = ref_rva - 0x0C
            if img.u32(col) != 1 or img.u32(col + 0x14) != col:
                continue
            # The vtable stores its COL pointer one slot ahead of slot 0.
            for holder_off in img.find_all(struct.pack("<Q", img.image_base + col)):
                holder_rva = img.off_to_rva(holder_off)
                if holder_rva is not None:
                    return holder_rva + 8, td_rva
    return None


def follow_thunk(img, rva, md):
    """Vtable slots point into an incremental-link jmp table, not the body."""
    if md is None:
        return rva, False
    off = img.rva_to_off(rva)
    if off is None:
        return rva, False
    for ins in md.disasm(img.data[off:off + 16], img.image_base + rva):
        if ins.mnemonic == "jmp" and ins.op_str.startswith("0x"):
            return int(ins.op_str, 16) - img.image_base, True
        break
    return rva, False


def scan_out_accesses(img, body_rva, md):
    """Find where the compute body stashes `out` (r8), then every access
    through it. Returns (register, chunk_count, [(rva, disp, text)])."""
    chunks = img.function_chunks(body_rva)
    if not chunks:
        return None, 0, []
    saved = None
    sites = []
    for begin, end in chunks:
        off = img.rva_to_off(begin)
        if off is None:
            continue
        for ins in md.disasm(img.data[off:off + (end - begin)], img.image_base + begin):
            ops = ins.op_str
            if saved is None and ins.mnemonic == "mov" and ops.endswith(", r8") \
                    and not ops.startswith(("qword", "dword")):
                saved = ops.split(",")[0].strip()
                continue
            if saved and ("[%s + " % saved) in ops:
                start = ops.index("[%s + " % saved) + len("[%s + " % saved)
                stop = ops.index("]", start)
                text = ops[start:stop]
                try:
                    disp = int(text, 16 if text.startswith("0x") else 10)
                except ValueError:
                    continue
                sites.append((ins.address - img.image_base, disp,
                              "%s %s" % (ins.mnemonic, ops)))
    return saved, len(chunks), sites


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", help="AssettoCorsaEVO.exe (default: detected install)")
    ap.add_argument("--slot", type=lambda s: int(s, 0), default=2,
                    help="expected camera_compute_slot (default 2)")
    ap.add_argument("--transform-offset", type=lambda s: int(s, 0), default=0x2C,
                    help="expected camera_out_transform (default 0x2C)")
    args = ap.parse_args()

    exe = args.exe
    if not exe:
        import subprocess
        here = __file__.replace("\\", "/").rsplit("/", 2)[0]
        ps = ("Import-Module '%s/cameraunlock-core/powershell/GamePathDetection.psm1' "
              "-Force; Find-GamePath -GameId 'assetto-corsa-evo'" % here)
        found = subprocess.run(["powershell", "-NoProfile", "-Command", ps],
                               capture_output=True, text=True).stdout.strip()
        if not found:
            raise SystemExit("Assetto Corsa EVO not found. Pass --exe explicitly.")
        exe = found + "\\AssettoCorsaEVO.exe"

    img = Image(exe)
    print("EXE: %s" % exe)
    print("  TimeDateStamp 0x%08X   SizeOfImage 0x%08X   CheckSum 0x%08X"
          % (img.timestamp, img.size_of_image, img.checksum))
    print("  expecting camera_compute_slot=%d, camera_out_transform=0x%X\n"
          % (args.slot, args.transform_offset))

    try:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_64
        md = Cs(CS_ARCH_X86, CS_MODE_64)
    except ImportError:
        md = None
        print("capstone not installed - the transform offset CANNOT be checked.")
        print("Install it with: pip install capstone\n")

    failures = []

    base = find_vtable(img, BASE_CLASS)
    if base is None:
        failures.append("%s: RTTI vtable not found" % BASE_CLASS)
    else:
        vt, _ = base
        targets = {img.u64(vt + s * 8) for s in range(4)}
        pure = len(targets) == 1
        print("%-22s vtable RVA 0x%-9X slot %d -> %s"
              % (BASE_CLASS, vt, args.slot,
                 "_purecall (abstract, as expected)" if pure else "A REAL BODY - interface reshaped"))
        if not pure:
            failures.append("%s slot %d is no longer _purecall" % (BASE_CLASS, args.slot))
    print()

    for cls in CONCRETE_CLASSES:
        found = find_vtable(img, cls)
        if found is None:
            print("%-22s RTTI vtable NOT FOUND" % cls)
            failures.append("%s: RTTI vtable not found" % cls)
            continue
        vt, _ = found
        slot_rva = img.u64(vt + args.slot * 8)
        if slot_rva is None:
            print("%-22s slot %d unreadable" % (cls, args.slot))
            failures.append("%s: slot %d unreadable" % (cls, args.slot))
            continue
        slot_rva -= img.image_base
        body_rva, thunked = follow_thunk(img, slot_rva, md)

        print("%-22s vtable RVA 0x%-9X slot %d -> 0x%X%s"
              % (cls, vt, args.slot, slot_rva,
                 "  (thunk -> body 0x%X)" % body_rva if thunked else ""))

        if md is None:
            continue
        reg, nchunks, sites = scan_out_accesses(img, body_rva, md)
        if not nchunks:
            print("    no .pdata entry for the body - cannot bound the scan")
            failures.append("%s: body has no .pdata entry" % cls)
            continue
        if reg is None:
            print("    could not find where the body stashes `out` (r8)")
            failures.append("%s: `out` register not identified" % cls)
            continue
        hits = [s for s in sites if s[1] == args.transform_offset]
        if hits:
            rva, _, text = hits[0]
            print("    out in %-4s (%d chunks)  transform at +0x%X confirmed at RVA 0x%X: %s"
                  % (reg, nchunks, args.transform_offset, rva, text))
        else:
            seen = sorted({s[1] for s in sites})
            print("    out in %-4s (%d chunks)  NO access at +0x%X; offsets seen: %s"
                  % (reg, nchunks, args.transform_offset,
                     ", ".join("0x%X" % d for d in seen) or "none"))
            failures.append("%s: no access at out+0x%X" % (cls, args.transform_offset))

    print()
    if md is None:
        print("INCOMPLETE - slot resolved, transform offset unverified (no capstone).")
        return 1
    if failures:
        print("FAILED - the pinned values do not hold on this EXE:")
        for f in failures:
            print("  - %s" % f)
        print("\nRederive before shipping a profile. Do not edit an existing")
        print("profile's numbers; append a new one.")
        return 1
    print("VERIFIED - slot %d and out+0x%X hold across all %d camera classes."
          % (args.slot, args.transform_offset, len(CONCRETE_CLASSES)))
    print("Safe to append a profile with these values for this build.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
