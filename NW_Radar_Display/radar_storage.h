#pragma once
#include "radar_model.h"
namespace radar_storage {
radar::Config load();
bool save(const radar::Config &);
}
