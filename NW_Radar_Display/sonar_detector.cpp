#include "sonar_detector.h"
#include <cmath>
namespace sonar_audio {
void PendingEvents::clear() { for(auto &p:pending) p.valid=false; }
void Detector::update(const system_state::Settings &settings,const radar::TargetFrame &frame,uint32_t now) {
  const bool muted=settings.audioMode==system_state::AudioMode::Off || settings.volume==0;
  if(muted) clear_pending();
  for(unsigned i=0;i<3;++i) {
    auto &s=states[i];
    if(i>=settings.activeNodes) { s={}; events.cancel(i); continue; }
    const auto &target=frame.nodes[i].targets[0];
    if(radar::in_sector(target)) {
      if(!s.present) { s.present=true; s.since=now; }
      s.absent=false;
      if(s.armed && uint32_t(now-s.since)>=150) {
        s.armed=false;
        if(!muted) events.push({i,now,std::sqrt(target.xMm*target.xMm+target.yMm*target.yMm)});
      }
    } else {
      s.present=false; events.cancel(i);
      if(!s.absent) { s.absent=true; s.lostAt=now; }
      if(uint32_t(now-s.lostAt)>=1000) s.armed=true;
    }
  }
}
bool PendingEvents::take(const radar::TargetFrame &frame,unsigned active,uint32_t now,DetectionEvent &out) {
  int chosen=-1;
  for(unsigned i=0;i<3;++i) {
    auto &p=pending[i];
    if(!p.valid) continue;
    if(i>=active || !radar::in_sector(frame.nodes[i].targets[0]) || uint32_t(now-p.event.at)>2000) { p.valid=false; continue; }
    if(chosen<0 || uint32_t(now-p.event.at)>uint32_t(now-pending[chosen].event.at)) chosen=int(i);
  }
  if(chosen<0) return false;
  out=pending[chosen].event; pending[chosen].valid=false; return true;
}
}
