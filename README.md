# NW Radar – ESP32-S3 & Elecrow ESP32-P4

Created by Northworks and sudo_solder a surveillance system for under $100

A radar display system for up to three ESP32-S3 transmitters, each connected to
an Ai-Thinker RD-03D, and an Elecrow CrowPanel Advance 5-inch display. The
transmitters send target coordinates over Wi-Fi/UDP. The display shows targets
on a room plan with a consistent physical scale, along with each node's Wi-Fi
signal strength.

<img width="4284" height="5712" alt="tempImagePV9V1e" src="https://github.com/user-attachments/assets/88530656-d923-4512-b8bc-46d4383c4d93" />


**Package date: October 28, 2026 · Firmware: RSSI update from October 27, 2026**

## Package contents

```text
NW_Radar_GitHub/
├── NW_Radar_Display/
│   ├── NW_Radar_Display.ino
│   └── … required .h, .cpp and .c files
├── NW_Radar_Transmitter/
│   └── NW_Radar_Transmitter.ino
├── libraries/
│   ├── lvgl.zip
│   ├── ESP32_Display_Panel.zip
│   ├── ESP32_IO_Expander.zip
│   └── esp-lib-utils.zip
├── docs/
│   ├── USAGE.md
│   ├── PROTOCOL.md
│   ├── VALIDATION.md
│   └── rssi-preview.png
├── CHANGELOG.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
└── SHA256SUMS.txt
```

Both sketches can be edited and uploaded using Arduino IDE on Windows, macOS
or Linux. Keep each sketch folder intact. The display requires all the supplied
source files alongside its `.ino` file.

## Hardware

| Component | Configuration used |
| --- | --- |
| Display | Elecrow CrowPanel Advance 5-inch ESP32-P4 **V1.0**, 800 × 480, ST7262 / GT911 |
| Display chip | ESP32-P4 revision **v1.3**, 16 MB flash; Arduino chip variant **Before v3.00** |
| Transmitters | Up to 3 × ESP32-S3 development boards, one transmitter per radar |
| Radar | Ai-Thinker **RD-03D**, UART at 256000 baud |

The display configuration targets this Elecrow model. Other P4 displays may
require different pins and drivers.

### Wiring each transmitter

| RD-03D | ESP32-S3 |
| --- | --- |
| VCC | 5V |
| GND | GND |
| TX | **GPIO4** – ESP32 UART RX |
| RX | **GPIO5** – ESP32 UART TX |

Cross-connect TX and RX. The sketch receives radar frames and does not send
configuration commands to the radar. Check the 5 V supply on the development
board: the way its 5 V pin is supplied depends on the board. If an IN–OUT solder
bridge is present, use it according to that board's schematic.

## Arduino IDE and required libraries

Use Arduino IDE 2 and install **esp32 by Espressif Systems, version 3.3.7**
in Boards Manager. This is the version used to verify the sketches. If needed,
add the following URL under Additional Boards Manager URLs in Preferences:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

### Library overview

| Library | Version | Required for | Installation |
| --- | --- | --- | --- |
| LVGL (`lvgl`) | **8.4.0** | Display user interface | `libraries/lvgl.zip` |
| ESP32_Display_Panel | **1.0.4** | Display and touch drivers | `libraries/ESP32_Display_Panel.zip` |
| ESP32_IO_Expander | **1.1.1** | Display driver dependency | `libraries/ESP32_IO_Expander.zip` |
| esp-lib-utils | **0.2.1** | Display driver utilities | `libraries/esp-lib-utils.zip` |
| WiFi, WiFiUdp, Preferences, ESP32/ESP-IDF drivers | Included in ESP32 core 3.3.7 | Networking, storage and hardware | Installed with the board package |

**The transmitter requires no additional library installations.** `WiFi.h` and
`WiFiUdp.h` are included in the ESP32 board package.

For the display, install all four supplied ZIPs using **Sketch → Include Library →
Add .ZIP Library**, then restart Arduino IDE. The ZIPs include the matching
Elecrow/LVGL configuration. No additional compiler flags or absolute file paths
are required.

**LVGL 9 is incompatible with this sketch.** If a different version is already
installed, back it up and replace it with the supplied version. Use the included
ZIPs to get the matching configuration as well as the library versions.

### Fixing errors caused by the wrong LVGL version

Errors such as `lv_draw_ctx_t was not declared`, `lv_disp_drv_t does not name a type`
or `lv_event_get_draw_ctx was not declared` occur when Arduino IDE uses LVGL 9
instead of the required 8.4.0. The reported build failure used LVGL 9.6.0.

1. Back up the existing LVGL installation and remove it from the active Arduino
   sketchbook library folder. Keep the backup outside the sketchbook's `libraries`
   folder so Arduino does not continue detecting it as a library.
2. Use **Sketch → Include Library → Add .ZIP Library** to install the supplied
   **`libraries/lvgl.zip`**. It contains **LVGL 8.4.0 and the project configuration**.
3. Install the other three supplied library ZIPs at the versions listed above.
4. **Restart Arduino IDE** and compile the display sketch again.

A generic LVGL 8.4.0 installation does not automatically include this project's
configuration. Use the supplied ZIP for this project.

## Uploading the display firmware

Open `NW_Radar_Display/NW_Radar_Display.ino` in Arduino IDE. Select or install
**esp32 by Espressif Systems 3.3.7** in Boards Manager. Set the following under
**Tools**:

