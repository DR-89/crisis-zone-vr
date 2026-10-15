from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
cc=ROOT/'.tools/llvm-mingw/llvm-mingw-20260922-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
with tempfile.TemporaryDirectory(prefix='crisis-zone-scene-') as temp:
    exe=Path(temp)/'scene.exe'
    subprocess.run([str(cc),'-std=c11','-O1','-DTCVR_PC','-I'+str(ROOT/'quest'),'-I'+str(ROOT/'upstream/tc2/src'),str(ROOT/'tests/crisis_zone_scene_fixture.c'),str(ROOT/'quest/tc2_scene.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
