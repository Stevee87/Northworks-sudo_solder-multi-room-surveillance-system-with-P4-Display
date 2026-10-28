#include "radar_storage.h"
#include <Arduino.h>
#include <Preferences.h>

namespace radar_storage {
static void report(const char *operation,const radar::Config &c) {
  Serial.printf("RADAR %s: scale=%.0f",operation,c.mapWidthMm);
  for (const auto &p:c.nodes) Serial.printf(" N%u=%.6f,%.6f,%.1f",p.nodeId,p.normalizedX,p.normalizedY,p.angleDeg);
  Serial.println();
}
radar::Config load() {
  Preferences prefs;
  auto c=radar::defaults();
  if (prefs.begin("radarmap",true)) {
    radar::Config stored{};
    if (prefs.getBytesLength("layout-v1")==sizeof(stored)
        && prefs.getBytes("layout-v1",&stored,sizeof(stored))==sizeof(stored)
        && radar::valid_config(stored)) c=stored;
    prefs.end();
  }
  report("LOAD",c);
  return c;
}
bool save(const radar::Config &c) {
  if (!radar::valid_config(c)) return false;
  Preferences prefs;
  if (!prefs.begin("radarmap",false)) return false;
  const bool ok=prefs.putBytes("layout-v1",&c,sizeof(c))==sizeof(c);
  prefs.end();
  report(ok?"SAVE OK":"SAVE FAILED",c);
  return ok;
}
}
