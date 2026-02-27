#include "Config.h"

#define LOAD_JSON_KEY(_var) do {                                                            \
                                JsonVariantConst j##_var = doc[#_var];                      \
                                if (j##_var.isNull() || !j##_var.is<const char*>()) break;  \
                                const char* s##_var = j##_var.as<const char*>();            \
                                if (!s##_var) break;                                        \
                                _var.assign(s##_var);                                       \
                            } while (0)

#define LOAD_JSON_KEY_BOOL(_var) do {                                                       \
                                    JsonVariantConst j##_var = doc[#_var];                  \
                                    if (j##_var.isNull() || !j##_var.is<bool>()) break;     \
                                    _var = j##_var.as<bool>();                              \
                                } while (0)

#define LOAD_JSON_KEY_INT(_var) do {                                \
                                    JsonVariantConst j##_var = doc[#_var]; \
                                    if (j##_var.isNull() || !j##_var.is<int>()) break; \
                                    _var = j##_var.as<int>();        \
                                } while (0)

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

    LOAD_JSON_KEY(ssid);
    LOAD_JSON_KEY(password);
    LOAD_JSON_KEY_BOOL(record);
    LOAD_JSON_KEY_BOOL(record_icon);
    LOAD_JSON_KEY_BOOL(show_camera);
    LOAD_JSON_KEY_INT(face_timeout_s);
    return true;
}
