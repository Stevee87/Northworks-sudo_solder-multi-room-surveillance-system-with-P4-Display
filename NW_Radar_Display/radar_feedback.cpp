#include "radar_feedback.h"
namespace radar_feedback {
Feedback &shared() { static Feedback value; return value; }
void Feedback::clear_trails() {
  for(auto &n:nodes) { n.head=0; n.count=0; n.lastTrail={}; n.lock=false; ++n.revision; }
}
float sweep_angle(unsigned node,uint32_t now) {
  const float phase=float((now+node*370u)%2600u)/1300.0f;
  return -60+120*(phase<1?phase:2-phase);
}
unsigned Feedback::update(const system_state::Settings &settings,const radar::TargetFrame &frame,uint32_t now,sonar_audio::DetectionEvent events[3]) {
  // One detector feeds visual confirmation and audio, even while audio is OFF.
  auto visualSettings=settings; visualSettings.audioMode=system_state::AudioMode(2); visualSettings.volume=100;
  detector.update(visualSettings,frame,now);
  for(unsigned i=0;i<3;++i) {
    auto &n=nodes[i];
    if(i>=settings.activeNodes) { n={}; continue; }
    if(n.lock && uint32_t(now-n.acquiredAt)>=500) { n.lock=false; ++n.revision; }
    const auto next=frame.nodes[i].targets[0];
    const bool valid=radar::in_sector(next),previous=radar::in_sector(n.target);
    if(valid!=previous || (valid && (next.xMm!=n.target.xMm || next.yMm!=n.target.yMm))) ++n.revision;
    if(valid && !previous) { n.count=0; n.head=0; n.lastSample=now-67; n.lastTrail={}; }
    n.target=valid?next:radar::Target{};
    while(n.count && uint32_t(now-n.sample(0).at)>=1000) { --n.count; ++n.revision; }
    if(valid && uint32_t(now-n.lastSample)>=67) {
      bool moved=!n.lastTrail.valid || n.lastTrail.xMm!=next.xMm || n.lastTrail.yMm!=next.yMm;
      if(moved) { n.history[n.head]={next,now}; n.lastTrail=next; n.head=(n.head+1)%history_size; if(n.count<history_size) ++n.count; ++n.revision; }
      n.lastSample=now;
    }
  }
  unsigned count=0;
  sonar_audio::DetectionEvent event;
  while(count<3 && detector.take(frame,settings.activeNodes,now,event)) {
    auto &n=nodes[event.node]; n.acquiredAt=now; n.lock=true; n.count=0; n.head=0; n.lastSample=now-67; n.lastTrail={}; ++n.revision;
    events[count++]=event;
  }
  return count;
}
}
