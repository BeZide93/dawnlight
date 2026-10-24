// Offline static point-light visibility baker. No engine/runtime dependency.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>
struct Vec {float x,y,z; float operator[](int i)const{return i==0?x:i==1?y:z;}};
Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec operator*(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
struct Triangle {Vec a,b,c;};
struct Node {Vec low,high;int first,count,left=-1,right=-1;};
class Scene {
public:
 std::vector<Triangle> faces;std::vector<int> order;std::vector<Node> nodes;
 int build(int first,int count){
  const int id=int(nodes.size());nodes.push_back({{1e20f,1e20f,1e20f},{-1e20f,-1e20f,-1e20f},first,count});
  Vec low=nodes[id].low,high=nodes[id].high;
  for(int i=first;i<first+count;++i)for(auto p:{faces[order[i]].a,faces[order[i]].b,faces[order[i]].c}){
   low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
   high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
  }
  nodes[id].low=low;nodes[id].high=high;
  if(count>8){
   const Vec size=high-low;const int axis=size.x>size.y?(size.x>size.z?0:2):(size.y>size.z?1:2);
   const int middle=first+count/2;
   std::nth_element(order.begin()+first,order.begin()+middle,order.begin()+first+count,[&](int a,int b){
    const auto& x=faces[a];const auto& y=faces[b];return (x.a+x.b+x.c)[axis]<(y.a+y.b+y.c)[axis];});
   const int left=build(first,count/2),right=build(middle,count-count/2);
   nodes[id].left=left;nodes[id].right=right;
  }
  return id;
 }
 void build(){order.resize(faces.size());std::iota(order.begin(),order.end(),0);build(0,int(faces.size()));}
 bool hit(int id,Vec origin,Vec direction,float limit)const{
  const Node& node=nodes[id];float near=0,far=limit;
  for(int axis=0;axis<3;++axis){
   if(std::abs(direction[axis])<1e-8f){if(origin[axis]<node.low[axis]||origin[axis]>node.high[axis])return false;}
   else {float a=(node.low[axis]-origin[axis])/direction[axis],b=(node.high[axis]-origin[axis])/direction[axis];
    if(a>b){std::swap(a,b);}
    near=std::max(near,a);far=std::min(far,b);if(near>far)return false;}
  }
  if(node.left>=0)return hit(node.left,origin,direction,limit)||hit(node.right,origin,direction,limit);
  for(int i=node.first;i<node.first+node.count;++i){
   const auto& t=faces[order[i]];const Vec e1=t.b-t.a,e2=t.c-t.a,p=cross(direction,e2);
   const float det=dot(e1,p);if(std::abs(det)<1e-7f)continue;
   const Vec offset=origin-t.a;const float u=dot(offset,p)/det;if(u<0||u>1)continue;
   const Vec q=cross(offset,e1);const float v=dot(direction,q)/det;if(v<0||u+v>1)continue;
   const float distance=dot(e2,q)/det;if(distance>.01f&&distance<limit)return true;
  }
  return false;
 }
 bool blocked(Vec point,Vec light)const{
  Vec direction=light-point;const float distance=std::sqrt(dot(direction,direction));
  if(distance<2)return false;
  direction=direction*(1/distance);
  return hit(0,point+direction*.8f,direction,distance-1.6f);
 }
};
float illumination(const Scene& scene,Vec point,Vec light,Vec hearth){
 const std::array<Vec,5> offsets{{{0,0,0},{40,20,0},{-40,-20,0},{0,20,40},{0,-20,-40}}};
 int visible=0,hearthVisible=0;
 for(auto offset:offsets){visible+=!scene.blocked(point,light+offset);hearthVisible+=!scene.blocked(point,hearth+offset*.6f);}
 const Vec delta=point-hearth;
 // The fireplace contributes at most 22%, falling off locally. Add the two
 // lights before normalization: one source can fill the other's shadows.
 const float hearthWeight=.22f/(1+dot(delta,delta)/(700*700));
 return (.38f+.62f*visible/offsets.size()+hearthWeight*hearthVisible/offsets.size())/(1+hearthWeight);
}
template<class T>void read(std::ifstream& file,T& value){file.read(reinterpret_cast<char*>(&value),sizeof(value));if(!file)throw std::runtime_error("Truncated bake input");}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--self-test"){
  Scene scene;scene.faces={{{-10,0,-10},{10,0,-10},{10,0,10}},{{-10,0,-10},{10,0,10},{-10,0,10}}};scene.build();
  if(!scene.blocked({0,-10,0},{0,10,0})||scene.blocked({30,-10,0},{30,10,0})||
    scene.blocked({0,0,0},{0,10,0})||!scene.blocked({0,10,0},{0,-10,0}))return 1;
  Scene screen;screen.faces={{{-1000,0,-1000},{1000,0,-1000},{1000,0,1000}},{{-1000,0,-1000},{1000,0,1000},{-1000,0,1000}}};screen.build();
  const Vec receiver{0,-100,0},blocked{0,100,0},clear{0,-300,0};
  const float dark=illumination(screen,receiver,blocked,blocked);
  const float tableBlocked=illumination(screen,receiver,blocked,clear);
  const float hearthBlocked=illumination(screen,receiver,clear,blocked);
  if(!(dark<tableBlocked&&tableBlocked<hearthBlocked&&hearthBlocked<1)||
      std::abs(illumination(screen,receiver,clear,clear)-1)>.00001f)return 1;
  std::cout<<"Shadow BVH: blocker, clear ray, surface bias, double-sided visibility and weaker secondary light passed\n";return 0;
 }
 if(argc!=3)return 2;
 std::ifstream input(argv[1],std::ios::binary);uint32_t size,count,samples;Vec light,hearth;read(input,size);read(input,count);read(input,samples);read(input,light);read(input,hearth);
 if(size!=1024||count>100000||samples>size*size)throw std::runtime_error("Invalid bake dimensions");
 Scene scene;scene.faces.resize(count);for(auto& face:scene.faces)read(input,face);scene.build();
 std::vector<unsigned char> pixels(size*size,255);
 for(uint32_t i=0;i<samples;++i){uint32_t pixel;Vec point;read(input,pixel);read(input,point);if(pixel>=pixels.size())throw std::runtime_error("Invalid pixel index");
  pixels[pixel]=static_cast<unsigned char>(std::lround(255*illumination(scene,point,light,hearth)));
 }
 std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
 if(!output)throw std::runtime_error("Cannot write shadow atlas");
 std::cout<<samples<<" lightmap texels, "<<count<<" occluder triangles\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
