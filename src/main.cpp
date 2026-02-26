/**
 * @file main.cpp
 * @author Brian Griffiths
 * @brief https://github.com
 * @version 1.0
 * @date 2026-02-25
 *
 * @Hardwares: M5Stack CoreS3
 * @Platform: PlatformIO (ESP32)
 * @Dependent Library:
 * M5Unified: https://github.com/m5stack/M5Unified
 * ESP File Manager: https://github.com/arslan437/EspFileManager
 * AsyncTCP: https://github.com/ESP32Async/AsyncTCP
 * ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer
 * ArduinoJson: https://github.com/bblanchon/ArduinoJson
 * DNSServer: https://github.com/espressif/arduino-esp32/tree/master/libraries/DNSServer
 * SHA-1 Hash: https://github.com/Cathedrow/Cryptosuite
 **/

/*
MIT License

Copyright (c) 2025 Yoshiharu Hirose

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EspFileManager.h>
#include <sntp.h>

#include "Common.h"
#include "Camera.h"
#include "AviHandler.h"
#include "Config.h"

static AsyncWebServer server(80);
static EspFileManager FileManager;
static SemaphoreHandle_t stateMutex;
static AviWriter avi;
static char filename[80];
static Config& config = Config::getInstance();
static Camera& camera = Camera::getInstance();

void setup()
{
    int retry = 32;

    M5.begin();
    M5.In_I2C.release();

    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
    M5.Lcd.setTextSize(2);
    if(!config.load_config()) {
        M5.Display.printf(HELP_STRING);
        while(1) delay(1);
    }

    camera.setup();

    if(config.ssid != "") {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        WiFi.begin(config.ssid.c_str(), config.password.c_str());
        WiFi.setSleep(false);
        delay(100);
        FileManager.initSDCard(&SD, SD_SPI_CS_PIN);
        M5.Display.printf("\nWiFi:");

        while(retry-- > 0 && WiFi.status() != WL_CONNECTED)
        {
            M5.Display.printf(".");
            delay(200);
        }

        if(WiFi.status() != WL_CONNECTED)
        {
            M5.Display.printf("\nFailed to connect WiFi\n");
            delay(1000);
            abort();
        }

        M5.Display.printf("\nhttp://%s\n", WiFi.localIP().toString().c_str());

        server.on("/", HTTP_GET, sendIndex);
        server.on("/stream", HTTP_GET, streamJpg);

        FileManager.setServer(&server);

        if (!M5.Rtc.isEnabled()) {
            M5_LOGE("RTC not found");
            while (true) delay(500);
        }

        configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
        M5.Display.print("NTP: ");
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
            M5.Display.print(".");
            delay(500);
        }
        M5.Display.println("\nNTP connected");
        time_t t = time(nullptr);
        M5.Rtc.setDateTime(gmtime(&t));

        server.begin();
    }

    M5.Display.clear(TFT_BLACK);
    if(!config.show_camera) M5.Lcd.setBrightness(0);
    M5.Display.setTextColor(TFT_RED);
}

static inline void updateRecIndicator(bool recording) {
  static uint32_t lastToggle = 0;
  static bool visible = true;

  if (!recording) {
    if(!config.show_camera) M5.Display.clear(TFT_BLACK);
    visible = true;
    return;
  }

  if ((uint32_t)(millis() - lastToggle) >= 500) {
    lastToggle = millis();
    visible = !visible;

    if(!config.show_camera) M5.Display.clear(TFT_BLACK);
    if (visible) {
        M5.Display.setCursor(0,0);
        M5.Display.print("R");
    }
  }
}

void loop()
{
    static uint64_t start_us = micros();
    static int cnt = 0;

    camera.update();

    uint64_t end_us = micros();

    if (cnt++ % 5 == 0) {
        M5_LOGI("[%lu]  %.2ffps", (unsigned long)cnt, 1000000.0 / (end_us - start_us));
    }
    start_us = end_us;

    if(config.record) {
        // persistent state
        static bool recording = false;
        static uint32_t no_faces_start = 0;     // while recording: when face_cnt became 0
        static uint32_t face_present_start = 0; // while NOT recording: when face_cnt became >0
        const uint32_t START_DELAY_MS = 250;

        // Start logic: require face_cnt > 0 continuously for 250ms
        if (!recording) {
            if (camera.face_cnt > 0) {
                if (face_present_start == 0) {
                    face_present_start = millis(); // start counting continuous presence
                } else if ((uint32_t)(millis() - face_present_start) >= START_DELAY_MS) {
                    auto dt = M5.Rtc.getDateTime();
                    snprintf(filename, 80, "/%02d_%02d_%04d-%02d_%02d_%02d.avi", dt.date.date, dt.date.month,
                                            dt.date.year, dt.time.hours, dt.time.minutes, dt.time.seconds);
                    ASSERT(avi.begin(SD, filename, FRAMESIZE_QVGA, 15), false)
                    recording = true;
                    no_faces_start = 0;         // reset "no faces" timer
                    face_present_start = 0;     // reset start timer
                    M5.Lcd.setBrightness(config.show_camera ? 100 : 50);
                }
            } else {
                // face not present -> reset the continuous presence timer
                face_present_start = 0;
            }
        }

        // Recording logic: stop after 5s of continuous no faces
        if (recording) {
            if(config.record_icon) updateRecIndicator(recording);
            if (camera.face_cnt == 0) {
                if (no_faces_start == 0) {
                    no_faces_start = millis();
                } else if ((uint32_t)(millis() - no_faces_start) >= (config.face_timeout_s* 1000)) {
                    avi.end();
                    recording = false;
                    no_faces_start = 0;
                    face_present_start = 0; // ensure start delay is re-applied next time
                    if(!config.show_camera) M5.Lcd.setBrightness(0);
                    return; // optional: avoid adding a frame after stopping
                }
            } else {
                // face is present again -> reset stop timer
                no_faces_start = 0;
            }

            // Only add frames while still recording
            ASSERT(avi.addJpegFrame(camera.jpg_buf, camera.jpg_buf_len), false);
        }
    }
}
