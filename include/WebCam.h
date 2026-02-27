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
    AsyncWebServer server{80};
    EspFileManager fileManager;
    AviWriter aviWriter;

    char filename[FILENAME_MAX_LEN];

    Config& config = Config::getInstance();
    Camera& camera = Camera::getInstance();

    bool recording = false;
    uint32_t no_faces_start = 0;
    uint32_t face_present_start = 0;
};