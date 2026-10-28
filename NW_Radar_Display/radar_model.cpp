#include "radar_model.h"
#include <cmath>
#include <algorithm>

namespace radar {
constexpr float radians=0.0174532925199433f;
Config defaults() { return {{{1,0.17f,0.20f,115},{2,0.83f,0.20f,245},{3,0.5f,0.84f,0}},18000}; }
bool valid_config(const Config &c) {
  if (!std::isfinite(c.mapWidthMm) || c.mapWidthMm<6000 || c.mapWidthMm>60000) return false;
  for (unsigned i=0;i<node_count;++i) {
    const auto &p=c.nodes[i];
    if (p.nodeId!=i+1 || !std::isfinite(p.normalizedX) || !std::isfinite(p.normalizedY)
        || !std::isfinite(p.angleDeg) || p.normalizedX<0 || p.normalizedX>1
        || p.normalizedY<0 || p.normalizedY>1 || p.angleDeg<0 || p.angleDeg>=360) return false;
  }
  return true;
}
Point local_to_map(const RadarPlacement &p,float x,float y,const MapScale &m) {
  const float a=p.angleDeg*radians, s=std::sin(a), c=std::cos(a), k=m.width/m.widthMm*m.visualScale;
  return {m.x+p.normalizedX*m.width+(x*c+y*s)*k,
          m.y+p.normalizedY*m.height+(x*s-y*c)*k};
}
Point local_to_room(const RadarPlacement &p,float x,float y,const Bounds &room,const RoomSize &size) {
  const float a=p.angleDeg*radians,s=std::sin(a),c=std::cos(a);
  const float dx=x*c+y*s,dy=x*s-y*c;
  return {room.x1+(p.normalizedX+dx/size.widthMm)*(room.x2-room.x1),
          room.y1+(p.normalizedY+dy/size.heightMm)*(room.y2-room.y1)};
}
MapScale fit_room(const RadarPlacement &p,const MapScale &base,const Bounds &room) {
  MapScale result=base;
  if(!(base.width>0 && base.widthMm>0 && room.x2>room.x1 && room.y2>room.y1)) return result;
  const auto origin=local_to_map(p,0,0,base);
  const float dx=std::sin(p.angleDeg*radians),dy=-std::cos(p.angleDeg*radians);
  float depth=std::hypot(room.x2-room.x1,room.y2-room.y1);
  if(dx>0.000001f) depth=std::min(depth,(room.x2-origin.x)/dx);
  if(dx<-.000001f) depth=std::min(depth,(room.x1-origin.x)/dx);
  if(dy>0.000001f) depth=std::min(depth,(room.y2-origin.y)/dy);
  if(dy<-.000001f) depth=std::min(depth,(room.y1-origin.y)/dy);
  // Keep a finite scale even when a sensor is dragged against a wall.
  depth=std::max(1.0f,depth);
  result.visualScale=depth*base.widthMm/(range_mm*base.width);
  return result;
}
bool in_sector(const Target &t) {
  return t.valid && std::isfinite(t.xMm) && std::isfinite(t.yMm) && t.yMm>=0
      && t.xMm*t.xMm+t.yMm*t.yMm<=range_mm*range_mm+1
      && std::abs(std::atan2(t.xMm,t.yMm))<=60*radians+0.00001f;
}
TargetFrame demo_frame(uint32_t ms) {
  TargetFrame result{};
  const float t=(ms%120000)*0.00005235987756f;
  for (unsigned i=0;i<node_count;++i) {
    const float phase=i*2.0943951f;
    const float bearing=38*radians*std::sin(11*t+phase);
    const float distance=3000+1000*std::sin(7*t+phase);
    result.nodes[i].targets[0]={true,distance*std::sin(bearing),distance*std::cos(bearing)};
  }
  return result;
}
}
