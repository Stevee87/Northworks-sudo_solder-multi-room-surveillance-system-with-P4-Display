#pragma once
#include "sonar_detector.h"
namespace radar_feedback {
constexpr unsigned history_size=16;
struct Sample { radar::Target target{}; uint32_t at=0; };
struct Node {
  radar::Target target{};
  radar::Target lastTrail{};
  Sample history[history_size]{};
  unsigned head=0,count=0;
  uint32_t lastSample=0,acquiredAt=0,revision=0;
  bool lock=false;
  const Sample &sample(unsigned i) const { return history[(head+history_size-count+i)%history_size]; }
  bool animated(uint32_t now) const { return count>0 || (lock && uint32_t(now-acquiredAt)<500); }
};
class Feedback {
  sonar_audio::Detector detector;
  Node nodes[3];
public:
  void clear_trails();
  unsigned update(const system_state::Settings &,const radar::TargetFrame &,uint32_t now,sonar_audio::DetectionEvent events[3]);
  const Node &node(unsigned i) const { return nodes[i]; }
};
Feedback &shared();
float sweep_angle(unsigned node,uint32_t now);
}
