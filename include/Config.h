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

    std::string m_ssid;
    std::string m_password;

    bool m_record;
    bool m_recordIcon;
    bool m_showCamera;
    int m_faceTimeout;

private:
    Config();
    template<typename T>
    bool readValue(const JsonDocument& doc, const char* key, T& out);
};