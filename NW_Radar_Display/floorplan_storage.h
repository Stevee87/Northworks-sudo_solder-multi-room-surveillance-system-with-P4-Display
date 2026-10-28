#pragma once
#include "floorplan_geometry.h"

namespace floorplan_storage {
floorplan::FloorplanConfig load();
bool save(const floorplan::FloorplanConfig &config);
}
