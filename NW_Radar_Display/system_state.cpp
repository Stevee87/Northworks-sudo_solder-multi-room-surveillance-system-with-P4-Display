#include "system_state.h"
#ifdef ARDUINO
#include <Preferences.h>
#endif
namespace system_state {
namespace { Settings current; radar::TargetFrame live{}; bool demo=false,connected[3]{},tracked[3]{}; int signal[3]{}; }
bool valid(const Settings &s) {
  return s.activeNodes>=1 && s.activeNodes<=3 && s.scalePercent>=40 && s.scalePercent<=100 &&
         s.scalePercent%5==0 && unsigned(s.audioMode)<=2 && s.volume<=100;
}
const Settings &settings() { return current; }
void apply(const Settings &s) { current=valid(s)?s:Settings{}; }
bool enabled(unsigned i) { return i<current.activeNodes; }
bool &demo_mode() { return demo; }
void set_live(const radar::TargetFrame &f) { live=f; }
void set_status(unsigned i,bool on,bool target) { if(i<3) { connected[i]=on; tracked[i]=on&&target;if(!on)signal[i]=0; } }
void set_rssi(unsigned i,int dbm) { if(i<3)signal[i]=(dbm>=-127&&dbm<=-1)?dbm:0; }
int rssi(unsigned i) { return enabled(i)&&!demo&&connected[i]?signal[i]:0; }
const char *status(unsigned i) {
  if(!enabled(i)) return "DISABLED";
  if(demo) return "DEMO";
  return !connected[i]?"OFFLINE":tracked[i]?"TRACKING":"NO TARGET";
}
unsigned online_count() { unsigned count=0; for(unsigned i=0;i<3;++i) if(enabled(i)&&(demo||connected[i])) ++count; return count; }
radar::TargetFrame frame(uint32_t now) {
  auto f=demo?radar::demo_frame(now):live;
  for(unsigned i=0;i<3;++i) {
    if(!enabled(i)) f.nodes[i]={};
    for(unsigned j=1;j<radar::target_count;++j) f.nodes[i].targets[j]={};
  }
  return f;
}
Settings load() {
  Settings s;
#ifdef ARDUINO
  Preferences p;
  if(p.begin("nw-system",true)) {
    const uint32_t packed=p.getUInt("settings-v1",0);
    if((packed>>28)==1) {
      Settings candidate{uint8_t(packed&3),uint8_t((packed>>2)&127),AudioMode((packed>>9)&3),uint8_t((packed>>11)&127)};
      if(valid(candidate)) s=candidate;
    }
    p.end();
  }
#endif
  return s;
}
bool save(const Settings &s) {
  if(!valid(s)) return false;
#ifdef ARDUINO
  Preferences p; if(!p.begin("nw-system",false)) return false;
  const uint32_t packed=(1u<<28)|s.activeNodes|(uint32_t(s.scalePercent)<<2)|(uint32_t(s.audioMode)<<9)|(uint32_t(s.volume)<<11);
  const bool ok=p.putUInt("settings-v1",packed)==sizeof(packed); p.end(); return ok;
#else
  return true;
#endif
}
}
