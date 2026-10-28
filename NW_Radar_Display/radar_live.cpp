#include "radar_live.h"
#include "radar_map.h"
#include "system_state.h"
#include "board_select.h"
#include <Arduino.h>
#if defined(BOARD_M5STACK_TAB5) || defined(BOARD_ELECROW_P4)
#include <WiFi.h>
#include <WiFiUdp.h>
#include <atomic>
#if !CONFIG_ESP_WIFI_REMOTE_ENABLED
#error "This P4 board requires an ESP-Hosted remote-enabled SDK"
#endif
#if defined(BOARD_M5STACK_TAB5) && !defined(BOARD_HAS_SDIO_ESP_HOSTED)
#error "Tab5 requires the ESP-Hosted SDIO Wi-Fi board variant and remote-enabled SDK"
#endif
#ifdef BOARD_ELECROW_P4
#include "board_config.h"
#endif
namespace radar_live {
namespace {
Node nodes[radar::node_count]{};
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
bool apReady=false;
std::atomic<unsigned> activeCount{1};
void network_task(void *) {
#ifdef BOARD_ELECROW_P4
  // Must precede WiFi.mode()/softAP(): this board is not the Tab5 variant.
  if(!WiFi.setPins(WIFI_HOSTED_SDIO_PIN_CLK,WIFI_HOSTED_SDIO_PIN_CMD,
      WIFI_HOSTED_SDIO_PIN_D0,WIFI_HOSTED_SDIO_PIN_D1,WIFI_HOSTED_SDIO_PIN_D2,
      WIFI_HOSTED_SDIO_PIN_D3,WIFI_HOSTED_SDIO_PIN_RESET)) {
    Serial.println("RADAR: Elecrow V1.0 C6 SDIO pin configuration failed");
    vTaskDelete(nullptr); return;
  }
  Serial.println("RADAR: Elecrow V1.0 C6 SDIO clk=53 cmd=54 d0=52 d1=51 d2=50 d3=49 reset=20");
#endif
  WiFiUDP udp;
  uint32_t rejected=0,lastLog=millis(),previousPackets[3]{};
  bool wasOnline[3]{};
  for(;;) {
    const IPAddress ip(192,168,4,1), mask(255,255,255,0);
    Serial.println("RADAR AP: starting ESP-Hosted/C6 SDIO hotspot NW-Radar");
    const bool ready=WiFi.mode(WIFI_AP) && WiFi.softAPConfig(ip,ip,mask) &&
                     WiFi.softAP("NW-Radar","NWradar2026",1,false,3) && udp.begin(5006);
    portENTER_CRITICAL(&mux); apReady=ready; portEXIT_CRITICAL(&mux);
    Serial.printf("RADAR AP: %s IP=%s UDP=5006\n",ready?"READY":"FAILED",WiFi.softAPIP().toString().c_str());
    if(!ready) { udp.stop(); vTaskDelay(pdMS_TO_TICKS(5000)); continue; }
    for(;;) {
      // Bounded work per iteration; display lives in a different task.
      for(unsigned budget=0;budget<16;++budget) {
        const int size=udp.parsePacket(); if(size<=0) break;
        char buffer[96];
        if(size>=int(sizeof(buffer))) { udp.clear(); ++rejected; continue; }
        const int count=udp.read(reinterpret_cast<uint8_t*>(buffer),size);
        if(count!=size) { ++rejected;continue; }
        Packet packet{};SignalPacket signal{};
        const bool targetPacket=parse(buffer,size,packet);
        const bool signalPacket=!targetPacket&&parse_signal(buffer,size,signal);
        if(!targetPacket&&!signalPacket) { ++rejected;continue; }
        const uint32_t now=millis();
        portENTER_CRITICAL(&mux);
        const bool accepted=targetPacket?accept(nodes[packet.node-1],packet,now):accept_signal(nodes[signal.node-1],signal,now);
        portEXIT_CRITICAL(&mux);
        if(!accepted) ++rejected;
      }
      const uint32_t now=millis(); Node snapshot[3];
      portENTER_CRITICAL(&mux); for(unsigned i=0;i<3;++i) snapshot[i]=nodes[i]; portEXIT_CRITICAL(&mux);
      const bool log=uint32_t(now-lastLog)>=2000;
      for(unsigned i=0;i<3;++i) {
        const auto &node=snapshot[i]; const bool connected=online(node,now);
        if(i<activeCount.load()) {
          if(connected!=wasOnline[i]) Serial.printf("NODE %02u %s%s\n",i+1,connected?"ONLINE":"OFFLINE",connected?"":" (UDP timeout >2000 ms)");
          if(log) Serial.printf("NODE %02u %s pps=%.1f seq=%lu valid=%u x=%.0f y=%.0f age_ms=%lu\n",i+1,
            connected?"ONLINE":"OFFLINE",(node.packets-previousPackets[i])*1000.0f/uint32_t(now-lastLog),
            (unsigned long)node.sequence,unsigned(fresh_target(node,now).valid),node.target.xMm,node.target.yMm,
            (unsigned long)(node.seen?now-node.receivedAt:0));
        }
        wasOnline[i]=connected; if(log) previousPackets[i]=node.packets;
      }
      if(log) {
        Serial.printf("RADAR AP=%s stations=%u rejected=%lu\n",WiFi.softAPIP().toString().c_str(),unsigned(WiFi.softAPgetStationNum()),(unsigned long)rejected);
        lastLog=now;
      }
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}
}
void begin() {
  if(xTaskCreate(network_task,"radar-udp",6144,nullptr,1,nullptr)!=pdPASS)
    Serial.println("RADAR: network task creation failed; DEMO remains available");
}
void poll_ui() {
  activeCount.store(system_state::settings().activeNodes);
  static uint32_t last=0;
  const uint32_t now=millis(); if(uint32_t(now-last)<30) return; last=now;
  Node snapshot[radar::node_count];
  portENTER_CRITICAL(&mux);
  for(unsigned i=0;i<radar::node_count;++i) snapshot[i]=nodes[i];
  portEXIT_CRITICAL(&mux);
  radar::TargetFrame frame{};
  for(unsigned i=0;i<radar::node_count;++i) {
    frame.nodes[i].targets[0]=fresh_target(snapshot[i],now);
    radar_map::set_node_status(i,online(snapshot[i],now),frame.nodes[i].targets[0].valid);
    system_state::set_rssi(i,fresh_rssi(snapshot[i],now));
  }
  system_state::set_live(frame);
}
}
#else
namespace radar_live { void begin() {} void poll_ui() {} }
#endif
