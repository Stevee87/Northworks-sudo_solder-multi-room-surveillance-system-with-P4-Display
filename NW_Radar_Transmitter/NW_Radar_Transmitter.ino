/*
  NorthWorks Radar Node Firmware
  Hardware: ESP32-S3 Devboard + Ai-Thinker RD-03D
  Version: 1.0.0

  One radar per node.
  Set NODE_ID to 1, 2 or 3 before flashing each node.

  Wiring:
    RD03D TX -> ESP32-S3 GPIO4 (UART RX)
    RD03D GND -> ESP32-S3 GND
    RD03D VCC -> suitable board supply used in the build
    RD03D RX -> ESP32-S3 GPIO5 (UART TX)
    This sketch receives target frames; no radar configuration commands are sent.

  Radar:
    RD-03D @ 256000 baud
    30-byte target frame:
      AA FF 03 00
      3 x 8-byte target records
      55 CC

  Wi-Fi / UDP:
    SSID: NW-Radar
    UDP destination: 192.168.4.1:5006
    Send rate: 10 Hz

  Packet:
    NWR1,nodeId,sequence,millis,valid,x_mm,y_mm
    NWQ1,nodeId,sequence,millis,rssi_dbm (separate 1 Hz Wi-Fi telemetry)
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

// -----------------------------------------------------------------------------
// USER CONFIG
// -----------------------------------------------------------------------------

#ifndef NODE_ID
#define NODE_ID 1   // Set to 1, 2 or 3 before each upload
#endif

static const char* WIFI_SSID = "NW-Radar";
static const char* WIFI_PASS = "NWradar2026";

// -----------------------------------------------------------------------------
// FIXED PROJECT CONFIG
// -----------------------------------------------------------------------------

static constexpr int RADAR_RX_PIN = 4;
static constexpr int RADAR_TX_PIN = 5;    // Connect to RD-03D RX
static constexpr uint32_t RADAR_BAUD = 256000;

static const IPAddress UDP_DEST_IP(192, 168, 4, 1);
static constexpr uint16_t UDP_DEST_PORT = 5006;

static constexpr uint32_t SEND_INTERVAL_MS = 100;   // 10 Hz
static constexpr uint32_t HOLD_TIME_MS = 500;
static constexpr uint32_t WIFI_RETRY_MS = 2500;

static constexpr float REASSOC_DISTANCE_MM = 850.0f;
static constexpr float FILTER_NEW_WEIGHT = 0.60f;
static constexpr float FILTER_OLD_WEIGHT = 0.40f;

static constexpr int32_t MAX_RANGE_MM = 6000;
static constexpr float MAX_BEARING_DEG = 60.0f;

#if NODE_ID < 1 || NODE_ID > 3
#error "NODE_ID must be 1, 2 or 3"
#endif

// -----------------------------------------------------------------------------
// GLOBALS
// -----------------------------------------------------------------------------

HardwareSerial RadarSerial(1);
WiFiUDP udp;

static uint8_t frameBuf[30];
static uint8_t frameIndex = 0;

static uint32_t sequenceNumber = 0;
static uint32_t lastSendMs = 0;
static uint32_t lastWiFiAttemptMs = 0;

struct RadarTarget {
  bool valid;
  int32_t x;
  int32_t y;
  int32_t speed;
  uint16_t resolution;
};

// Diagnostic output is local USB/serial only; NWR1 radio packets are unchanged.
static uint32_t radarBytes = 0, radarFrames = 0, radarBadFooters = 0;
static uint32_t lastRadarFrameMs = 0;
static RadarTarget diagnosticTargets[3]{};
static uint8_t recentRadarBytes[12]{};
static uint8_t recentByteIndex = 0;

static bool targetLocked = false;
static float trackedX = 0.0f;
static float trackedY = 0.0f;
static uint32_t lastTargetSeenMs = 0;

// -----------------------------------------------------------------------------
// RD03D HELPERS
// -----------------------------------------------------------------------------

static int32_t decodeRD03DSigned(uint8_t lo, uint8_t hi) {
  const uint16_t raw = (uint16_t)lo | ((uint16_t)hi << 8);

  // RD03D uses bit 15 as direction/sign:
  // bit 15 set   -> positive magnitude
  // bit 15 clear -> negative magnitude
  if (raw & 0x8000) {
    return (int32_t)(raw - 0x8000);
  }
  return -(int32_t)raw;
}

static uint16_t decodeU16(uint8_t lo, uint8_t hi) {
  return (uint16_t)lo | ((uint16_t)hi << 8);
}

static bool targetPassesGate(const RadarTarget& t) {
  if (!t.valid) return false;
  if (t.y <= 0) return false;

  const int64_t x2 = (int64_t)t.x * (int64_t)t.x;
  const int64_t y2 = (int64_t)t.y * (int64_t)t.y;
  const int64_t maxRange2 = (int64_t)MAX_RANGE_MM * (int64_t)MAX_RANGE_MM;

  if ((x2 + y2) > maxRange2) return false;

  const float bearingDeg =
      fabsf(atan2f((float)t.x, (float)t.y) * 180.0f / PI);

  if (bearingDeg > MAX_BEARING_DEG) return false;

  return true;
}

static float distanceBetween(float x1, float y1, float x2, float y2) {
  const float dx = x1 - x2;
  const float dy = y1 - y2;
  return sqrtf(dx * dx + dy * dy);
}

static int findNearestValidTarget(const RadarTarget targets[3]) {
  int best = -1;
  float bestRange = 1e30f;

  for (int i = 0; i < 3; ++i) {
    if (!targetPassesGate(targets[i])) continue;

    const float range =
        sqrtf((float)targets[i].x * (float)targets[i].x +
              (float)targets[i].y * (float)targets[i].y);

    if (range < bestRange) {
      bestRange = range;
      best = i;
    }
  }

  return best;
}

static int findReassociatedTarget(const RadarTarget targets[3]) {
  int best = -1;
  float bestDistance = REASSOC_DISTANCE_MM + 1.0f;

  for (int i = 0; i < 3; ++i) {
    if (!targetPassesGate(targets[i])) continue;

    const float d = distanceBetween(
        (float)targets[i].x,
        (float)targets[i].y,
        trackedX,
        trackedY
    );

    if (d <= REASSOC_DISTANCE_MM && d < bestDistance) {
      bestDistance = d;
      best = i;
    }
  }

  return best;
}

static void updateTrackedTarget(const RadarTarget targets[3]) {
  const uint32_t now = millis();
  int selected = -1;

  if (targetLocked) {
    selected = findReassociatedTarget(targets);

    if (selected < 0 && (uint32_t)(now - lastTargetSeenMs) > HOLD_TIME_MS) {
      // Existing lock has expired. Allow acquisition of a new nearest target.
      targetLocked = false;
    }
  }

  if (!targetLocked) {
    selected = findNearestValidTarget(targets);
  }

  if (selected >= 0) {
    const float newX = (float)targets[selected].x;
    const float newY = (float)targets[selected].y;

    if (targetLocked) {
      trackedX = FILTER_NEW_WEIGHT * newX + FILTER_OLD_WEIGHT * trackedX;
      trackedY = FILTER_NEW_WEIGHT * newY + FILTER_OLD_WEIGHT * trackedY;
    } else {
      trackedX = newX;
      trackedY = newY;
      targetLocked = true;
    }

    lastTargetSeenMs = now;
  }
}

static void processRadarFrame(const uint8_t* f) {
  ++radarFrames;
  lastRadarFrameMs = millis();
  RadarTarget targets[3];

  for (int i = 0; i < 3; ++i) {
    const int base = 4 + i * 8;

    targets[i].x = decodeRD03DSigned(f[base + 0], f[base + 1]);
    targets[i].y = decodeRD03DSigned(f[base + 2], f[base + 3]);
    targets[i].speed = decodeRD03DSigned(f[base + 4], f[base + 5]);
    targets[i].resolution = decodeU16(f[base + 6], f[base + 7]);

    targets[i].valid =
        !(targets[i].x == 0 &&
          targets[i].y == 0 &&
          targets[i].speed == 0 &&
          targets[i].resolution == 0);
  }

  for (int i = 0; i < 3; ++i) diagnosticTargets[i] = targets[i];
  updateTrackedTarget(targets);
}

static void feedRadarByte(uint8_t b) {
  ++radarBytes;
  recentRadarBytes[recentByteIndex] = b;
  recentByteIndex = (recentByteIndex + 1) % sizeof(recentRadarBytes);
  static uint32_t lastByteMs = 0;
  const uint32_t now = millis();
  if (frameIndex && (uint32_t)(now - lastByteMs) > 100) frameIndex = 0;
  lastByteMs = now;
  frameBuf[frameIndex++] = b;
  const uint8_t header[] = {0xAA, 0xFF, 0x03, 0x00};
  // Sliding window preserves headers after noise, bad footers and missing bytes.
  for (;;) {
    bool prefix = true;
    for (uint8_t i = 0; i < frameIndex && i < 4; ++i) {
      if (frameBuf[i] != header[i]) { prefix = false; break; }
    }
    if (prefix && frameIndex < sizeof(frameBuf)) return;
    if (prefix && frameBuf[28] == 0x55 && frameBuf[29] == 0xCC) {
      processRadarFrame(frameBuf);
      frameIndex = 0;
      return;
    }
    if (prefix) ++radarBadFooters;
    --frameIndex;
    for (uint8_t i = 0; i < frameIndex; ++i) frameBuf[i] = frameBuf[i + 1];
    if (!frameIndex) return;
  }
}

static void readRadar() {
  while (RadarSerial.available() > 0) {
    feedRadarByte((uint8_t)RadarSerial.read());
  }
}

// -----------------------------------------------------------------------------
// WIFI / UDP
// -----------------------------------------------------------------------------

static void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastWiFiAttemptMs = millis();
}

static void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  const uint32_t now = millis();

  if ((uint32_t)(now - lastWiFiAttemptMs) >= WIFI_RETRY_MS) {
    lastWiFiAttemptMs = now;
    Serial.println("Wi-Fi reconnect...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

static bool currentTargetValid(uint32_t now) {
  if (!targetLocked) return false;

  if ((uint32_t)(now - lastTargetSeenMs) > HOLD_TIME_MS) {
    targetLocked = false;
    return false;
  }

  return true;
}

// Separate 1 Hz telemetry keeps the original NWR1 target packets compatible.
static bool signalSent=false;
static uint32_t lastSignalMs=0;
static void sendSignalPacket(uint32_t now,uint32_t frameSequence) {
  if(signalSent&&uint32_t(now-lastSignalMs)<1000)return;
  signalSent=true;lastSignalMs=now;
  const int rssi=WiFi.RSSI();
  if(rssi<-127||rssi>-1)return;
  char packet[96];
  const int length=snprintf(packet,sizeof(packet),"NWQ1,%d,%lu,%lu,%d",NODE_ID,(unsigned long)frameSequence,(unsigned long)now,rssi);
  if(length<=0||length>=int(sizeof(packet)))return;
  udp.beginPacket(UDP_DEST_IP,UDP_DEST_PORT);udp.write((const uint8_t*)packet,size_t(length));udp.endPacket();
  Serial.println(packet);
}

static void sendUdpPacket() {
  const uint32_t now = millis();
  const bool valid = currentTargetValid(now);

  const int32_t outX = valid ? (int32_t)lroundf(trackedX) : 0;
  const int32_t outY = valid ? (int32_t)lroundf(trackedY) : 0;

  char packet[96];

  const int len = snprintf(
      packet,
      sizeof(packet),
      "NWR1,%d,%lu,%lu,%d,%ld,%ld",
      NODE_ID,
      (unsigned long)sequenceNumber++,
      (unsigned long)now,
      valid ? 1 : 0,
      (long)outX,
      (long)outY
  );

  if (WiFi.status() == WL_CONNECTED && len > 0) {
    udp.beginPacket(UDP_DEST_IP, UDP_DEST_PORT);
    udp.write((const uint8_t*)packet, (size_t)len);
    udp.endPacket();
    sendSignalPacket(now,sequenceNumber-1);
  } else signalSent=false;

  Serial.println(packet);
}

static void printRadarDiagnostics(uint32_t now) {
  static uint32_t lastLogMs = 0, previousBytes = 0, previousFrames = 0;
  if ((uint32_t)(now - lastLogMs) < 2000) return;
  lastLogMs = now;
  unsigned accepted = 0;
  for (unsigned i = 0; i < 3; ++i) if (targetPassesGate(diagnosticTargets[i])) ++accepted;
  Serial.printf("RADAR_DIAG node=%d bytes_2s=%lu frames_2s=%lu bad_footer_total=%lu accepted=%u seen=%u age_ms=%lu\n",
      NODE_ID, (unsigned long)(radarBytes - previousBytes),
      (unsigned long)(radarFrames - previousFrames), (unsigned long)radarBadFooters,
      accepted, radarFrames ? 1u : 0u,
      (unsigned long)(radarFrames ? now - lastRadarFrameMs : 0));
  previousBytes = radarBytes; previousFrames = radarFrames;
  for (unsigned i = 0; i < 3; ++i) {
    const auto &t = diagnosticTargets[i];
    Serial.printf("RADAR_SLOT %u valid=%u x=%ld y=%ld speed=%ld resolution=%u gate=%u\n",
        i + 1, unsigned(t.valid), (long)t.x, (long)t.y, (long)t.speed,
        unsigned(t.resolution), unsigned(targetPassesGate(t)));
  }
  Serial.printf("RADAR_RAW_LAST:");
  const unsigned count = radarBytes < sizeof(recentRadarBytes) ? unsigned(radarBytes) : sizeof(recentRadarBytes);
  const unsigned start = count < sizeof(recentRadarBytes) ? 0 : recentByteIndex;
  for (unsigned i = 0; i < count; ++i)
    Serial.printf(" %02X", unsigned(recentRadarBytes[(start + i) % sizeof(recentRadarBytes)]));
  Serial.println();
}

// -----------------------------------------------------------------------------
// SETUP / LOOP
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("========================================");
  Serial.println("NorthWorks Radar Node Firmware v1.0.0");
  Serial.printf("NODE_ID: %d\n", NODE_ID);
  Serial.println("RD03D: 256000 baud, RX GPIO4, TX GPIO5");
  Serial.println("UDP: 192.168.4.1:5006 @ 10 Hz");
  Serial.println("========================================");

  RadarSerial.setRxBufferSize(1024);
  RadarSerial.begin(
      RADAR_BAUD,
      SERIAL_8N1,
      RADAR_RX_PIN,
      RADAR_TX_PIN
  );

  startWiFi();

  // Local UDP port only needs to be unique enough for the three nodes.
  udp.begin((uint16_t)(5100 + NODE_ID));
}

void loop() {
  readRadar();
  maintainWiFi();

  const uint32_t now = millis();

  if ((uint32_t)(now - lastSendMs) >= SEND_INTERVAL_MS) {
    lastSendMs = now;
    sendUdpPacket();
  }

  printRadarDiagnostics(now);
  delay(1);
}
