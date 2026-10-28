#pragma once
#include "system_state.h"
#include "sonar_detector.h"
#include <cstddef>
namespace sonar_audio {
constexpr unsigned sample_rate=24000;
constexpr size_t sample_count=9600;
struct Plan { system_state::AudioMode mode; bool echo; float distanceMm; };
float echo_delay_ms(float distanceMm);
float echo_gain(float distanceMm);
void synthesize(const Plan &,int16_t *samples,size_t count);
int select_target(const radar::TargetFrame &,unsigned active,unsigned &cursor);
void begin();
void update(const system_state::Settings &,const radar::TargetFrame &,uint32_t now=0);
void submit(const DetectionEvent &);
}
