#pragma once
#include "radar_model.h"
namespace system_state {
enum class AudioMode : uint8_t { Off,Classic,Tactical };
struct Settings {
  uint8_t activeNodes=1,scalePercent=65;
  AudioMode audioMode=AudioMode::Tactical;
  uint8_t volume=65;
};
bool valid(const Settings &);
Settings load();
bool save(const Settings &);
const Settings &settings();
void apply(const Settings &);
bool enabled(unsigned);
bool &demo_mode();
void set_live(const radar::TargetFrame &);
void set_status(unsigned,bool,bool);
const char *status(unsigned);
void set_rssi(unsigned,int dbm);
int rssi(unsigned); // zero = unavailable/offline/demo/disabled
radar::TargetFrame frame(uint32_t now);
unsigned online_count();
}
