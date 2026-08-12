# Find who fills the 0x1324-byte acevo_pmf_graphics page, so the offset of its
# status field can be pinned rather than assumed from AC1/ACC.
# Usage: pixi run ghidra-script find_graphics_writer
# @category CameraUnlock

import sys

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

OUT_PATH = r"C:\temp\ACEvo\graphics_writer.txt"
out = open(OUT_PATH, "w")


def emit(line):
    print(line)
    out.write(line + "\n")
    out.flush()


PAGE_SIZE = 0x1324

base = currentProgram.getImageBase()
listing = currentProgram.getListing()

owners = {}
for instr in listing.getInstructions(True):
    for i in range(instr.getNumOperands()):
        for obj in instr.getOpObjects(i):
            try:
                value = obj.getValue()
            except AttributeError:
                continue
            if value == PAGE_SIZE:
                fn = getFunctionContaining(instr.getAddress())
                key = fn.getEntryPoint() if fn else instr.getAddress()
                owners.setdefault(key, []).append((instr.getAddress(), str(instr)))

emit("functions referencing the 0x%X page size: %d" % (PAGE_SIZE, len(owners)))
for key in sorted(owners, key=lambda a: a.getOffset()):
    fn = getFunctionContaining(key)
    name = fn.getName() if fn else "(no function)"
    emit("  %s rva=0x%X  %s" % (name, key.subtract(base), owners[key][0][1]))

decomp = DecompInterface()
decomp.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

for key in sorted(owners, key=lambda a: a.getOffset()):
    fn = getFunctionContaining(key)
    if fn is None:
        continue
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
