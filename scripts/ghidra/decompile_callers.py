# Decompile every caller of the functions listed in TARGET_RVAS.
# Usage: pixi run ghidra-script decompile_callers
# @category CameraUnlock

import sys

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

OUT_PATH = r"C:\temp\ACEvo\callers.txt"
out = open(OUT_PATH, "w")


def emit(line):
    print(line)
    out.write(line + "\n")
    out.flush()


TARGET_RVAS = [
    0x11C95D,  # thunk to the graphics page begin-frame
]

base = currentProgram.getImageBase()
decomp = DecompInterface()
decomp.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

callers = set()
for rva in TARGET_RVAS:
    addr = base.add(rva)
    emit("=== callers of rva 0x%X ===" % rva)
    for ref in getReferencesTo(addr):
        src = ref.getFromAddress()
        fn = getFunctionContaining(src)
        if fn is None:
            emit("  %s - no function" % src)
            continue
        emit("  %s in %s (entry rva 0x%X)"
             % (src, fn.getName(), fn.getEntryPoint().subtract(base)))
        callers.add(fn)

for fn in callers:
    emit("")
    emit("======== %s (entry rva 0x%X) ========"
         % (fn.getName(), fn.getEntryPoint().subtract(base)))
    res = decomp.decompileFunction(fn, 240, monitor)
    if res.decompileCompleted():
        emit(res.getDecompiledFunction().getC())
    else:
        emit("decompile failed: %s" % res.getErrorMessage())

decomp.dispose()
out.close()
sys.stdout.flush()
