#pragma once
#include <cstddef>
#include <cstdint>

namespace sonar_audio {
inline void stereo_pcm(const int16_t *mono,int16_t *stereo,size_t count,unsigned volume) {
  if(volume>100) volume=100;
  for(size_t i=0;i<count;++i) {
    const int16_t value=int32_t(mono[i])*int32_t(volume)/100;
    stereo[i*2]=value; stereo[i*2+1]=value;
  }
}
}
