#include "radar_live.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
namespace radar_live {
namespace {
bool decode_fields(const char *data,size_t length,const char *prefix,unsigned count,unsigned signedMask,int64_t *values) {
  if(!data||length<5||length>=96||std::memchr(data,0,length))return false;
  char text[96];std::memcpy(text,data,length);
  while(length&&(text[length-1]=='\r'||text[length-1]=='\n'))--length;
  text[length]=0;if(length<5||std::memcmp(text,prefix,5))return false;
  char *p=text+5;
  for(unsigned i=0;i<count;++i) {
    char *start=p;if((signedMask&(1u<<i))&&*p=='-')++p;
    if(*p<'0'||*p>'9')return false;
    while(*p>='0'&&*p<='9')++p;
    if((i+1<count&&*p!=',')||(i+1==count&&*p!=0))return false;
    const char delimiter=*p;*p=0;errno=0;char *end=nullptr;
    values[i]=std::strtoll(start,&end,10);if(errno||*end)return false;
    if(delimiter)++p;
  }
  return true;
}
}
bool parse(const char *data,size_t length,Packet &out) {
  int64_t fields[6]{};if(!decode_fields(data,length,"NWR1,",6,(1u<<4)|(1u<<5),fields))return false;
  if(fields[0]<1 || fields[0]>3 || fields[1]<0 || fields[1]>UINT32_MAX ||
     fields[2]<0 || fields[2]>UINT32_MAX || fields[3]<0 || fields[3]>1 ||
     fields[4]<-6000 || fields[4]>6000 || fields[5]<-6000 || fields[5]>6000) return false;
  if(fields[3] && (fields[5]<0 || fields[4]*fields[4]+fields[5]*fields[5]>36000000)) return false;
  out={uint32_t(fields[0]),uint32_t(fields[1]),uint32_t(fields[2]),fields[3]!=0,int32_t(fields[4]),int32_t(fields[5])};
  return true;
}
bool parse_signal(const char *data,size_t length,SignalPacket &out) {
  int64_t f[4]{};if(!decode_fields(data,length,"NWQ1,",4,1u<<3,f))return false;
  if(f[0]<1||f[0]>3||f[1]<0||f[1]>UINT32_MAX||f[2]<0||f[2]>UINT32_MAX||f[3]<-127||f[3]>-1)return false;
  out={uint32_t(f[0]),uint32_t(f[1]),uint32_t(f[2]),int32_t(f[3])};return true;
}
bool online(const Node &n,uint32_t now) { return n.seen && uint32_t(now-n.receivedAt)<=2000; }
bool accept(Node &n,const Packet &p,uint32_t now) {
  if(n.seen && online(n,now)) {
    const uint32_t advance=p.sequence-n.sequence;
    // Allow a reboot only when BOTH counters drop; normal uint32 rollover works.
    const bool reboot=p.uptime<n.uptime && p.sequence<n.sequence && p.uptime<10000;
    if(!reboot && (advance==0 || advance>=0x80000000u)) return false;
    if(reboot)n.signalSeen=false;
  }
  if(!online(n,now))n.signalSeen=false;
  n.seen=true; n.receivedAt=now; n.sequence=p.sequence; n.uptime=p.uptime; ++n.packets;
  n.target={p.valid,float(p.x),float(p.y)};
  return true;
}
bool accept_signal(Node &n,const SignalPacket &p,uint32_t now) {
  // Telemetry cannot make a node online or refresh its target/heartbeat.
  if(!online(n,now)||p.rssi<-127||p.rssi>-1)return false;
  if(n.uptime-p.uptime>1500 && p.uptime-n.uptime>1500)return false;
  if(n.sequence-p.sequence>20 && p.sequence-n.sequence>20)return false;
  if(n.signalSeen) {
    const uint32_t advance=p.sequence-n.signalSequence;
    if(advance==0||advance>=0x80000000u)return false;
  }
  n.signalSeen=true;n.signalAt=now;n.signalSequence=p.sequence;n.signalUptime=p.uptime;n.rssi=int(p.rssi);return true;
}
int fresh_rssi(const Node &n,uint32_t now) {
  return online(n,now)&&n.signalSeen&&uint32_t(now-n.signalAt)<=2500?n.rssi:0;
}
radar::Target fresh_target(const Node &n,uint32_t now) {
  return online(n,now) && uint32_t(now-n.receivedAt)<=650 ? n.target : radar::Target{};
}
}
