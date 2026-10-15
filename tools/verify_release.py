"""Audit packaged chips, Android identity/ABI and portable Windows content."""
from pathlib import Path
import argparse, hashlib, json, struct, subprocess, zipfile
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
def sha(data): return hashlib.sha256(data).hexdigest()
def verify_apk(apk):
    audit=json.loads((ROOT/'build/rom-audit.json').read_text())
    with zipfile.ZipFile(apk) as z:
        assert z.testzip() is None
        assert len(z.namelist())==len(set(z.namelist()))
        manifest=dict(line.split('  ',1)[::-1] for line in z.read('assets/roms.sha256').decode().splitlines())
        assert len(manifest)==23
        assert set(manifest)=={c['file'] for c in audit['chips']}
        for chip in audit['chips']:
            data=z.read('assets/roms/'+chip['file'])
            assert len(data)==chip['bytes'] and sha(data)==chip['sha256']==manifest[chip['file']]
        assert {n for n in z.namelist() if n.startswith('assets/roms/')}=={'assets/roms/'+n for n in manifest}
        for line in z.read('assets/models.sha256').decode().splitlines():
            digest,name=line.split('  ',1);assert sha(z.read('assets/models/'+name))==digest
        assert z.read('assets/models/machine-gun.tcgun')==(ROOT/'quest/assets/models/machine-gun.tcgun').read_bytes()
        assert z.read('assets/models/namco-gun.tcgun')==(ROOT/'quest/assets/models/namco-gun.tcgun').read_bytes()
        libs=sorted(n for n in z.namelist() if n.startswith('lib/'))
        assert libs==['lib/arm64-v8a/libSDL2.so','lib/arm64-v8a/libmain.so','lib/arm64-v8a/libopenxr_loader.so']
        for name in libs:
            elf=z.read(name);assert elf[:5]==b'\x7fELF\x02' and struct.unpack_from('<H',elf,18)[0]==183
        assert z.read('classes.dex')[:4]==b'dex\n'
        assert b'Crisis Zone' in z.read('assets/licenses/NOTICE.md')
    aapt=ROOT/'.tools/android-sdk/build-tools/35.0.0/aapt.exe'
    badging=subprocess.check_output([str(aapt),'dump','badging',str(apk)],text=True)
    source=ET.parse(ROOT/'quest/AndroidManifest.xml').getroot();ns='{http://schemas.android.com/apk/res/android}'
    assert f"package: name='org.crisiszone.vr' versionCode='{source.get(ns+'versionCode')}' versionName='{source.get(ns+'versionName')}'" in badging
    assert "launchable-activity: name='org.crisiszone.vr.MainActivity'" in badging
    tree=subprocess.check_output([str(aapt),'dump','xmltree',str(apk),'AndroidManifest.xml'],text=True)
    assert 'LauncherActivity' not in tree
    assert 'org.khronos.openxr.intent.category.IMMERSIVE_HMD' in tree
    assert 'com.oculus.intent.category.VR' in tree
    assert 'android.permission.INTERNET' not in tree
    return {'apk':apk.name,'bytes':apk.stat().st_size,'sha256':sha(apk.read_bytes()),'chips_verified':23,'bundled':True,'importer':False,'abi':'arm64-v8a'}
def verify_windows(archive):
    audit=json.loads((ROOT/'build/rom-audit.json').read_text())
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        prefix='CrisisZoneVR-PC/'
        for chip in audit['chips']:
            assert sha(z.read(prefix+'extracted/'+chip['file']))==chip['sha256']
        for line in z.read(prefix+'SHA256SUMS.txt').decode().splitlines():
            digest,name=line.split('  ',1);assert sha(z.read(prefix+name))==digest
        assert z.read(prefix+'CrisisZoneVR.exe')[:2]==b'MZ'
        assert z.read(prefix+'openxr_loader.dll')[:2]==b'MZ'
        assert z.read(prefix+'models/machine-gun.tcgun')==(ROOT/'quest/assets/models/machine-gun.tcgun').read_bytes()
        assert z.read(prefix+'models/namco-gun.tcgun')==(ROOT/'quest/assets/models/namco-gun.tcgun').read_bytes()
        assert b'CrisisZoneVR.exe' in z.read(prefix+'Play VR.cmd')
        assert b'--desktop' in z.read(prefix+'Play Desktop.cmd')
        assert not any(n.endswith(('.log','.rec','.nv','.cfg','.keystore')) for n in z.namelist())
    return {'windows_zip':archive.name,'bytes':archive.stat().st_size,'sha256':sha(archive.read_bytes()),'chips_verified':23,'bundled':True}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--apk',type=Path);p.add_argument('--windows',type=Path);a=p.parse_args()
    reports=[]
    if a.apk:reports.append(verify_apk(a.apk))
    if a.windows:reports.append(verify_windows(a.windows))
    assert reports,'Select a release package to verify'
    (ROOT/'build/release-verification.json').write_text(json.dumps(reports,indent=2)+'\n')
    print(json.dumps(reports,indent=2))
