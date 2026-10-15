#!/bin/bash
# tools/export_public.sh DEST -- the PUBLIC source tree of Crisis Zone: what a player needs to build and play it from their own crszone.zip
# (beside ../tc2's export: Crisis Zone builds against the System 23 host there), and nothing else -- no ROM data, no Ghidra project,
# no MAME traces or disassembly (cov/*.dasm), no scratch. Included: the game program as translated C (gen/crszone_lifted_*.c: made with
# Ghidra, which a player should not need), the H8 coverage ADDRESSES the build-time translator reads (cov/h8*.pcs/.calls/.jumps: numbers
# only), the sources and the tools the build runs.
set -e
DEST=${1:?usage: tools/export_public.sh DEST}
cd "$(dirname "$0")/.."
mkdir -p "$DEST"
FILES=(CMakeLists.txt build.sh launch.sh build-windows.sh README.md
       tools/make_prog.py tools/setup_roms.py tools/export_public.sh
       include/tc2_lifted.h gen/crszone_lifted.h gen/crszone_lifted_tab.c src/csz_game.c)
for f in gen/crszone_lifted_[0-9][0-9].c cov/h8sub_*.pcs cov/h8sub_*.calls cov/h8sub_*.jumps cov/h8io_*.pcs cov/h8io_*.calls cov/h8io_*.jumps; do
    [ -e "$f" ] && FILES+=("$f")
done
ls gen/crszone_lifted_[0-9][0-9].c >/dev/null 2>&1 || { echo "gen/crszone_lifted_NN.c missing: run the lift first (CMakeLists.txt)"; exit 1; }
for f in "${FILES[@]}"; do
    [ -f "$f" ] || { echo "missing: $f"; exit 1; }
    mkdir -p "$DEST/$(dirname "$f")"; cp -p "$f" "$DEST/$f"
done
[ -f ../LICENSE ] && cp -p ../LICENSE "$DEST/LICENSE"
cat > "$DEST/.gitignore" <<'IGN'
# Game files are yours and never belong in git.
/extracted/
/roms/
*.zip
# Made by the build or the game.
/build/
/build-win/
/windows-release/
/boot.bin
/prog.bin
/user1.bin
/sub.bin
/io.bin
/gen/csz_sub.c
/gen/csz_io.c
/crszone.cfg
/tc2.log
/crszone_backup.nv
/crszone_backup.nv.tmp
/crszone_last.rec
/crszone_prev.rec
/screenshots/
/crosshairs/
__pycache__/
IGN
echo "Exported ${#FILES[@]} files to $DEST"
