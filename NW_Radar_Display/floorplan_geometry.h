#pragma once
#include "radar_model.h"
#include <cstdint>
namespace floorplan {
struct Rect { int x1,y1,x2,y2; }; // half-open logical display bounds
struct PhysicalRoom { float xMm,yMm,widthMm,heightMm; };
struct Sensor { float xMm,yMm,angleDeg; }; // relative to room top-left, clockwise heading
struct FloorplanConfig { PhysicalRoom rooms[3]; Sensor sensors[3]; };
constexpr FloorplanConfig defaults={
  {{0,0,5000,3000},{0,3000,5000,3000},{5000,0,1800,6000}},
  {{200,180,120.96376f},{200,180,120.96376f},{72,360,163.30076f}}
};
constexpr float min_room_mm=500,max_room_mm=20000,max_extent_mm=30000;
constexpr float min_position_mm=-max_extent_mm;
constexpr Rect building={24,84,1256,656},monitoring_interior={40,92,1240,648};
constexpr Rect drawing_area={56,140,1224,620};
struct Room { int id; const char *name; Rect interior; };
struct PlanScale { float pixelsPerMm,x,y; };
struct Geometry { Room rooms[3]; PlanScale scale; };
bool valid_dimensions(const FloorplanConfig &);
bool overlaps(const FloorplanConfig &);
bool valid_config(const FloorplanConfig &);
PlanScale fit(const FloorplanConfig &);
Geometry calculate_geometry(const FloorplanConfig &);
Geometry calculate_geometry(const FloorplanConfig &,PlanScale);
radar::Point to_plan(PlanScale,float worldXmm,float worldYmm);
radar::Point from_plan(PlanScale,float logicalX,float logicalY);
radar::Point target_world(const FloorplanConfig &,unsigned node,float radarXmm,float radarYmm);
}
