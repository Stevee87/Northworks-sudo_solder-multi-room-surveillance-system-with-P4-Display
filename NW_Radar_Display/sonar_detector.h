#pragma once
#include "system_state.h"
namespace sonar_audio {
struct DetectionEvent { unsigned node=0; uint32_t at=0; float distanceMm=0; };
class PendingEvents {
  struct Pending { bool valid=false; DetectionEvent event; } pending[3];
public:
  void push(const DetectionEvent &event) { if(event.node<3) pending[event.node]={true,event}; }
  void cancel(unsigned node) { if(node<3) pending[node].valid=false; }
  void clear();
  bool take(const radar::TargetFrame &,unsigned active,uint32_t now,DetectionEvent &out);
};
class Detector {
  struct State { bool armed=true,present=false,absent=false; uint32_t since=0,lostAt=0; } states[3];
  PendingEvents events;
public:
  void update(const system_state::Settings &,const radar::TargetFrame &,uint32_t now);
  bool take(const radar::TargetFrame &frame,unsigned active,uint32_t now,DetectionEvent &out) { return events.take(frame,active,now,out); }
  void clear_pending() { events.clear(); }
};
}
