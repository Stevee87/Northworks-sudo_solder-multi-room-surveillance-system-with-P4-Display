#include "sonar_audio.h"
#include "tactical_pcm.h"
#include <cmath>
#include <algorithm>
namespace sonar_audio {
namespace {
constexpr float pi=3.14159265359f;
float normalized(float d) { return std::max(0.0f,std::min(1.0f,d/6000)); }
static_assert(sample_rate==tactical_pcm::rate && sample_count==tactical_pcm::count,"PCM format must match generated asset");
}
float echo_delay_ms(float d) { return 120+380*normalized(d); }
float echo_gain(float d) { return .45f-.20f*normalized(d); }
int select_target(const radar::TargetFrame &f,unsigned active,unsigned &cursor) {
  active=std::min(3u,active); if(!active) return -1;
  for(unsigned j=0;j<active;++j) {
    const unsigned i=(cursor+j)%active;
    if(radar::in_sector(f.nodes[i].targets[0])) { cursor=(i+1)%active; return int(i); }
  }
  return -1;
}
void synthesize(const Plan &plan,int16_t *samples,size_t count) {
  for(size_t i=0;i<count;++i) {
    const float t=float(i)/sample_rate;
    float value=0;
    if(plan.mode==system_state::AudioMode::Classic && t<.16f) {
      const float attack=std::min(1.0f,t/.008f), release=std::min(1.0f,(.16f-t)/.045f);
      const float phase=2*pi*1640*t;
      value=.55f*attack*release*(std::sin(phase)+.12f*std::sin(phase*2));
    } else if(plan.mode==system_state::AudioMode::Tactical) {
      value=i<tactical_pcm::count?tactical_pcm::alert[i]/32767.0f:0;
    }
    samples[i]=int16_t(std::max(-.95f,std::min(.95f,value))*30000);
  }
}
}
