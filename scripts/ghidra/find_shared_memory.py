# Locate the acevo_pmf_* shared memory pages and decompile whoever creates and
# fills them, to pin down the graphics page layout (status / session fields).
# Usage: pixi run ghidra-script find_shared_memory
# @category CameraUnlock

import sys

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

OUT_PATH = r"C:\temp\ACEvo\shared_memory.txt"
out = open(OUT_PATH, "w")


def emit(line):
    print(line)
    out.write(line + "\n")
    out.flush()


NEEDLES = [
    "acevo_pmf_graphics",
    "acevo_pmf_physics",
    "acevo_pmf_static",
]

base = currentProgram.getImageBase()
decomp = DecompInterface()
decomp.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

seen_functions = set()

for needle in NEEDLES:
    emit("=== %s ===" % needle)
    for addr in findBytes(None, needle, 8):
        emit("  at %s (rva 0x%X)" % (addr, addr.subtract(base)))
        refs = getReferencesTo(addr)
        if not refs:
            emit("      (no xrefs - referenced by computed address?)")
        for ref in refs:
            src = ref.getFromAddress()
            fn = getFunctionContaining(src)
            if fn is None:
                emit("      xref from %s - no function" % src)
                continue
            entry = fn.getEntryPoint()
            emit("      xref from %s in %s (entry rva 0x%X)"
                 % (src, fn.getName(), entry.subtract(base)))
            seen_functions.add(fn)

for fn in seen_functions:
    emit("")
    emit("======== %s (entry rva 0x%X) ========"
         % (fn.getName(), fn.getEntryPoint().subtract(base)))
    res = decomp.decompileFunction(fn, 180, monitor)
    if res.decompileCompleted():
        emit(res.getDecompiledFunction().getC())
    else:
        emit("decompile failed: %s" % res.getErrorMessage())

decomp.dispose()
out.close()
sys.stdout.flush()
