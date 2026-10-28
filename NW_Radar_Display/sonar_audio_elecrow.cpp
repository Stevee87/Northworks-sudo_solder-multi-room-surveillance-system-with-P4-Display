#ifdef ARDUINO
#include "board_select.h"
#ifdef BOARD_ELECROW_P4
#include "sonar_audio.h"
#include "sonar_i2s_pcm.h"
#include "board_config.h"
#include "bsp_stc8h1kxx.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <esp_heap_caps.h>
#include <algorithm>

namespace sonar_audio {
namespace {
struct Control { system_state::Settings settings; radar::TargetFrame frame; uint32_t uiTime=0; } control;
PendingEvents pendingEvents;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
bool workerStarted=false,playbackBusy=false;
uint32_t blockedAt=0,blockedFor=0;
Control snapshot() {
  portENTER_CRITICAL(&mux); const auto next=control; portEXIT_CRITICAL(&mux);
  return next;
}
bool allowed(const Control &next,system_state::AudioMode mode) {
  if(next.settings.audioMode!=mode || mode==system_state::AudioMode::Off || !next.settings.volume) return false;
  // One common alarm for the active scene, so another visible target can keep
  // the sound intact when the original triggering node briefly loses tracking.
  for(unsigned i=0;i<std::min(3u,unsigned(next.settings.activeNodes));++i)
    if(radar::in_sector(next.frame.nodes[i].targets[0])) return true;
  return false;
}

void task(void *) {
  auto *samples=static_cast<int16_t*>(heap_caps_malloc(sample_count*sizeof(int16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!samples) samples=static_cast<int16_t*>(heap_caps_malloc(sample_count*sizeof(int16_t),MALLOC_CAP_8BIT));
  if(!samples) { Serial.println("SONAR: Elecrow PCM allocation failed"); vTaskDelete(nullptr); return; }
  I2SClass speaker;
  speaker.setPins(AUDIO_GPIO_BCLK,AUDIO_GPIO_LRCLK,AUDIO_GPIO_SDATA);
  constexpr size_t chunkFrames=240; // 10 ms; bounded cancellation/volume updates.
  int16_t stereo[chunkFrames*2];
  const int16_t silence[chunkFrames]{};
  for(;;) {
    Control next; DetectionEvent event; bool queued=false;
    portENTER_CRITICAL(&mux);
    next=control;
    if(next.settings.audioMode==system_state::AudioMode::Off || !next.settings.volume) pendingEvents.clear();
    else if(!playbackBusy && uint32_t(millis()-blockedAt)>=blockedFor) {
      queued=pendingEvents.take(control.frame,control.settings.activeNodes,control.uiTime,event);
      if(queued) {
        // Simultaneous detections are one alert, never a train of node sounds.
        pendingEvents.clear(); playbackBusy=true;
      }
    }
    portEXIT_CRITICAL(&mux);
    if(!queued) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    const auto mode=next.settings.audioMode;
    if(!allowed(next,mode)) {
      portENTER_CRITICAL(&mux); playbackBusy=false; portEXIT_CRITICAL(&mux);
      continue;
    }
    synthesize({mode,mode==system_state::AudioMode::Tactical,event.distanceMm},samples,sample_count);
    bool ready=stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD,AUDIO_POWER_DISABLE)==ESP_OK;
    if(ready) ready=speaker.begin(I2S_MODE_STD,sample_rate,I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO,I2S_STD_SLOT_BOTH);
    if(ready) ready=stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD,AUDIO_POWER_ENABLE)==ESP_OK;
    bool complete=ready;
    if(ready) {
      Serial.printf("SONAR ACQUIRED node=%u range_mm=%.0f backend=ELECROW_I2S\n",event.node+1,event.distanceMm);
      // A silence tail pushes all PCM out of DMA before amplifier shutdown.
      // It is never an additional sound or a periodic sweep alert.
      for(size_t offset=0;offset<sample_count+sample_rate/10;offset+=chunkFrames) {
        next=snapshot();
        if(!allowed(next,mode)) { complete=false; break; }
        const bool tail=offset>=sample_count;
        const size_t count=tail?chunkFrames:std::min(chunkFrames,sample_count-offset);
        stereo_pcm(tail?silence:samples+offset,stereo,count,next.settings.volume);
        size_t written=0;
        const auto result=i2s_channel_write(speaker.txChan(),stereo,count*2*sizeof(int16_t),&written,30);
        if(result!=ESP_OK || written!=count*2*sizeof(int16_t)) { complete=false; break; }
        taskYIELD();
      }
    } else Serial.println("SONAR: Elecrow I2S/amplifier initialization failed");
    stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD,AUDIO_POWER_DISABLE);
    speaker.end();
    portENTER_CRITICAL(&mux);
    pendingEvents.clear(); blockedAt=millis(); blockedFor=ready?400:2000;
    playbackBusy=false;
    portEXIT_CRITICAL(&mux);
    if(ready && !complete) Serial.println("SONAR: Elecrow playback stopped (target/mode/mute or I2S error)");
  }
}
}
void begin() {
  portENTER_CRITICAL(&mux);
  if(workerStarted) { portEXIT_CRITICAL(&mux); return; }
  workerStarted=true;
  portEXIT_CRITICAL(&mux);
  if(xTaskCreate(task,"elecrow-sonar",6144,nullptr,1,nullptr)!=pdPASS) {
    portENTER_CRITICAL(&mux); workerStarted=false; portEXIT_CRITICAL(&mux);
    Serial.println("SONAR: Elecrow task creation failed");
  }
}
void update(const system_state::Settings &settings,const radar::TargetFrame &frame,uint32_t now) {
  portENTER_CRITICAL(&mux);
  if(settings.audioMode!=control.settings.audioMode || settings.audioMode==system_state::AudioMode::Off || !settings.volume) pendingEvents.clear();
  control={settings,frame,now}; portEXIT_CRITICAL(&mux);
}
void submit(const DetectionEvent &event) {
  portENTER_CRITICAL(&mux);
  if(!playbackBusy && uint32_t(millis()-blockedAt)>=blockedFor &&
     control.settings.audioMode!=system_state::AudioMode::Off && control.settings.volume && event.node<control.settings.activeNodes)
    pendingEvents.push(event);
  portEXIT_CRITICAL(&mux);
}
}
#endif
#endif
