#pragma once
#include "radar_model.h"
namespace floor_radar_storage {
radar::Config load(const radar::Config &fallback);
bool save(const radar::Config &);
}
