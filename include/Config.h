#pragma once

#include <string>
#include <cstdio>
#include <ArduinoJson.h>

#define MAX_CONFIG_FILE_SIZE 256
#define CONFIG_FILE_NAME "/sd/config.json"

class Config {
public:
    static Config& getInstance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    bool loadConfig();

    std::string ssid;
    std::string password;

    bool record;
    bool record_icon;
    bool show_camera;
    int face_timeout_s;

private:
    Config();
};