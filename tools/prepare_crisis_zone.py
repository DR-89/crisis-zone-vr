"""Validate Crisis Zone 0.9.0 ROMs and generate its native H8 programs."""
from pathlib import Path
import argparse, ast, hashlib, json, os, shutil, subprocess, sys, zipfile, zlib
ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'upstream/crszone'
HOST = ROOT / 'upstream/tc2'
def chip_table():
    tree = ast.parse((GAME / 'tools/setup_roms.py').read_text(encoding='utf-8'))
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'CHIPS' for t in n.targets))
def prepare(rom, device=None):
    rom = Path(rom)
    device = Path(device) if device else rom.with_name('namco_csz1.zip')
    chips = chip_table()
    found = {}
    archives = [rom] + ([device] if device.is_file() else [])
    for archive in archives:
        with zipfile.ZipFile(archive) as z:
            for entry in z.infolist():
                if entry.is_dir() or entry.file_size > 8 * 1024 * 1024: continue
                names = [n for n, spec in chips.items() if spec == (entry.file_size, entry.CRC)]
                if not names: continue
                data = z.read(entry)
                for name in names:
                    if (len(data), zlib.crc32(data)) != chips[name]: raise RuntimeError('ROM checksum mismatch: ' + name)
                    found[name] = data
    missing = [n for n in chips if n not in found]
    if missing: raise RuntimeError('Missing Crisis Zone World CSZO4 Ver. B chips: ' + ', '.join(missing))
    extracted = GAME / 'extracted'; extracted.mkdir(exist_ok=True)
    manifest = []
    for name, data in found.items():
        dest = extracted / name
        if not dest.exists() or dest.read_bytes() != data: dest.write_bytes(data)
        manifest.append({'file': name, 'bytes': len(data), 'crc32': f'{zlib.crc32(data):08x}', 'sha256': hashlib.sha256(data).hexdigest()})
    def run(*args, env=None): subprocess.run([str(x) for x in args], cwd=GAME, env=env, check=True)
    run(sys.executable, 'tools/make_prog.py', 'extracted')
    for kind, prefix, mode in [('sub', 'tc2sub', '1'), ('io', 'tc2io', '0')]:
        output = GAME / f'gen/csz_{kind}.c'
        if not output.exists():
            env = os.environ.copy(); env['PYTHONUTF8'] = '1'
            if kind == 'io': env['H8_ROMSIZE'] = '0x4000'
            cov = [str(p.with_suffix('')) for p in sorted((GAME / 'cov').glob('h8' + kind + '_*.pcs'))]
            run(sys.executable, HOST / 'tools/h8/h8_translate.py', kind + '.bin', mode, prefix, output, *cov, env=env)
    assets = ROOT / 'build/assets/roms'; assets.mkdir(parents=True, exist_ok=True)
    for item in manifest: shutil.copy2(extracted / item['file'], assets / item['file'])
    (assets.parent / 'roms.sha256').write_text(''.join(m['sha256'] + '  ' + m['file'] + '\n' for m in manifest), encoding='ascii')
    report = {'upstream': json.loads((ROOT / 'upstream/SOURCE.json').read_text()), 'archives': {a.name: hashlib.sha256(a.read_bytes()).hexdigest() for a in archives}, 'chips': manifest}
    (ROOT / 'build/rom-audit.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'Validated {len(manifest)} Crisis Zone ROM chips; MIPS and H8 sources ready.', flush=True)
    return assets.parent
if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('--device',type=Path);a=p.parse_args();prepare(a.rom,a.device)
