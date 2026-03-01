#include "WebCam.h"
#include <M5Unified.h>

#define SD_SPI_CS_PIN    4
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37

#define NTP_TIMEZONE "UTC+0"
#define NTP_SERVER1  "0.pool.ntp.org"
#define NTP_SERVER2  "1.pool.ntp.org"
#define NTP_SERVER3  "2.pool.ntp.org"

#define HELP_STRING "Config not found\nAdd config.json to sd card\n\n" \
    "{\"ssid\":\"<your-ssid>\",\n" \
    "\"password\":\"<your-pass>\",\n" \
    "\"record\":true,\n" \
    "\"record_icon\":true,\n" \
    "\"show_camera\":false,\n" \
    "\"face_timeout_s\":10}\""

void Webcam::begin()
{
    M5.begin();
    M5.In_I2C.release();

    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
    M5.Lcd.setTextSize(2);

    if(!m_config.loadConfig()) {
        M5.Display.printf(HELP_STRING);
        while(1) delay(1);
    }

    m_camera.setup();

    if(m_config.m_ssid != "") {
        setupWiFi();
        setupServer();
    }

    M5.Display.clear(TFT_BLACK);
    if(!m_config.m_showCamera) M5.Lcd.setBrightness(0);
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
}

void Webcam::setupWiFi()
{
    int retry = 32;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(m_config.m_ssid.c_str(), m_config.m_password.c_str());
    WiFi.setSleep(false);

    M5.Display.printf("\nWiFi:");

    while(retry-- > 0 && WiFi.status() != WL_CONNECTED) {
        M5.Display.printf(".");
        delay(200);
    }

    if(WiFi.status() != WL_CONNECTED) {
        M5.Display.printf("\nFailed to connect WiFi\n");
        abort();
    }

    M5.Display.printf("\nhttp://%s\n", WiFi.localIP().toString().c_str());

    configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    M5.Display.print("NTP: ");

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        M5.Display.print(".");
        delay(500);
    }

    time_t t = time(nullptr);
    M5.Rtc.setDateTime(gmtime(&t));
}

void Webcam::setupServer()
{
    m_fileManager.initSDCard(&SD, SD_SPI_CS_PIN);
    m_fileManager.setServer(&m_server);

    m_server.on("/", HTTP_GET, handleIndex);
    m_server.on("/stream", HTTP_GET, handleStream);

    m_server.begin();
}

void Webcam::handleStream(AsyncWebServerRequest *request)
{
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();

    if(!response){
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void Webcam::handleIndex(AsyncWebServerRequest *request)
{
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<style>"
        "html,body{margin:0;padding:0;height:100%;background:black;}"
        "img{position:fixed;top:0;left:0;width:100vw;height:100vh;object-fit:cover;}"
        "</style>"
        "</head>"
        "<body>"
        "<img src=\"/stream\">"
        "</body>"
        "</html>";
    request->send(200, "text/html", html);
}

void Webcam::update()
{
    m_camera.update();

    if(m_config.m_record && m_config.m_recordIcon && m_recording) {
        updateRecIndicator();
    }

    m_camera.updateCameraJpg();

    if(m_config.m_record) {
        updateRecordingStatus();
    }

    if(m_config.m_showCamera) {
        m_camera.updateDisplay();
    }

    if(m_config.m_record && m_recording) {
        m_aviWriter.addJpegFrame(m_camera.m_jpgBuf, m_camera.m_jpgBufLen);
    }
}

void Webcam::updateRecordingStatus()
{
    const uint32_t START_DELAY_MS = 250;

    // START recording when face present
    if (!m_recording) {
        if (m_camera.m_faceCnt > 0) {
            if (m_facePresentStart == 0)
                m_facePresentStart = millis();
            else if (millis() - m_facePresentStart >= START_DELAY_MS) {

                auto dt = M5.Rtc.getDateTime();

                snprintf(filename, FILENAME_MAX_LEN,
                    "/%02d_%02d_%04d-%02d_%02d_%02d.avi",
                    dt.date.date, dt.date.month, dt.date.year,
                    dt.time.hours, dt.time.minutes, dt.time.seconds);

                m_aviWriter.begin(SD, filename, FRAMESIZE_QVGA, 15);

                m_recording = true;
                m_noFacesStart = 0;
                m_facePresentStart = 0;

                M5.Lcd.setBrightness(m_config.m_showCamera ? 100 : 50);
            }
        } else {
            m_facePresentStart = 0;
        }
    }

    // STOP recording when no faces
    if (m_recording) {
        if (m_camera.m_faceCnt == 0) {
            if (m_noFacesStart == 0)
                m_noFacesStart = millis();
            else if (millis() - m_noFacesStart >=
                     m_config.m_faceTimeout * 1000) {

                m_aviWriter.end();
                m_recording = false;
                m_noFacesStart = 0;
                m_facePresentStart = 0;

                if(!m_config.m_showCamera) M5.Lcd.setBrightness(0);
            }
        } else {
            m_noFacesStart = 0;
        }
    }
}

void Webcam::updateRecIndicator()
{
    static uint32_t lastToggle = 0;
    static bool visible = true;

    if (millis() - lastToggle >= 1000) {
        lastToggle = millis();
        visible = !visible;
    }

    if (visible) {
        M5.Display.setCursor(0,0);
        M5.Display.print("R");
    } else {
        if(!m_config.m_showCamera) {
            M5.Display.setCursor(0,0);
            M5.Display.print(" ");
        }
    }
}