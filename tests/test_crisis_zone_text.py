"""Validate the real text renderer against locally captured menu/game frames."""
from pathlib import Path
import argparse,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('frames',type=Path);a=p.parse_args()
frames=sorted(a.frames.glob('frame-*.bin'));assert frames,'Captured menu and gameplay frames are required'
cc=ROOT/'.tools/llvm-mingw/llvm-mingw-20260922-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
with tempfile.TemporaryDirectory(prefix='crisis-zone-text-') as temp:
    exe=Path(temp)/'text.exe'
    subprocess.run([str(cc),'-std=c11','-O1','-I'+str(ROOT/'quest'),'-I'+str(ROOT/'upstream/tc2/src'),str(ROOT/'tests/crisis_zone_text_fixture.c'),str(ROOT/'quest/crisis_zone_text.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe),*map(str,frames)],check=True)
