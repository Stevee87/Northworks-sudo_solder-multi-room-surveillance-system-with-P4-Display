#include "floorplan.h"
#include "radar_palette.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
namespace floorplan {
namespace {
Geometry current;
FloorplanConfig saved,draft,dragStart;
SaveCallback persist;
bool edit_mode=false,save_failed=false;
lv_obj_t *root,*buttons[4],*hitbox;
enum Command { Edit,Save,Cancel,Reset };
enum Drag { None,Move,Width,Height,Both };
Drag active=None;
unsigned selected=0;
float startX=0,startY=0;
PlanScale dragScale;
uint32_t layoutRevision=0;
Viewport view;
int scaled(int n) { return std::max(1,(n*view.width+640)/1280); }
void rect(lv_draw_ctx_t *ctx,Rect r,uint32_t color,int opacity=255,int border=0) {
  const auto a=view.point(r.x1,r.y1),b=view.point(r.x2,r.y2);
  lv_area_t area={a.x,a.y,lv_coord_t(b.x-1),lv_coord_t(b.y-1)};
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
  d.bg_color=lv_color_hex(color);d.bg_opa=border?LV_OPA_TRANSP:lv_opa_t(opacity);
  d.border_color=lv_color_hex(color);d.border_width=scaled(border);d.border_opa=lv_opa_t(opacity);
  lv_draw_rect(ctx,&d,&area);
}
void line(lv_draw_ctx_t *ctx,int x1,int y1,int x2,int y2,uint32_t color,int width=1) {
  const auto a=view.point(x1,y1),b=view.point(x2,y2);
  lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(color);d.width=scaled(width);lv_draw_line(ctx,&d,&a,&b);
}
void label(lv_draw_ctx_t *ctx,Rect r,const char *s,uint32_t color,bool center=false,const lv_font_t *font=&lv_font_montserrat_14) {
  const auto a=view.point(r.x1,r.y1),b=view.point(r.x2,r.y2);
  lv_area_t area={a.x,a.y,b.x,b.y};lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);
  d.color=lv_color_hex(color);d.font=font;d.align=center?LV_TEXT_ALIGN_CENTER:LV_TEXT_ALIGN_LEFT;
  lv_draw_label(ctx,&d,&area,s,nullptr);
}
void invalidate() {
  const auto a=view.point(20,82),b=view.point(1260,719);lv_area_t area={a.x,a.y,b.x,b.y};lv_obj_invalidate_area(root,&area);
}
void update(bool autoFit=true) {
  current=autoFit?calculate_geometry(draft):calculate_geometry(draft,dragScale);
  ++layoutRevision;invalidate();
}
void handle(lv_draw_ctx_t *ctx,int x,int y,Drag type) {
  rect(ctx,{x-15,y-15,x+16,y+16},radar_palette::panel);
  rect(ctx,{x-15,y-15,x+16,y+16},radar_palette::text,255,1);
  if(type==Width||type==Both) line(ctx,x-8,y,x+8,y,radar_palette::text,2);
  if(type==Height||type==Both) line(ctx,x,y-8,x,y+8,radar_palette::text,2);
}
void draw(lv_event_t *event) {
  auto *ctx=lv_event_get_draw_ctx(event);
  rect(ctx,building,radar_palette::background);
  const auto scale=current.scale;
  // One real metre in both axes, fixed to the world origin.
  const auto top=from_plan(scale,drawing_area.x1,drawing_area.y1),bottom=from_plan(scale,drawing_area.x2,drawing_area.y2);
  for(int x=int(std::ceil(top.x/1000));x*1000<=bottom.x;++x) {
    const auto p=to_plan(scale,x*1000,0);line(ctx,int(p.x),drawing_area.y1,int(p.x),drawing_area.y2,radar_palette::grid);
  }
  for(int y=int(std::ceil(top.y/1000));y*1000<=bottom.y;++y) {
    const auto p=to_plan(scale,0,y*1000);line(ctx,drawing_area.x1,int(p.y),drawing_area.x2,int(p.y),radar_palette::grid);
  }
  const bool overlap=overlaps(draft);
  for(unsigned i=0;i<3;++i) {
    const auto r=current.rooms[i].interior;
    rect(ctx,r,radar_palette::map,220);
    rect(ctx,r,edit_mode&&i==selected?(overlap?radar_palette::alarm:radar_palette::text):radar_palette::wall,255,2);
    const int cy=(r.y1+r.y2)/2;
    char x[32],y[32];std::snprintf(x,sizeof(x),"X %.2f m",draft.rooms[i].widthMm/1000);
    std::snprintf(y,sizeof(y),"Y %.2f m",draft.rooms[i].heightMm/1000);
    const auto pa=view.point(r.x1+2,r.y1+2),pb=view.point(r.x2-2,r.y2-2);
    const lv_area_t bounds={pa.x,pa.y,pb.x,pb.y};lv_area_t clipped;
    const auto *previous=ctx->clip_area;
    if(_lv_area_intersect(&clipped,previous,&bounds)) {
      ctx->clip_area=&clipped;
      label(ctx,{r.x1+3,cy-40,r.x2-3,cy-16},current.rooms[i].name,radar_palette::muted,true);
      label(ctx,{r.x1+3,cy-10,r.x2-3,cy+14},x,radar_palette::text,true);
      label(ctx,{r.x1+3,cy+20,r.x2-3,cy+44},y,radar_palette::text,true);
      ctx->clip_area=previous;
    }
  }
  if(edit_mode) {
    const auto r=current.rooms[selected].interior;
    handle(ctx,r.x2,(r.y1+r.y2)/2,Width);handle(ctx,(r.x1+r.x2)/2,r.y2,Height);handle(ctx,r.x2,r.y2,Both);
  }
  label(ctx,{24,14,610,44},"TACTICAL SECURITY SYSTEM",radar_palette::text,false,view.width<1000?&lv_font_montserrat_18:&lv_font_montserrat_24);
  label(ctx,{24,46,610,68},"MULTI-ZONE RADAR MONITORING",radar_palette::muted);
  line(ctx,24,80,1256,80,radar_palette::grid);
  char info[180];
  if(edit_mode) {
    const auto &r=draft.rooms[selected];
    std::snprintf(info,sizeof(info),"ROOM %02u  X %.2f  Y %.2f m   WIDTH %.2f  HEIGHT %.2f m",selected+1,r.xMm/1000,r.yMm/1000,r.widthMm/1000,r.heightMm/1000);
  } else std::snprintf(info,sizeof(info),"METRIC PLAN  /  GRID 1 m  /  X RIGHT, Y DOWN");
  label(ctx,{30,94,1230,122},info,radar_palette::text);
  const int bar=int(std::lround(1000*scale.pixelsPerMm));
  line(ctx,56,642,56+bar,642,radar_palette::text,2);line(ctx,56,637,56,647,radar_palette::text);line(ctx,56+bar,637,56+bar,647,radar_palette::text);
  label(ctx,{64+bar,630,250+bar,656},"1 m",radar_palette::muted);
  line(ctx,24,668,1256,668,radar_palette::grid);
  if(edit_mode) label(ctx,{24,680,1256,718},save_failed?"SAVE FAILED - TRY AGAIN":overlap?"OVERLAP - MOVE ROOMS APART BEFORE SAVE":"DRAG ROOM TO MOVE / RIGHT = X / BOTTOM = Y / CORNER = BOTH / 5 cm STEPS",save_failed||overlap?radar_palette::alarm:radar_palette::text);
}
void position(lv_obj_t *obj,Rect r) {
  const auto a=view.point(r.x1,r.y1),b=view.point(r.x2,r.y2);lv_obj_set_pos(obj,a.x,a.y);lv_obj_set_size(obj,b.x-a.x,b.y-a.y);
}
void set_mode(bool on) {
  edit_mode=on;active=None;save_failed=false;
  for(unsigned i=0;i<4;++i) {
    if((i==Edit)?!on:on) lv_obj_clear_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);
  }
  if(on) lv_obj_clear_flag(hitbox,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(hitbox,LV_OBJ_FLAG_HIDDEN);
  invalidate();
}
void command(lv_event_t *event) {
  const auto c=Command(uintptr_t(lv_event_get_user_data(event)));
  if(c==Edit&&!edit_mode) { draft=saved;selected=0;set_mode(true); }
  else if(edit_mode&&c==Cancel) { draft=saved;update();set_mode(false); }
  else if(edit_mode&&c==Reset) { draft=defaults;save_failed=false;update(); }
  else if(edit_mode&&c==Save) {
    if(!valid_config(draft)) { invalidate();return; }
    if(persist&&persist(draft)) { saved=draft;update();set_mode(false); }
    else { save_failed=true;invalidate(); }
  }
}
float snap(float mm) { return std::round(mm/50)*50; }
void drag(lv_event_t *event) {
  if(!edit_mode) return;
  const auto code=lv_event_get_code(event);
  if(code==LV_EVENT_RELEASED||code==LV_EVENT_PRESS_LOST) { if(active!=None) { active=None;update(); }return; }
  if(code!=LV_EVENT_PRESSED&&code!=LV_EVENT_PRESSING) return;
  auto *input=lv_indev_get_act();if(!input)return;lv_point_t p;lv_indev_get_point(input,&p);
  const float x=(p.x-view.x)*1280.0f/view.width,y=(p.y-view.y)*720.0f/view.height;
  if(code==LV_EVENT_PRESSED) {
    const auto r=current.rooms[selected].interior;
    const float hit=22.0f*1280/view.width;
    auto near=[&](float hx,float hy) { return std::hypot(x-hx,y-hy)<=hit; };
    if(near(r.x2,r.y2))active=Both;
    else if(near(r.x2,(r.y1+r.y2)/2))active=Width;
    else if(near((r.x1+r.x2)/2,r.y2))active=Height;
    else {
      active=None;
      for(unsigned i=0;i<3;++i) { const auto q=current.rooms[i].interior;if(x>=q.x1&&x<q.x2&&y>=q.y1&&y<q.y2){selected=i;active=Move;break;} }
    }
    if(active==None)return;
    dragStart=draft;dragScale=current.scale;startX=x;startY=y;save_failed=false;invalidate();
  }
  if(active==None)return;
  auto c=dragStart;auto &r=c.rooms[selected];
  const float dx=(x-startX)/dragScale.pixelsPerMm,dy=(y-startY)/dragScale.pixelsPerMm;
  if(active==Move) { r.xMm=std::max(min_position_mm,std::min(max_extent_mm-r.widthMm,snap(r.xMm+dx)));r.yMm=std::max(min_position_mm,std::min(max_extent_mm-r.heightMm,snap(r.yMm+dy))); }
  else {
    if(active==Width||active==Both)r.widthMm=std::max(min_room_mm,std::min(std::min(max_room_mm,max_extent_mm-r.xMm),snap(r.widthMm+dx)));
    if(active==Height||active==Both)r.heightMm=std::max(min_room_mm,std::min(std::min(max_room_mm,max_extent_mm-r.yMm),snap(r.heightMm+dy)));
    auto &s=c.sensors[selected];s.xMm=std::min(s.xMm,r.widthMm);s.yMm=std::min(s.yMm,r.heightMm);
  }
  draft=c;update(false);
}
lv_obj_t *button(const char *caption,Command c,Rect bounds) {
  auto *obj=lv_btn_create(root);lv_obj_remove_style_all(obj);position(obj,bounds);
  lv_obj_set_height(obj,std::max(44,int(lv_obj_get_height(obj))));lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::panel),0);lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
  lv_obj_set_style_border_color(obj,lv_color_hex(radar_palette::wall),0);lv_obj_set_style_border_width(obj,1,0);lv_obj_set_style_radius(obj,4,0);
  lv_obj_set_style_text_color(obj,lv_color_hex(radar_palette::text),0);lv_obj_set_style_text_font(obj,&lv_font_montserrat_14,0);lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
  auto *l=lv_label_create(obj);lv_label_set_text(l,caption);lv_obj_center(l);lv_obj_add_event_cb(obj,command,LV_EVENT_CLICKED,reinterpret_cast<void*>(uintptr_t(c)));return obj;
}
}
const Geometry &geometry(){return current;}
FloorplanConfig config(){return draft;}
uint32_t revision(){return layoutRevision;}
bool is_editing(){return edit_mode;}
void set_sensor_editor_active(bool on){if(on)lv_obj_add_flag(buttons[Edit],LV_OBJ_FLAG_HIDDEN);else if(!edit_mode)lv_obj_clear_flag(buttons[Edit],LV_OBJ_FLAG_HIDDEN);}
bool save_sensors(const Sensor (&sensors)[3]) {
  if(edit_mode)return false;
  auto c=saved;for(unsigned i=0;i<3;++i)c.sensors[i]=sensors[i];
  if(!valid_config(c)||!persist||!persist(c))return false;
  saved=draft=c;update();return true;
}
Viewport viewport(int w,int h){int vw=w,vh=w*720/1280;if(vh>h){vh=h;vw=h*1280/720;}return {(w-vw)/2,(h-vh)/2,vw,vh};}
lv_point_t Viewport::point(int x1,int y1)const{return {lv_coord_t(x+(x1*width+640)/1280),lv_coord_t(y+(y1*height+360)/720)};}
lv_point_t Viewport::room_point(unsigned i,float u,float v)const{if(i>=3)return point(0,0);const auto r=current.rooms[i].interior;return point(r.x1+int(std::max(0.0f,std::min(1.0f,u))*(r.x2-r.x1-1)),r.y1+int(std::max(0.0f,std::min(1.0f,v))*(r.y2-r.y1-1)));}
lv_obj_t *create(lv_obj_t *parent,int w,int h,FloorplanConfig initial,SaveCallback save) {
  view=viewport(w,h);saved=draft=valid_config(initial)?initial:defaults;current=calculate_geometry(draft);persist=save;
  root=lv_obj_create(parent);lv_obj_remove_style_all(root);lv_obj_set_size(root,w,h);lv_obj_set_pos(root,0,0);lv_obj_clear_flag(root,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_bg_color(root,lv_color_hex(radar_palette::background),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);lv_obj_add_event_cb(root,draw,LV_EVENT_DRAW_MAIN,nullptr);
  hitbox=lv_obj_create(root);lv_obj_remove_style_all(hitbox);position(hitbox,{24,126,1256,662});lv_obj_clear_flag(hitbox,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(hitbox,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_PRESS_LOCK);lv_obj_add_event_cb(hitbox,drag,LV_EVENT_ALL,nullptr);
  buttons[Edit]=button("EDIT",Edit,{1076,8,1200,52});buttons[Save]=button("SAVE",Save,{824,8,936,52});buttons[Cancel]=button("CANCEL",Cancel,{948,8,1072,52});buttons[Reset]=button("RESET",Reset,{1084,8,1200,52});set_mode(false);return root;
}
}
