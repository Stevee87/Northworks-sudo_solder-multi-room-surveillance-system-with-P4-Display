#include "floor_radar_storage.h"
#ifdef ARDUINO
#include <Preferences.h>
#endif
namespace floor_radar_storage {
radar::Config load(const radar::Config &fallback) {
  auto result=fallback;
#ifdef ARDUINO
  Preferences p;
  if(p.begin("floorradar",true)) {
    radar::Config candidate{};
    if(p.getBytesLength("layout-v1")==sizeof(candidate) &&
       p.getBytes("layout-v1",&candidate,sizeof(candidate))==sizeof(candidate) && radar::valid_config(candidate)) result=candidate;
    p.end();
  }
#endif
  return result;
}
bool save(const radar::Config &config) {
  if(!radar::valid_config(config)) return false;
#ifdef ARDUINO
  Preferences p; if(!p.begin("floorradar",false)) return false;
  const bool ok=p.putBytes("layout-v1",&config,sizeof(config))==sizeof(config); p.end(); return ok;
#else
  return true;
#endif
}
}
