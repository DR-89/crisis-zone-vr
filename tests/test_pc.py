"""Verify the shipped Windows ZIP and smoke-test a clean extraction."""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, tempfile, zipfile
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
version=ET.parse(ROOT/'quest/AndroidManifest.xml').getroot().get('{http://schemas.android.com/apk/res/android}versionName')
p=argparse.ArgumentParser()
p.add_argument('--zip',type=Path,default=ROOT/f'artifacts/releases/v{version}/CrisisZoneVR-v{version}-windows-x64.zip')
p.add_argument('--capture-frame',type=int,default=3600)
a=p.parse_args()
assert a.capture_frame>0
frames=a.capture_frame+1
with tempfile.TemporaryDirectory(prefix='crisiszone-release-test-') as directory:
    temp=Path(directory)
    with zipfile.ZipFile(a.zip) as z:
        assert z.testzip() is None
        assert not any(n.endswith(('.inputs','.rec','.log','.nv','.cfg','.keystore')) for n in z.namelist()),'Private state in release'
        z.extractall(temp)
    game=temp/'CrisisZoneVR-PC'
    for line in (game/'SHA256SUMS.txt').read_text().splitlines():
        digest,name=line.split('  ',1)
        assert hashlib.sha256((game/name).read_bytes()).hexdigest()==digest,name
    info=json.loads((game/'build-info.json').read_text())
    assert info['version']==version
    assert hashlib.sha256((game/'CrisisZoneVR.exe').read_bytes()).hexdigest()==info['exe_sha256']
    assert 'Martoscar' in (game/'NOTICE.md').read_text(encoding='utf-8')
    assert 'patreon.com/c/OpenSourceDevDirk' in (game/'README.md').read_text(encoding='utf-8')
    env=os.environ.copy()
    env.update(TCVR_FAST='1',TCVR_TEST_HIDDEN='1',TCVR_CAPTURE_FRAME=str(a.capture_frame),SDL_AUDIODRIVER='dummy')
    startup=subprocess.STARTUPINFO()
    startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    subprocess.run([game/'CrisisZoneVR.exe','--desktop','--no-dialog','--frames',str(frames)],
                   env=env,startupinfo=startup,check=True,timeout=90)
    log=(game/'tc2.log').read_text()
    assert '[OPTIONS] laser ON' in log
    (ROOT/'build/release-pc-smoke.log').write_text(log)
    assert f'normal shutdown at frame {frames}' in log and '0 traps, 0 FP exceptions' in log
    assert 'models/namco-gun.tcgun loaded: 13149 vertices, 17108 triangles' in log
    assert not re.search(r'\[TRAP\]|\[GL\].*error|Shader:|fault [1-9]',log)
    capture=(game/'eye-0.ppm').read_bytes()
    (ROOT/f'build/release-pc-frame-{a.capture_frame}.ppm').write_bytes(capture)
    image=capture.split(b'\n',3)[3]
    print(f'Capture {a.capture_frame}: {len(image)} RGB bytes; {sum(v>20 for v in image)} channels above 20')
    assert len(image)==1280*960*3 and sum(v>20 for v in image)>len(image)//30,'Blank desktop render'
    print(f'PASS: clean Crisis Zone Windows ZIP, version/hashes, attribution, Patreon, desktop GL and {frames} frames')
