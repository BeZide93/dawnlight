"""Ground the title/save rest pose outside the new central table."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
fixture=r'''
#include "generated/roundtable_layout.hpp"
#include <cassert>
#include <cmath>
#include <limits>
namespace roundtable_layout=dawnlight::roundtable_layout;
struct cXyz{float x,y,z;cXyz(float a=0,float b=0,float c=0):x(a),y(b),z(c){}void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Link{struct{cXyz pos{0,-400,760};}current;}link;
bool available=true;Link* daAlink_getAlinkActorClass(){return available?&link:nullptr;}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(const cXyz* p){pos=*p;}};
float floorHeight=-400;
struct Background{float GroundCross(dBgS_ObjGndChk* g){assert(g->pos.x==0&&g->pos.z==760&&g->pos.y==600);return floorHeight;}}background;
Background& dComIfG_Bgsp(){return background;}
'''
main=r'''
int main(){cXyz anchor;assert(hub_center_ground(anchor)&&anchor.x==0&&anchor.y==-400&&anchor.z==0);
 available=false;assert(!hub_center_ground(anchor));available=true;
 for(float invalid:{-1.e9f,std::numeric_limits<float>::quiet_NaN()}){floorHeight=invalid;assert(!hub_center_ground(anchor));}}
'''
fixture='#include <initializer_list>\n'+fixture
core=(ROOT/'src/survivors.cpp').read_text()
assert 'SurvivorsHubBrazier' not in core and 'update_hub_brazier' not in core
with tempfile.TemporaryDirectory() as temp:
 cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test'
 cpp.write_text(fixture+(ROOT/'src/survivors_hub_layout.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Hub layout: rest-area ground probe, invalid floor and bowl removal passed')
