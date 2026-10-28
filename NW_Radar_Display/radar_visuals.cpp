#include "radar_visuals.h"
#include "radar_palette.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace radar_visuals {
namespace {
constexpr float radians=.01745329252f;
lv_point_t project(const Geometry &g,float x,float y) {
  const auto p=g.rectangular && g.roomSize.widthMm>0 && g.roomSize.heightMm>0
    ? radar::local_to_room(g.placement,x,y,g.roomBounds,g.roomSize)
    : radar::local_to_map(g.placement,x,y,g.map);
  return {lv_coord_t(std::lround(p.x)),lv_coord_t(std::lround(p.y))};
}
bool outside_room(const Geometry &g,float x,float y) {
  const auto p=radar::local_to_room(g.placement,x,y,g.roomBounds,g.roomSize);
  return p.x<g.roomBounds.x1||p.x>g.roomBounds.x2||p.y<g.roomBounds.y1||p.y>g.roomBounds.y2;
}
lv_area_t distance_area(const Geometry &g,lv_point_t target) {
  const int width=std::min(96,std::max(1,int(g.clip.x2-g.clip.x1)-4));
  const int x=std::max(int(g.clip.x1)+2,std::min(int(target.x)+12,int(g.clip.x2)-width-2));
  const int y=std::max(int(g.clip.y1)+2,std::min(int(target.y)+24>g.clip.y2-20?int(target.y)-40:int(target.y)+24,int(g.clip.y2)-20));
  return {lv_coord_t(x),lv_coord_t(y),lv_coord_t(x+width),lv_coord_t(y+18)};
}
void line(lv_draw_ctx_t *ctx,lv_point_t a,lv_point_t b,uint32_t color,int width,int opacity) {
  lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d); d.color=lv_color_hex(color); d.width=width; d.opa=lv_opa_t(opacity); lv_draw_line(ctx,&d,&a,&b);
}
void circle(lv_draw_ctx_t *ctx,lv_point_t p,int radius,uint32_t color,int opacity,bool fill) {
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.radius=LV_RADIUS_CIRCLE; d.bg_color=lv_color_hex(color); d.bg_opa=fill?lv_opa_t(opacity):LV_OPA_TRANSP;
  d.border_width=fill?0:1; d.border_color=lv_color_hex(color); d.border_opa=lv_opa_t(opacity);
  const lv_area_t a={lv_coord_t(p.x-radius),lv_coord_t(p.y-radius),lv_coord_t(p.x+radius),lv_coord_t(p.y+radius)}; lv_draw_rect(ctx,&d,&a);
}
lv_point_t beam(const Geometry &g,float angle) { return project(g,radar::range_mm*std::sin(angle*radians),radar::range_mm*std::cos(angle*radians)); }
void include(lv_area_t &a,bool &any,lv_point_t p,int pad) {
  lv_area_t b={lv_coord_t(p.x-pad),lv_coord_t(p.y-pad),lv_coord_t(p.x+pad),lv_coord_t(p.y+pad)};
  if(!any) { a=b; any=true; } else { a.x1=std::min(a.x1,b.x1); a.y1=std::min(a.y1,b.y1); a.x2=std::max(a.x2,b.x2); a.y2=std::max(a.y2,b.y2); }
}
}
Geometry build(const radar::RadarPlacement &p,const radar::MapScale &map) {
  Geometry g; g.placement=p; g.map=map; g.origin=project(g,0,0);
  g.clip={lv_coord_t(map.x),lv_coord_t(map.y),lv_coord_t(map.x+map.width),lv_coord_t(map.y+map.height)};
  g.radius=int(std::lround(radar::range_mm*map.width/map.widthMm*map.visualScale));
  for(unsigned j=0;j<=24;++j) g.arc[j]=beam(g,-60+5.0f*j);
  return g;
}
lv_point_t target_point(const Geometry &g,float x,float y) {
  auto p=project(g,x,y);
  if(g.rectangular) {
    // In-room measurements keep their exact metric position, even near walls.
    // Only out-of-room measurements use the earlier show-mode edge marker.
    if(g.roomSize.widthMm>0 && g.roomSize.heightMm>0 &&
       p.x>=std::floor(g.roomBounds.x1) && p.x<=std::ceil(g.roomBounds.x2) && p.y>=std::floor(g.roomBounds.y1) && p.y<=std::ceil(g.roomBounds.y2)) {
      p.x=std::max(g.clip.x1,std::min(p.x,g.clip.x2));p.y=std::max(g.clip.y1,std::min(p.y,g.clip.y2));return p;
    }
    // Show mode: keep the complete bright core/halo inside the assigned room.
    const int mx=std::min(25,std::max(0,int(g.clip.x2-g.clip.x1)/2));
    const int my=std::min(25,std::max(0,int(g.clip.y2-g.clip.y1)/2));
    p.x=lv_coord_t(std::max(int(g.clip.x1)+mx,std::min(int(p.x),int(g.clip.x2)-mx)));
    p.y=lv_coord_t(std::max(int(g.clip.y1)+my,std::min(int(p.y),int(g.clip.y2)-my)));
    // Reserve the warning icon's upper-right area even in its dark phase,
    // so a corner target stays visible and never jumps with the blink.
    if(p.x>g.clip.x2-62 && p.y<g.clip.y1+58 && g.clip.x2-62>=g.clip.x1+mx)
      p.x=lv_coord_t(g.clip.x2-62);
  }
  return p;
}
void sector(lv_draw_ctx_t *ctx,const Geometry &g,unsigned node,bool online) {
  const auto color=radar_palette::node[node];
  // The floorplan walls alone define the room radar area.
  if(g.rectangular) return;
  lv_area_t bounds={lv_coord_t(g.origin.x-g.radius),lv_coord_t(g.origin.y-g.radius),lv_coord_t(g.origin.x+g.radius),lv_coord_t(g.origin.y+g.radius)};
  lv_draw_mask_radius_param_t radius; lv_draw_mask_angle_param_t angle;
  lv_draw_mask_radius_init(&radius,&bounds,LV_RADIUS_CIRCLE,false);
  const int heading=int(g.placement.angleDeg);
  lv_draw_mask_angle_init(&angle,g.origin.x,g.origin.y,(heading+210)%360,(heading+330)%360);
  const auto r=lv_draw_mask_add(&radius,nullptr),a=lv_draw_mask_add(&angle,nullptr);
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.bg_color=lv_color_hex(color); d.bg_opa=online?60:45;
  if(r>=0 && a>=0) lv_draw_rect(ctx,&d,&bounds);
  if(r>=0) lv_draw_mask_remove_id(r); if(a>=0) lv_draw_mask_remove_id(a);
  lv_draw_mask_free_param(&radius); lv_draw_mask_free_param(&angle);
  line(ctx,g.origin,g.arc[0],color,1,online?225:180); line(ctx,g.origin,g.arc[24],color,1,online?225:180);
  for(int ring=1;ring<=3;++ring) for(unsigned j=0;j<24;++j) {
    const float k=ring/3.0f;
    lv_point_t p={lv_coord_t(g.origin.x+(g.arc[j].x-g.origin.x)*k),lv_coord_t(g.origin.y+(g.arc[j].y-g.origin.y)*k)};
    lv_point_t q={lv_coord_t(g.origin.x+(g.arc[j+1].x-g.origin.x)*k),lv_coord_t(g.origin.y+(g.arc[j+1].y-g.origin.y)*k)};
    line(ctx,p,q,color,1,online?165:120);
  }
}
void dynamic(lv_draw_ctx_t *ctx,const Geometry &g,unsigned node,bool selected,bool online,bool /*animate*/,uint32_t now) {
  const auto color=radar_palette::node[node]; const auto &fx=radar_feedback::shared().node(node);
  for(unsigned j=0;j<fx.count;++j) {
    const auto &s=fx.sample(j); const uint32_t age=now-s.at; if(age>=1000) continue;
    const auto p=target_point(g,s.target.xMm,s.target.yMm); const int opacity=int(155*(1000-age)/1000);
    if(j) { const auto &before=fx.sample(j-1); line(ctx,target_point(g,before.target.xMm,before.target.yMm),p,color,1,opacity); }
    circle(ctx,p,2,color,opacity,true);
  }
  const bool locked=fx.lock && uint32_t(now-fx.acquiredAt)<500 && radar::in_sector(fx.target);
  circle(ctx,g.origin,selected?17:11,color,selected?255:online?190:110,false);
  circle(ctx,g.origin,5,online?color:radar_palette::support[node],255,true);
  if(locked) circle(ctx,g.origin,15,color,80,false);
  const auto forward=project(g,0,1000);
  const float dx=forward.x-g.origin.x,dy=forward.y-g.origin.y;
  const float length=std::max(1.0f,std::hypot(dx,dy));
  line(ctx,g.origin,{lv_coord_t(g.origin.x+20*dx/length),lv_coord_t(g.origin.y+20*dy/length)},color,2,online?255:130);
  char name[4]; std::snprintf(name,sizeof(name),"N%u",node+1);
  const int x=std::max(int(g.clip.x1)+2,std::min(int(g.origin.x)+16,int(g.clip.x2)-28));
  const int y=std::max(int(g.clip.y1)+2,std::min(int(g.origin.y)-22,int(g.clip.y2)-18));
  lv_area_t area={lv_coord_t(x),lv_coord_t(y),lv_coord_t(x+27),lv_coord_t(y+18)};
  lv_draw_label_dsc_t label; lv_draw_label_dsc_init(&label); label.font=&lv_font_montserrat_14; label.color=lv_color_hex(color); lv_draw_label(ctx,&label,&area,name,nullptr);
  if(!radar::in_sector(fx.target)) return;
  const auto target=target_point(g,fx.target.xMm,fx.target.yMm);
  // Bright red targets remain distinct from the colored sensor origins.
  constexpr auto red=radar_palette::alarm;
  circle(ctx,target,22,red,16,true); circle(ctx,target,17,red,38,true);
  circle(ctx,target,12,red,70,true); circle(ctx,target,10,red,235,false);
  circle(ctx,target,7,red,255,true); circle(ctx,target,3,radar_palette::target_core,255,true);
  if(locked) {
    const float t=float(uint32_t(now-fx.acquiredAt))/500;
    circle(ctx,target,int(8+27*t),red,int(245*(1-t)),false);
    circle(ctx,target,int(6+19*t),red,int(80*(1-t)),false);
  }
  if(g.rectangular) {
    const auto area=distance_area(g,target);char distance[24];
    const bool outside=outside_room(g,fx.target.xMm,fx.target.yMm);
    std::snprintf(distance,sizeof(distance),outside?"OUT %.1fm":"%.2f m",std::hypot(fx.target.xMm,fx.target.yMm)/1000);
    lv_draw_rect_dsc_t box;lv_draw_rect_dsc_init(&box);box.bg_color=lv_color_hex(radar_palette::background);box.bg_opa=230;lv_draw_rect(ctx,&box,&area);
    lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.font=&lv_font_montserrat_14;label.color=lv_color_hex(outside?radar_palette::alarm:radar_palette::target_core);lv_draw_label(ctx,&label,&area,distance,nullptr);
  }
}
bool room_alarm_active(const Geometry &g,unsigned node,bool online) {
  if(!online || !system_state::enabled(node)) return false;
  const auto &target=radar_feedback::shared().node(node).target;
  if(!radar::in_sector(target)) return false;
  const auto p=target_point(g,target.xMm,target.yMm);
  return p.x>=g.clip.x1 && p.x<=g.clip.x2 && p.y>=g.clip.y1 && p.y<=g.clip.y2;
}
bool room_alarm_lit(const Geometry &g,unsigned node,bool online,bool animate,uint32_t now) {
  return animate && room_alarm_active(g,node,online) && (now/400u)%2u==0;
}
void room_alarm(lv_draw_ctx_t *ctx,const Geometry &g,unsigned node,bool online,bool animate,uint32_t now) {
  if(!room_alarm_lit(g,node,online,animate,now)) return;
  const auto red=radar_palette::alarm;
  // Draw inside the room layer; neither walls nor neighboring rooms are painted.
  for(unsigned pass=0;pass<3;++pass) {
    lv_area_t area=g.clip;
    const int inset=pass==0?1:pass==1?2:3;
    area.x1+=inset; area.y1+=inset; area.x2-=inset; area.y2-=inset;
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
    d.bg_opa=LV_OPA_TRANSP; d.border_color=lv_color_hex(red);
    d.border_width=pass==0?7:pass==1?4:2; d.border_opa=pass==0?35:pass==1?90:255;
    lv_draw_rect(ctx,&d,&area);
  }
  const int x=g.clip.x2-22,y=g.clip.y1+12;
  const lv_point_t triangle[]={{lv_coord_t(x),lv_coord_t(y)},
    {lv_coord_t(x-12),lv_coord_t(y+23)},{lv_coord_t(x+12),lv_coord_t(y+23)}};
  lv_draw_rect_dsc_t fill; lv_draw_rect_dsc_init(&fill);
  fill.bg_color=lv_color_hex(radar_palette::panel); fill.bg_opa=230;
  lv_draw_polygon(ctx,&fill,triangle,3);
  for(unsigned j=0;j<3;++j) line(ctx,triangle[j],triangle[(j+1)%3],red,2,255);
  line(ctx,{lv_coord_t(x),lv_coord_t(y+8)},{lv_coord_t(x),lv_coord_t(y+15)},radar_palette::target_core,2,255);
  circle(ctx,{lv_coord_t(x),lv_coord_t(y+19)},1,radar_palette::target_core,255,true);
}
void invalidate_room_alarm(lv_obj_t *obj,const Geometry &g) {
  const auto &c=g.clip;
  // Four narrow edge strips and one icon area, instead of a whole-room repaint.
  const lv_area_t areas[]={{c.x1,c.y1,c.x2,lv_coord_t(c.y1+8)},
    {c.x1,lv_coord_t(c.y2-8),c.x2,c.y2},
    {c.x1,c.y1,lv_coord_t(c.x1+8),c.y2},
    {lv_coord_t(c.x2-8),c.y1,c.x2,c.y2},
    {lv_coord_t(c.x2-37),lv_coord_t(c.y1+8),lv_coord_t(c.x2-6),lv_coord_t(c.y1+39)}};
  for(const auto &area:areas) lv_obj_invalidate_area(obj,&area);
}
Regions regions(const Geometry &g,unsigned node,bool online,bool animate,uint32_t now) {
  Regions result; const auto &fx=radar_feedback::shared().node(node);
  if(radar::in_sector(fx.target)) {
    const auto point=target_point(g,fx.target.xMm,fx.target.yMm);
    include(result.target,result.hasTarget,point,38);
    if(g.rectangular) { const auto area=distance_area(g,point);include(result.target,result.hasTarget,{area.x1,area.y1},2);include(result.target,result.hasTarget,{area.x2,area.y2},2); }
  }
  for(unsigned j=0;j<fx.count;++j) include(result.target,result.hasTarget,target_point(g,fx.sample(j).target.xMm,fx.sample(j).target.yMm),4);
  if(online && animate) {
    // Only the sensor/lock marker animates; there is no sweeping beam.
    result.hasSensor=true;
    result.sensor={lv_coord_t(g.origin.x-28),lv_coord_t(g.origin.y-28),lv_coord_t(g.origin.x+45),lv_coord_t(g.origin.y+22)};
  }
  return result;
}
void invalidate(lv_obj_t *obj,const Regions &r) {
  if(r.hasTarget) lv_obj_invalidate_area(obj,&r.target);
  if(r.hasSensor) lv_obj_invalidate_area(obj,&r.sensor);
}
}
