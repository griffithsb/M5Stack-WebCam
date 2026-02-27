#pragma once

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EspFileManager.h>
#include <WiFi.h>
#include <sntp.h>

#include "Camera.h"
#include "AviWriter.h"
#include "Config.h"
#include "AsyncWebCamera.h"

const size_t FILENAME_MAX_LEN = 80;

class Webcam
{
public:
    void begin();
    void update();

private:
    void setupWiFi();
    void setupServer();
    void updateRecording();
    void updateRecIndicator();

    static void handleStream(AsyncWebServerRequest *request);
    static void handleIndex(AsyncWebServerRequest *request);

private:
    AsyncWebServer m_server{80};
    EspFileManager m_fileManager;
    AviWriter m_aviWriter;

    char filename[FILENAME_MAX_LEN];

    Config& m_config = Config::getInstance();
    Camera& m_camera = Camera::getInstance();

    bool m_recording = false;
    uint32_t m_noFacesStart = 0;
    uint32_t m_facePresentStart = 0;
};