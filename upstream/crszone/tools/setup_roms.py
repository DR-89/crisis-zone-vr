#!/usr/bin/env python3
"""setup_roms.py [ZIP-or-FOLDER ...] -- unpack MAME's crszone.zip (and namco_csz1.zip, if the I/O board's program is there) into
extracted/, every chip checked by name, size and MAME's CRC32. A source build needs this before building: the two H8 programs are
translated from the ROM at build time. (The game itself does the same unpack on its first start: ../tc2/src/tc2_rom.c with src/csz_game.c's chip list.)
  --check   exit 0 if extracted/ is already complete"""
import os, sys, zipfile, zlib
CHIPS = {"cszo4verb.ic4": (0x800000, 0x6192533d), "cszo3verb.ic1": (0x80000, 0xc790743b), "csz1prg0a.8f": (0x20000, 0x8edc36b3), "csz1mtah.2j": (0x800000, 0x66b076ad), "csz1mtal.2h": (0x800000, 0x38dc639a), "csz1mtbh.2m": (0x800000, 0xbdec4188), "csz1mtbl.2f": (0x800000, 0x9c8f8d7a), "csz1cgll.4m": (0x800000, 0x0bcd41f2), "csz1cglm.4k": (0x800000, 0xd4af93d1), "csz1cgum.4j": (0x800000, 0x913c98b5), "csz1cguu.5f": (0x800000, 0xe1d1bf24), "csz1ccrl.7f": (0x400000, 0x1c20768d), "csz1ccrh.7e": (0x200000, 0xbc2fa03c), "csz1pt0h.7a": (0x400000, 0xe82f1abb), "csz1pt0l.7c": (0x400000, 0xb0d66afe), "csz1pt1h.5a": (0x400000, 0xe54f80ad), "csz1pt1l.5c": (0x400000, 0x527171c8), "csz1pt2h.4a": (0x400000, 0xe295137a), "csz1pt2l.4c": (0x400000, 0xc87d6dbd), "csz1pt3h.3a": (0x400000, 0x05f65bdf), "csz1pt3l.3c": (0x400000, 0x5d077c0f), "csz1wavel.2c": (0x800000, 0xd0d74132), "csz1waveh.2a": (0x800000, 0xde9d14a8)}
DEST = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "extracted")
def complete():
    return all(os.path.isfile(os.path.join(DEST, n)) and os.path.getsize(os.path.join(DEST, n)) == s for n, (s, _) in CHIPS.items())
args = sys.argv[1:]
if args == ["--check"]: sys.exit(0 if complete() else 1)
if not args: args = [p for p in ("crszone.zip", "namco_csz1.zip", "roms", os.path.expanduser("~/Downloads")) if os.path.exists(p)]
os.makedirs(DEST, exist_ok=True)
got = {}
def take(name, data):
    n = name.lower().split("/")[-1]
    if n in CHIPS and n not in got:
        size, crc = CHIPS[n]
        if len(data) != size: print(f"  {n}: {len(data)} bytes, expected {size} -- skipped"); return
        if zlib.crc32(data) & 0xFFFFFFFF != crc: print(f"  {n}: CRC32 {zlib.crc32(data) & 0xFFFFFFFF:08x}, MAME's is {crc:08x} -- a different version or a bad dump, skipped"); return
        open(os.path.join(DEST, n), "wb").write(data); got[n] = 1
for a in args:
    paths = [os.path.join(a, f) for f in sorted(os.listdir(a)) if f.lower() in ("crszone.zip", "namco_csz1.zip")] if os.path.isdir(a) else [a]
    for p in paths:
        try:
            with zipfile.ZipFile(p) as z:
                for i in z.infolist():
                    if not i.is_dir(): take(i.filename, z.read(i))
        except (zipfile.BadZipFile, OSError) as e: print(f"{p}: {e}")
if complete(): print(f"ROMs ready in {DEST} ({len(CHIPS)} chips)"); sys.exit(0)
print("Still missing: " + ", ".join(n for n in CHIPS if not os.path.isfile(os.path.join(DEST, n))))
print("Give the path to MAME's crszone.zip (Crisis Zone, World CSZO4 Ver. B):  python3 tools/setup_roms.py /path/to/crszone.zip"); sys.exit(1)
