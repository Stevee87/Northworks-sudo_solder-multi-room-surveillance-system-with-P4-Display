#include "sonar_audio.h"
#include "sonar_detector.h"
#include <cmath>
#ifdef ARDUINO
#include "board_select.h"
#include <Arduino.h>
#ifdef BOARD_M5STACK_TAB5
#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <atomic>
namespace sonar_audio {
namespace {
struct Control { system_state::Settings settings; radar::TargetFrame frame; uint32_t uiTime=0; } control;
PendingEvents pendingEvents;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
std::atomic<bool> released{true};
void release_buffer(void *,const void *,uint8_t) { released.store(true); }
void task(void *) {
  auto *samples=static_cast<int16_t*>(heap_caps_malloc(sample_count*sizeof(int16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!samples) samples=static_cast<int16_t*>(heap_caps_malloc(sample_count*sizeof(int16_t),MALLOC_CAP_8BIT));
  if(!samples) { Serial.println("SONAR: PCM allocation failed"); vTaskDelete(nullptr); return; }
  bool running=false;
  int echoNode=-1;
  uint32_t cooldownAt=0,cooldownMs=0;
  auto previousMode=system_state::AudioMode::Off;
  for(;;) {
    Control next;
    portENTER_CRITICAL(&mux); next=control; portEXIT_CRITICAL(&mux);
    const bool mute=next.settings.audioMode==system_state::AudioMode::Off || next.settings.volume==0;
    const bool lost=echoNode>=0 && (unsigned(echoNode)>=next.settings.activeNodes || !next.frame.nodes[echoNode].targets[0].valid);
    const bool changed=previousMode!=next.settings.audioMode;
    const uint32_t now=millis();
    if(mute) { portENTER_CRITICAL(&mux); pendingEvents.clear(); portEXIT_CRITICAL(&mux); }
    if(running && (mute || lost || changed)) {
      // end() guarantees the playback task has released PCM before reuse,
      // including queued samples which stop() may discard without a callback.
      M5.Speaker.end(); running=false; released.store(true); echoNode=-1; cooldownAt=now; cooldownMs=200;
    }
    previousMode=next.settings.audioMode;
    if(echoNode>=0 && released.load() && !M5.Speaker.isPlaying()) {
      echoNode=-1; cooldownAt=now; cooldownMs=200;
    }
    DetectionEvent event;
    bool queued=false;
    if(!mute && echoNode<0 && uint32_t(now-cooldownAt)>=cooldownMs && released.load() && !M5.Speaker.isPlaying()) {
      // Pair queued events with the current UI snapshot, not the older task
      // copy: a newly submitted event can arrive between these two locks.
      portENTER_CRITICAL(&mux);
      queued=pendingEvents.take(control.frame,control.settings.activeNodes,control.uiTime,event);
      if(queued) next=control;
      portEXIT_CRITICAL(&mux);
    }
    if(queued) {
      if(!running) {
        running=M5.Speaker.begin();
        if(running) M5.Speaker.setBufferReleaseCallback(nullptr,release_buffer);
        else { Serial.println("SONAR: Tab5 speaker initialization failed"); cooldownAt=now; cooldownMs=2000; }
      }
      if(running) {
        echoNode=int(event.node);
        const Plan plan{next.settings.audioMode,next.settings.audioMode==system_state::AudioMode::Tactical,event.distanceMm};
        synthesize(plan,samples,sample_count);
        M5.Speaker.setVolume(uint8_t(next.settings.volume*255/100));
        released.store(false);
        if(!M5.Speaker.playRaw(samples,sample_count,sample_rate,false,1,0,false)) { released.store(true); echoNode=-1; cooldownAt=now; cooldownMs=200; }
        Serial.printf("SONAR ACQUIRED node=%u range_mm=%.0f\n",event.node+1,event.distanceMm);
      }
    }
    if(running) M5.Speaker.setVolume(uint8_t(next.settings.volume*255/100));
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
}
void begin() { if(xTaskCreate(task,"shared-sonar",6144,nullptr,1,nullptr)!=pdPASS) Serial.println("SONAR task failed"); }
void update(const system_state::Settings &settings,const radar::TargetFrame &frame,uint32_t now) {
  portENTER_CRITICAL(&mux);
  if(settings.audioMode!=control.settings.audioMode || settings.audioMode==system_state::AudioMode::Off || !settings.volume) pendingEvents.clear();
  control={settings,frame,now}; portEXIT_CRITICAL(&mux);
}
void submit(const DetectionEvent &event) {
  portENTER_CRITICAL(&mux);
  if(control.settings.audioMode!=system_state::AudioMode::Off && control.settings.volume && event.node<control.settings.activeNodes) pendingEvents.push(event);
  portEXIT_CRITICAL(&mux);
}
}
#elif !defined(BOARD_ELECROW_P4)
namespace sonar_audio { void begin() {} void update(const system_state::Settings &,const radar::TargetFrame &,uint32_t) {} void submit(const DetectionEvent &) {} }
#endif
#else
namespace sonar_audio { void begin() {} void update(const system_state::Settings &,const radar::TargetFrame &,uint32_t) {} void submit(const DetectionEvent &) {} }
#endif
