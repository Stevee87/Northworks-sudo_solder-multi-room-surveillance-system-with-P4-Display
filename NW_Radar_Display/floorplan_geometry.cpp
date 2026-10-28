#include "floorplan_geometry.h"
#include <algorithm>
#include <cmath>
namespace floorplan {
bool valid_dimensions(const FloorplanConfig &c) {
  for(unsigned i=0;i<3;++i) {
    const auto &r=c.rooms[i]; const auto &s=c.sensors[i];
    if(!std::isfinite(r.xMm)||!std::isfinite(r.yMm)||!std::isfinite(r.widthMm)||!std::isfinite(r.heightMm)
      ||r.xMm<min_position_mm||r.yMm<min_position_mm||r.widthMm<min_room_mm||r.heightMm<min_room_mm
      ||r.widthMm>max_room_mm||r.heightMm>max_room_mm
      ||r.xMm+r.widthMm>max_extent_mm||r.yMm+r.heightMm>max_extent_mm
      ||!std::isfinite(s.xMm)||!std::isfinite(s.yMm)||!std::isfinite(s.angleDeg)
      ||s.xMm<0||s.xMm>r.widthMm||s.yMm<0||s.yMm>r.heightMm||s.angleDeg<0||s.angleDeg>=360) return false;
  }
  return true;
}
bool overlaps(const FloorplanConfig &c) {
  for(unsigned i=0;i<3;++i) for(unsigned j=i+1;j<3;++j) {
    const auto &a=c.rooms[i],&b=c.rooms[j];
    if(std::min(a.xMm+a.widthMm,b.xMm+b.widthMm)-std::max(a.xMm,b.xMm)>.5f &&
       std::min(a.yMm+a.heightMm,b.yMm+b.heightMm)-std::max(a.yMm,b.yMm)>.5f) return true;
  }
  return false;
}
bool valid_config(const FloorplanConfig &c) { return valid_dimensions(c)&&!overlaps(c); }
PlanScale fit(const FloorplanConfig &c) {
  // Fit the real room bounds, including rooms left/above the world origin.
  float left=c.rooms[0].xMm,top=c.rooms[0].yMm;
  float right=left+c.rooms[0].widthMm,bottom=top+c.rooms[0].heightMm;
  for(const auto &r:c.rooms) {
    left=std::min(left,r.xMm);top=std::min(top,r.yMm);
    right=std::max(right,r.xMm+r.widthMm);bottom=std::max(bottom,r.yMm+r.heightMm);
  }
  const float width=right-left,height=bottom-top;
  const float k=std::min((drawing_area.x2-drawing_area.x1)/width,(drawing_area.y2-drawing_area.y1)/height);
  return {k,drawing_area.x1+((drawing_area.x2-drawing_area.x1)-width*k)/2-left*k,
            drawing_area.y1+((drawing_area.y2-drawing_area.y1)-height*k)/2-top*k};
}
radar::Point to_plan(PlanScale s,float x,float y) { return {s.x+x*s.pixelsPerMm,s.y+y*s.pixelsPerMm}; }
radar::Point from_plan(PlanScale s,float x,float y) { return {(x-s.x)/s.pixelsPerMm,(y-s.y)/s.pixelsPerMm}; }
Geometry calculate_geometry(const FloorplanConfig &config,PlanScale scale) {
  const auto &c=valid_dimensions(config)?config:defaults;
  Geometry g{};g.scale=scale;
  const char *names[]={"ROOM 01","ROOM 02","ROOM 03"};
  for(unsigned i=0;i<3;++i) {
    const auto &r=c.rooms[i]; const auto a=to_plan(scale,r.xMm,r.yMm),b=to_plan(scale,r.xMm+r.widthMm,r.yMm+r.heightMm);
    g.rooms[i]={int(i+1),names[i],{int(std::lround(a.x)),int(std::lround(a.y)),int(std::lround(b.x)),int(std::lround(b.y))}};
  }
  return g;
}
Geometry calculate_geometry(const FloorplanConfig &c) { const auto &valid=valid_dimensions(c)?c:defaults;return calculate_geometry(valid,fit(valid)); }
radar::Point target_world(const FloorplanConfig &c,unsigned n,float x,float y) {
  if(n>=3) return {};
  const auto &r=c.rooms[n]; const auto &s=c.sensors[n]; const float a=s.angleDeg*.0174532925199433f;
  return {r.xMm+s.xMm+x*std::cos(a)+y*std::sin(a),r.yMm+s.yMm+x*std::sin(a)-y*std::cos(a)};
}
}
