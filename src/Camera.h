#pragma once
#include <M5Unified.h>
#include "esp_camera.h"
#include "Config.h"

class Camera
{
public:
    static Camera& getInstance();

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    int face_cnt = 0;
    void setup();
    void face_detect(camera_fb_t *fb);

    void get_jpeg_frame_copy(uint8_t *jpg_buf_copy, size_t *jpg_len_copy);
    void update();
    size_t jpg_buf_len;
    uint8_t *jpg_buf = NULL;

private:
    Camera();
    bool get_jpeg_frame(uint8_t **jpg_buf, size_t *jpg_len);
    SemaphoreHandle_t mutex;
    M5GFX display;
    M5Canvas canvas;
    Config& _config;
};
