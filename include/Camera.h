#pragma once
#include <M5Unified.h>
#include "esp_camera.h"
#include "Config.h"
#include "model_zoo/human_face_detect_msr01.hpp"

class Camera
{
public:
    static Camera& getInstance();

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    int m_faceCnt = 0;
    void setup();
    void faceDetect(camera_fb_t *fb);
    void updateDisplay();
    void updateCameraJpg();
    void getJpegFrameCopy(uint8_t *jpg_buf_copy, size_t *jpg_len_copy);
    void update();
    size_t m_jpgBufLen;
    uint8_t *m_jpgBuf = NULL;
    M5Canvas m_canvas;

private:
    Camera();
    HumanFaceDetectMSR01 face_detector;
    bool getJpegFrame(uint8_t **jpg_buf, size_t *jpg_len);
    SemaphoreHandle_t m_mutex;
    M5GFX m_display;
    Config& m_config;
    camera_fb_t* m_fb;
};
