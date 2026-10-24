"""Check the shipped shadow bake and exercise its actual offline ray tracer."""
from pathlib import Path
import hashlib,json,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
metadata=json.loads((ROOT/'res/survivors_roundtable/shadows.json').read_text())
for path,digest in metadata['sources'].items():
 assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest, f'Rebake shadows after changing {path}'
texture=(ROOT/'res/survivors_roundtable/shadows.i8').read_bytes()
assert len(texture)==1024*1024 and 75<=min(texture)<=97 and max(texture)==255
assert len(set(texture))>=5, 'Area light samples should produce soft intermediate shadows'
assert metadata['light']==[0,-50,0]
assert metadata['hearth'][0]<-1500 and metadata['hearth'][1:]==[-200,0]
assert metadata['hearth_weight']==.22
with tempfile.TemporaryDirectory() as temp:
 exe=Path(temp)/'baker'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O2','-Wall','-Wextra','-Werror',str(ROOT/'tools/bake_roundtable_shadows.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe),'--self-test'],check=True)
print('Roundtable shadows: source fingerprints, atlas bounds, soft shadow levels and BVH passed')
