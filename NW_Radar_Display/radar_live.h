#pragma once
#include <cstdint>
#include <cstddef>
#include "radar_model.h"
namespace radar_live {
struct Packet { uint32_t node,sequence,uptime; bool valid; int32_t x,y; };
struct SignalPacket { uint32_t node,sequence,uptime; int32_t rssi; };
struct Node {
  bool seen=false;
  uint32_t receivedAt=0,sequence=0,uptime=0,packets=0;
  radar::Target target{};
  bool signalSeen=false;
  uint32_t signalAt=0,signalSequence=0,signalUptime=0;
  int rssi=0; // zero = unknown; real RSSI values are negative dBm
};
bool parse(const char *data,size_t length,Packet &out);
bool parse_signal(const char *,size_t,SignalPacket &);
bool accept_signal(Node &,const SignalPacket &,uint32_t now);
int fresh_rssi(const Node &,uint32_t now);
bool accept(Node &node,const Packet &packet,uint32_t now);
bool online(const Node &node,uint32_t now);
radar::Target fresh_target(const Node &node,uint32_t now);
void begin();
// Called exclusively by the LVGL loop; networking task never touches LVGL.
void poll_ui();
}
