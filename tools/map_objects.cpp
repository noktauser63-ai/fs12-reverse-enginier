// Harita nesnelerini dunya sinir kutusuyla listeler. Derleme: g++ -std=c++17 -Isrc tools/map_objects.cpp src/game/MapData.cpp
// m3d yolu simdilik koda gomulu (/tmp/m/assets/map/map01.m3d); hedef nokta hypotf(cx-40.8,cz-182.4) satirinda.
#include "game/MapData.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <cmath>
#include <algorithm>
using namespace fs12;
int main(int argc,char**argv){
  std::ifstream f("/tmp/m/assets/map/map01.m3d",std::ios::binary); std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)),{});
  MapData m; m.parse(d.data(),d.size());
  struct R{size_t i;float cx,cz,sx,sy,sz,dd;unsigned vc;};
  std::vector<R> v;
  for(size_t i=0;i<m.objects.size();++i){auto&o=m.objects[i]; if(o.mesh>=m.meshes.size())continue;
    auto&me=m.meshes[o.mesh]; float mn[3]={1e9,1e9,1e9},mx[3]={-1e9,-1e9,-1e9};
    for(uint32_t k=0;k<me.vertCount;++k){float p[3];memcpy(p,me.vertices+k*24,12);
      for(int r=0;r<3;++r){float w=o.matrix[r]*p[0]+o.matrix[4+r]*p[1]+o.matrix[8+r]*p[2]+o.matrix[12+r]; mn[r]=std::min(mn[r],w);mx[r]=std::max(mx[r],w);}}
    R r{i,(mn[0]+mx[0])/2,(mn[2]+mx[2])/2,mx[0]-mn[0],mx[1]-mn[1],mx[2]-mn[2],0,me.vertCount};
    r.dd=hypotf(r.cx-40.8f,r.cz-182.4f); v.push_back(r);}
  std::sort(v.begin(),v.end(),[](const R&a,const R&b){return a.dd<b.dd;});
  int n=0; for(auto&r:v){ if(r.sy>3.5f && r.sx>4&&r.sz>4 && r.sx<70&&r.sz<70&&r.vc>40){ printf("d=%.0f #%zu %s mesh=%u v=%u c=(%.1f,%.1f) size=(%.1f x %.1f h=%.1f)\n",r.dd,r.i,m.textures[m.objects[r.i].texture].c_str(),m.objects[r.i].mesh,r.vc,r.cx,r.cz,r.sx,r.sz,r.sy); if(++n>40)break;}}
}
