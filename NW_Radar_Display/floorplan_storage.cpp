#include "floorplan_storage.h"
#include "radar_model.h"
#include <Preferences.h>
namespace floorplan_storage {
namespace {
constexpr uint32_t magic=0x4E575232;
struct Stored { uint32_t signature;uint16_t version,bytes;floorplan::FloorplanConfig config; };
}
floorplan::FloorplanConfig load() {
  auto result=floorplan::defaults;
  Preferences p;
  if(p.begin("floorplan",true)) {
    const auto length=p.getBytesLength("layout-v2");
    if(length) {
      Stored stored{};
      if(length==sizeof(stored)&&p.getBytes("layout-v2",&stored,sizeof(stored))==sizeof(stored)
         &&stored.signature==magic&&stored.version==2&&stored.bytes==sizeof(stored.config)
         &&floorplan::valid_config(stored.config))result=stored.config;
      p.end();return result;
    }
    p.end();
  }
  // Earlier releases stored room-relative sensor positions in a separate namespace.
  // Convert them using the user's established 5x3m / 1.8x6m room sizes.
  if(p.begin("floorradar",true)) {
    radar::Config legacy{};
    if(p.getBytesLength("layout-v1")==sizeof(legacy)&&p.getBytes("layout-v1",&legacy,sizeof(legacy))==sizeof(legacy)&&radar::valid_config(legacy)) {
      for(unsigned i=0;i<3;++i) result.sensors[i]={legacy.nodes[i].normalizedX*result.rooms[i].widthMm,
        legacy.nodes[i].normalizedY*result.rooms[i].heightMm,legacy.nodes[i].angleDeg};
    }
    p.end();
  }
  return result;
}
bool save(const floorplan::FloorplanConfig &config) {
  if(!floorplan::valid_config(config))return false;
  Preferences p;if(!p.begin("floorplan",false))return false;
  const Stored stored={magic,2,sizeof(config),config};
  // One versioned NVS blob commits rooms and sensor coordinates together.
  const bool ok=p.putBytes("layout-v2",&stored,sizeof(stored))==sizeof(stored);p.end();return ok;
}
}
