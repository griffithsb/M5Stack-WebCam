#include "Config.h"

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config() :  ssid(""),
                    password(""),
                    record(true),
                    record_icon(true),
                    show_camera(false),
                    face_timeout_s(10)
                    {}

template<typename T>
bool Config::readValue(const JsonDocument& doc, const char* key, T& out) {
  JsonVariantConst value = doc[key];
  if (!value.is<T>())
    return false;

  out = value.as<T>();
  return true;
}

bool Config::loadConfig() {
    FILE* file = std::fopen(CONFIG_FILE_NAME, "rb");
    if (!file) return false;

    char json[MAX_CONFIG_FILE_SIZE];
    size_t n = std::fread(json, 1, MAX_CONFIG_FILE_SIZE - 1, file);
    std::fclose(file);

    if (n == 0) return false;
    json[n] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return false;

    readValue(doc ,"ssid", ssid);
    readValue(doc ,"password", password);
    readValue(doc ,"record", record);
    readValue(doc ,"record_icon", record_icon);
    readValue(doc ,"show_camera", show_camera);
    readValue(doc ,"face_timeout_s", face_timeout_s);
    return true;
}
