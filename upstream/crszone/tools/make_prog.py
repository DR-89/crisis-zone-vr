#!/usr/bin/env python3
"""make_prog.py -- Crisis Zone's program images from the ROM files (no MAME needed).
cszo4verb.ic4 (8 MB) is stored WORD-SWAPPED (MAME: ROM_LOAD16_WORD_SWAP into "user1", mapped at 0x1F800000). The R4650's reset vector
0x1FC00000 is therefore user1 + 0x400000: the boot image (4 MB) is the SECOND half of the swapped file. As on Time Crisis 2 the boot image
holds the NMON boot monitor ("NMON 1.0.10 P for SYSTEM23 EV2") and, at 0x20000, the game as a MIPS ELF; the layout below is read from the
ELF's own program headers, not assumed.
  python3 tools/make_prog.py [extracted]  ->  user1.bin (8 MB), boot.bin (4 MB at 0x1FC00000), prog.bin (text+rodata, load at 0x80000000),
                                              sub.bin (H8/3002, word-swapped back), io.bin (the I/O board's H8/3334, csz1prg0a.8f)"""
import sys, os, struct
d = sys.argv[1] if len(sys.argv) > 1 else 'extracted'
rd = lambda n: open(os.path.join(d, n), 'rb').read()
def unswap(raw):
    out = bytearray(len(raw)); out[0::2] = raw[1::2]; out[1::2] = raw[0::2]; return out
user1 = unswap(rd('cszo4verb.ic4'))
boot = user1[0x400000:]
open('user1.bin', 'wb').write(user1)
open('boot.bin', 'wb').write(boot)
E = 0x20000
assert boot[E:E + 4] == b'\x7fELF', 'no ELF at boot.bin 0x20000'
entry, phoff = struct.unpack_from('>II', boot, E + 0x18)
phnum, = struct.unpack_from('>H', boot, E + 0x2C)
load = [struct.unpack_from('>8I', boot, E + phoff + 32 * k) for k in range(phnum)]
load = [p for p in load if p[0] == 1]                       # PT_LOAD: (type, off, vaddr, paddr, filesz, memsz, flags, align)
text = load[0]
assert text[2] == 0x80000000, 'text segment not at 0x80000000'
open('prog.bin', 'wb').write(boot[E + text[1]:E + text[1] + text[4]])
open('sub.bin', 'wb').write(unswap(rd('cszo3verb.ic1')))
io = bytearray(rd('csz1prg0a.8f'))
# MAME'S PATCH, KEPT (namco_csz1: ROM_FILL(0x04ed, 1, 0x05) // HACK: Increase timeout from 4): the I/O board masks every switch -- trigger,
# pedal -- when its main loop runs `limit` passes without a JVS byte from the sub-CPU (0x04F4..0x053C). The loop runs about once a frame and
# the sub-CPU's POST data check polls only every 3-4 frames (its A/D conversions wait for ADTRG = vblank, MAME's wiring), so 4 fires at frame
# ~467 and the gun is dead for good. The real board's ADTRG timing is unknown; 5 is the value MAME ships. ../src/csz_game.c does the same.
assert io[0x4EC:0x4EE] == b'\xf8\x04', 'csz1prg0a.8f: the timeout immediate is not where MAME patches it'
io[0x4ED] = 0x05
open('io.bin', 'wb').write(io)
print('boot.bin %d bytes; ELF entry %08X' % (len(boot), entry))
for p in load:
    print('  PT_LOAD vaddr %08X  boot.bin 0x%06X  filesz 0x%06X  memsz 0x%06X  %s' % (p[2], E + p[1], p[4], p[5], 'RX' if p[6] & 1 else 'RW'))
print('prog.bin %d bytes (0x80000000..0x%08X), sub.bin, io.bin' % (text[4], 0x80000000 + text[4] - 1))
