# Decompile functions at the RVAs listed in DECOMP_RVAS and print the C.
# Usage: pixi run ghidra-script decompile_at
#
# RVAs are relative to the image base (0x140000000 for AssettoCorsaEVO.exe).
# Edit the list, re-run. Kept in-repo because the camera-side rederive after a
# patch starts here.
# @category CameraUnlock

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

DECOMP_RVAS = [
    0x1A4B0B0,  # ExternalDampedCamera compute (chase cam)
]

base = currentProgram.getImageBase()
decomp = DecompInterface()
decomp.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

for rva in DECOMP_RVAS:
    addr = base.add(rva)
    fn = getFunctionContaining(addr)
    if fn is None:
        fn = createFunction(addr, None)
    if fn is None:
        print("=== RVA 0x%X: no function at %s ===" % (rva, addr))
        continue
    print("=== RVA 0x%X  %s  (%s) ===" % (rva, fn.getName(), fn.getEntryPoint()))
    res = decomp.decompileFunction(fn, 120, monitor)
    if res.decompileCompleted():
        print(res.getDecompiledFunction().getC())
    else:
        print("decompile failed: %s" % res.getErrorMessage())

decomp.dispose()