| Setting | Value |
| --- | --- |
| Board | **ESP32P4 Dev Module** |
| Chip Variant | **Before v3.00** – for the P4 v1.3 used here |
| PSRAM | Enabled |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| Upload Speed | 115200 |
| Erase All Flash Before Sketch Upload | Disabled |
| Port | The display's USB/COM port; `/dev/cu.wchusbserial1230` on the Mac used for this setup |

1. Select the actual USB/COM port of the connected display. The Mac port above
   is an example from this setup; Windows uses a COM port instead.
2. Close any open Serial Monitor before uploading.
3. Click **✓ Verify** and wait for compilation to finish successfully.
4. Click **→ Upload** and wait for the upload to finish.
5. Open Serial Monitor at **115200 baud** if you need diagnostic output.

Use the correct USB data interface on the display. On an already configured
device, keep the existing partition scheme to preserve stored settings.

## Uploading the transmitter firmware

Open `NW_Radar_Transmitter/NW_Radar_Transmitter.ino` in Arduino IDE.
Use **esp32 by Espressif Systems 3.3.7** in Boards Manager.
**The transmitter requires no additional libraries**: `WiFi.h` and `WiFiUdp.h`
are included in the ESP32 board package.

### Set the node ID before each upload

Set the appropriate ID in the sketch:

```cpp
#define NODE_ID 1
```

Use `1` for node 1, `2` for node 2 and `3` for node 3. Each ID must be unique.
The supplied sketch defaults to **1**. Save each change and connect the intended
transmitter before uploading.

### Settings under Tools

| Setting | Value |
| --- | --- |
| Board | **ESP32S3 Dev Module** |
| CPU Frequency | **240 MHz** |
| Flash Mode | **QIO** |
| Flash Frequency | **80 MHz** |
| Flash Size | **4MB (32Mb)** – verified baseline configuration |
| Partition Scheme | **Default 4MB with spiffs** |
| PSRAM | **Disabled** |
| USB Mode | **Hardware CDC and JTAG** |
| USB CDC On Boot | **Enabled** when using the S3's native USB connection |
| Upload Speed | **115200** |
| Erase All Flash Before Sketch Upload | **Disabled** |
| Port | The USB/COM port of the selected transmitter |

If you use a separate USB-to-UART connection instead of the S3's native USB,
select **USB CDC On Boot: Disabled** for serial output through that connection.

### Verify and upload

1. Select the USB/COM port of the connected transmitter.
2. Close any open Serial Monitor before uploading.
3. Click **✓ Verify** and wait for compilation to finish successfully.
4. Click **→ Upload** and wait for the upload to finish.
5. Open Serial Monitor at **115200 baud** if needed.
6. For the next node, change its ID to **2** or **3**, save and upload to that board.

## Connection and signal strength display

The display creates the **NW-Radar** Wi-Fi hotspot. The transmitters connect
automatically. No router or internet connection is required.

| Parameter | Default in the source code |
| --- | --- |
| SSID | `NW-Radar` |
| Wi-Fi password | `NWradar2026` |
| Display IP | `192.168.4.1` |
| UDP port | `5006` |
| Target packets | 10 Hz per node |
| RSSI telemetry | Approximately 1 Hz per node |

These defaults must match across devices. To change the SSID or password,
update both the transmitter (`WIFI_SSID` / `WIFI_PASS`) and the display
(`radar_live.cpp`, `WiFi.softAP`), then upload the firmware to both sides.

Select the required number of active nodes in the display settings and use
LIVE mode for real measurements. The header shows values such as
`N1 -52 dBm  N2 -67 dBm  N3 -84 dBm`.

Each value is the **signal strength of the display hotspot measured at that
transmitter**. A value closer to 0 indicates a stronger signal. `-- dBm` means
unknown or stale; `OFFLINE` means target/heartbeat packets are missing. DEMO
does not generate RSSI readings. Update both the display and all transmitters
to this firmware version to enable the signal strength display.

<img width="800" height="480" alt="rssi-preview" src="https://github.com/user-attachments/assets/1de7c54b-b607-4056-9e65-2a9deaf84c80" />


*This preview comes from the UI test; the RSSI readings shown are simulated.*

## Room plan and targets

All rooms use the same physical scale. Initial dimensions are **5 m horizontally
× 3 m vertically** for rooms 1 and 2, and **1.8 m horizontally × 6 m vertically**
for room 3. The editor lets you move rooms and resize them using handles.
Radar position and orientation are also configured on the display.

When a target is detected, the room border and an exclamation mark flash red.
The target marker has a red glow and a bright center. Room outlines define the
display area, without scan lines or additional inner radar rectangles.
Audio events from multiple nodes are combined into one shared tone.

See [Usage and coordinates](docs/USAGE.md),
[UDP protocol and diagnostics](docs/PROTOCOL.md), and [Validation](docs/VALIDATION.md).

## License and source

The existing [MIT license](LICENSE) was retained from the display source package.
Libraries and supplied manufacturer files retain their own notices; see
[Third-party notices](THIRD_PARTY_NOTICES.md).

The firmware is based on the supplied local radar source packages. The
[existing GitHub project](https://github.com/Stevee87/Tactical-Radar-System-ESP32-P4-elcrow-Display)
was used as a reference for the display hardware.

Official help: [Install the ESP32 board package](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html),
[Install ZIP libraries](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).
