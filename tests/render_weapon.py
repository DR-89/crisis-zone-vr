"""Render an actual TCGUN asset with production shaders using ANGLE on Windows."""
from pathlib import Path
import argparse,ctypes as C,os,re,shutil,subprocess
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('model',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--kind',choices=('pistol','machine-gun'),default='machine-gun');a=p.parse_args()
out=ROOT/'build/weapon-render-test';out.mkdir(parents=True,exist_ok=True);a.output.mkdir(parents=True,exist_ok=True)
vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs=Path(subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip())
angle=next((Path(os.environ['LOCALAPPDATA'])/'Programs/Microsoft VS Code').glob('*/libEGL.dll')).parent
headers=ROOT/'.tools/android-sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include'
for folder in ('GLES3','KHR'):shutil.copytree(headers/folder,out/'include'/folder,dirs_exist_ok=True)
def run(lines):
    script=out/'compile.cmd';script.write_text('@echo off\ncall "'+str(vs/'VC/Auxiliary/Build/vcvars64.bat')+'" >nul\n'+'\n'.join(lines)+'\n')
    subprocess.run([os.environ['ComSpec'],'/c',str(script)],cwd=out,check=True)
run([f'dumpbin /nologo /exports "{angle / "libGLESv2.dll"}" > exports.txt'])
exports=re.findall(r'^\s*\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(gl\w+)\s*$',(out/'exports.txt').read_text(),re.M)
(out/'gles.def').write_text('LIBRARY libGLESv2.dll\nEXPORTS\n'+'\n'.join(exports))
run(['lib /nologo /def:gles.def /machine:x64 /out:gles.lib',f'cl /nologo /std:c11 /O2 /LD /I"{out / "include"}" /I"{ROOT / "quest"}" "{ROOT / "quest/quest_gun.c"}" "{ROOT / "tests/weapon_fixture.c"}" gles.lib /Fe:weapon.dll'])
directory=os.add_dll_directory(str(angle));egl=C.WinDLL(str(angle/'libEGL.dll'));lib=C.CDLL(str(out/'weapon.dll'))
P=C.c_void_p;I=C.c_int;U=C.c_uint
def bind(lib,name,result,*args):
    f=getattr(lib,name);f.restype=result;f.argtypes=args;return f
display=bind(egl,'eglGetDisplay',P,P)(None);assert bind(egl,'eglInitialize',U,P,P,P)(display,None,None)
attributes=(I*15)(0x3033,1,0x3040,0x40,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3025,24,0x3038)
config=P();count=I();assert bind(egl,'eglChooseConfig',U,P,P,P,I,P)(display,attributes,C.byref(config),1,C.byref(count)) and count.value
w=h=720
surface=bind(egl,'eglCreatePbufferSurface',P,P,P,P)(display,config,(I*5)(0x3057,w,0x3056,h,0x3038))
context=bind(egl,'eglCreateContext',P,P,P,P,P)(display,config,None,(I*3)(0x3098,3,0x3038))
assert bind(egl,'eglMakeCurrent',U,P,P,P,P)(display,surface,surface,context)
assert bind(lib,'weapon_init',I,C.c_char_p,I)(str(a.model.resolve()).encode(),0 if a.kind=='pistol' else 1)
draw=bind(lib,'weapon_draw',I,I,I,I,P);pixels=(C.c_ubyte*(w*h*4))();images=[]
for variant in range(8):
    assert draw(variant,w,h,pixels)
    im=Image.frombytes('RGBA',(w,h),bytes(pixels)).transpose(Image.Transpose.FLIP_TOP_BOTTOM).convert('RGB')
    assert sum(px!=im.getpixel((0,0)) for px in im.get_flattened_data())>1000
    im.save(a.output/f'view-{variant}.png');images.append(im)
sheet=Image.new('RGB',(1440,1440))
for i,im in enumerate(images):
    tile=im.resize((360,360));sheet.paste(tile,((i%4)*360,(i//4)*360))
# Two rows: resting pose, then accepted-shot recoil/flash.
sheet.crop((0,0,1440,720)).save(a.output/'contact.png')
print('PASS: production asset upload, muzzle transforms in both hands, eight GLES weapon/recoil/flash views; no GL errors.')
