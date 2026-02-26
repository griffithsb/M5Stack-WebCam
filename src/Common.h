#ifndef __WEBCAM_H__
#define __WEBCAM_H__
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_timer.h>

#include "ESPAsyncWebServer.h"

#define SD_SPI_CS_PIN    4
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37

#define NTP_TIMEZONE "UTC+0"
#define NTP_SERVER1  "0.pool.ntp.org"
#define NTP_SERVER2  "1.pool.ntp.org"
#define NTP_SERVER3  "2.pool.ntp.org"
#define SNTP_ENABLED 1


static const char *af_str = "ASSERT FAILED";
static const char *cf_str = "CHECK FAILED";
#define ASSERT(_f, _r) if(_f == _r) {M5_LOGE("%s %s != %s", af_str, #_f, #_r); delay(50000); abort(); }
#define CHECK(_f, _r) if(_f != _r) {M5_LOGE("%s %s == %s", cf_str, #_f, #_r);  delay(50000); abort(); }

#define HELP_STRING "Config not found\nAdd config.json to sd card\n\n" \
    "{\"ssid\":\"<your-ssid>\",\n" \
    "\"password\":\"<your-pass>\",\n" \
    "\"record\":true,\n" \
    "\"record_icon\":true,\n" \
    "\"show_camera\":false}\n" \
    "\"face_timeout_s\":10}\""

extern void streamJpg(AsyncWebServerRequest *request);
extern void sendIndex(AsyncWebServerRequest *request);

#endif