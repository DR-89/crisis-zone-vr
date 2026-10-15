#!/bin/bash
# launch.sh -- Crisis Zone (Namco System 23 Evolution 2) on the shared System 23 host: builds what is missing, then runs from power-on.
#   ./launch.sh                  a window (Esc = the menu)
#   ./launch.sh 2                at 2x            ./launch.sh --fullscreen
#   ./launch.sh --headless N     no window: N frames, screenshots every 300 frames into work/run/shots, the sound into work/run/crszone.wav
# Needs MAME's crszone set unpacked in extracted/ and the lift in gen/ (tools/relift.sh). Memory: the build uses JOBS (default 6) gcc jobs of ~1 GB.
cd "$(dirname "$(readlink -f "$0")")" || exit 1
if [ ! -f extracted/cszo4verb.ic4 ]; then echo "launch.sh: put the crszone ROM files in extracted/ first" >&2; exit 1; fi
if [ ! -f boot.bin ] || [ ! -f sub.bin ] || [ ! -f io.bin ]; then python3 tools/make_prog.py extracted || exit 1; fi
if [ ! -x build/crszone ] || [ -n "$(find src include gen ../tc2/src ../tc2/include ../tc2/tools/h8 CMakeLists.txt ../tc2/host.cmake -newer build/crszone -print -quit 2>/dev/null)" ]; then
    mkdir -p build && (cd build && cmake .. >/dev/null && make -j"${JOBS:-6}" crszone) || exit 1
fi
if [ "$1" = "--headless" ]; then
    n=${2:-1800}; mkdir -p work/run/shots
    TC2_SHOTS=work/run/shots:300 TC2_WAV=work/run/crszone.wav exec build/crszone reset $((n * 3400000))
fi
[ -z "$WAYLAND_DISPLAY" ] && [ -z "$DISPLAY" ] && [ -S /run/user/$(id -u)/wayland-0 ] && export WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/$(id -u)
exec build/crszone --window "$@"
