# Crisis Zone for PC

Namco's 1999 arcade light-gun shooter **Crisis Zone** (System 23 Evolution 2), rebuilt to run natively on a PC on the same System 23 host
as Time Crisis 2 (`../tc2`): the game's own programs -- the MIPS main CPU, the H8 sub CPU and the I/O board's H8 -- are translated to C,
and the hardware around them is our own code. Nothing interprets ROM code; MAME was used only as a reference.

**No game data is included.** You need your own copy of MAME's ROM set `crszone.zip` (Crisis Zone, World CSZO4 Ver. B).

## Building from source (Linux)

```bash
./build.sh /path/to/crszone.zip      # unpacks and checks the ROM, translates the two H8 programs, builds
./launch.sh                          # play (./launch.sh 2 = a 2x window, --fullscreen)
```
The keys and the light gun are Time Crisis 2's: [HOW TO PLAY](../tc2/packaging/linux/HOW-TO-PLAY.txt). The game program is ~170 MB of
generated C (`gen/crszone_lifted_*.c`): allow about 1 GB of memory per parallel compile (`JOBS=2 ./build.sh ...`).
Windows: `./build-windows.sh` (MinGW-w64 cross compiler) after `./build.sh`.

Custom crosshair: put PNG files (8-bit, up to 256 x 256) in a `crosshairs/` folder beside the program and pick one in
Escape -> Display -> Crosshair image.
