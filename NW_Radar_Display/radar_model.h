#pragma once
#include <cstdint>

namespace radar {
constexpr unsigned node_count=3, target_count=3;
constexpr float range_mm=6000.0f;
struct RadarPlacement { uint8_t nodeId; float normalizedX, normalizedY, angleDeg; };
struct Config { RadarPlacement nodes[node_count]; float mapWidthMm; };
struct Point { float x,y; };
struct MapScale { float x,y,width,height,widthMm; float visualScale=1.0f; };
struct Bounds { float x1,y1,x2,y2; }; // inclusive display coordinates
struct RoomSize { float widthMm,heightMm; }; // physical plan X/Y dimensions
struct Target { bool valid; float xMm,yMm; };
struct NodeTargets { Target targets[target_count]; };
struct TargetFrame { NodeTargets nodes[node_count]; };
Config defaults();
bool valid_config(const Config &config);
// LD2450 decoded coordinates: +X sensor-right, +Y forward, millimetres.
// Heading: 0 degrees up, 90 right, clockwise. Output uses display +Y down.
Point local_to_map(const RadarPlacement &,float xMm,float yMm,const MapScale &);
// Rotate in physical millimetres first, then map each room axis to its pixels.
// Placement is relative to this room; no global zoom or range auto-fit.
Point local_to_room(const RadarPlacement &,float xMm,float yMm,const Bounds &,const RoomSize &);
// Uniform display scale: the 6m center ray reaches the forward room wall.
// Physical sensor coordinates/range are unchanged; X and Y share one scale.
MapScale fit_room(const RadarPlacement &,const MapScale &,const Bounds &);
bool in_sector(const Target &target);
TargetFrame demo_frame(uint32_t elapsedMs);
}
