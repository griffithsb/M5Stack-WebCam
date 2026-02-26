#include "Camera.h"
#include "model_zoo/human_face_detect_msr01.hpp"

Camera& Camera::getInstance() {
    static Camera instance;
    return instance;
}

Camera::Camera(): display(), canvas(&display), _config(Config::getInstance()) {}

void Camera::setup()
{
    camera_config_t _camera_config = {
        .pin_pwdn     = -1,
        .pin_reset    = -1,
        .pin_xclk     = -1,
        .pin_sscb_sda = 12,
        .pin_sscb_scl = 11,
        .pin_d7       = 47,
        .pin_d6       = 48,
        .pin_d5       = 16,
        .pin_d4       = 15,
        .pin_d3       = 42,
        .pin_d2       = 41,
        .pin_d1       = 40,
        .pin_d0       = 39,

        .pin_vsync = 46,
        .pin_href  = 38,
        .pin_pclk  = 45,

        .xclk_freq_hz = 20000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format  = PIXFORMAT_RGB565,
        .frame_size    = FRAMESIZE_QVGA,
        .jpeg_quality  = 0,
        .fb_count      = 2,
        .fb_location   = CAMERA_FB_IN_PSRAM,
        .grab_mode     = CAMERA_GRAB_WHEN_EMPTY,
        .sccb_i2c_port = -1,
    };

    esp_camera_init(&_camera_config);
    display.begin();
    canvas.createSprite(320, 240); //FRAMESIZE_QVGA // 320 x 240.
    canvas.setColor(TFT_GREEN);
    mutex = xSemaphoreCreateMutex();
}

void Camera::face_detect(camera_fb_t * fb)
{
        int x, y, w, h;

        HumanFaceDetectMSR01 s1(0.3F, 0.5F, 1, 0.5F);
        std::list<dl::detect::result_t> &results = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});

        face_cnt = results.size();


        for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++)
        {
            x = (int)prediction->box[0];
            y = (int)prediction->box[1];
            w = (int)prediction->box[2] - x + 1;
            h = (int)prediction->box[3] - y + 1;
            canvas.drawRect(x, y, w, h);
        }
}

bool Camera::get_jpeg_frame(uint8_t ** jpg_buf, size_t * jpg_len) {
    return fmt2jpg(
        (uint8_t*)canvas.getBuffer(),
        canvas.width() * canvas.height() * 2,  // buffer length (RGB565 = 2 bytes/pixel)
        canvas.width(),
        canvas.height(),
        PIXFORMAT_RGB565,
        80, // JPEG quality (0–100)
        jpg_buf,
        jpg_len
    );
}

void Camera::get_jpeg_frame_copy(uint8_t *jpg_buf_copy, size_t * jpg_len_copy)
{
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
        memcpy(jpg_buf_copy, jpg_buf, jpg_buf_len);
        *jpg_len_copy = jpg_buf_len;
        xSemaphoreGive(mutex);
    }
}

void Camera::update()
{
    camera_fb_t* fb = esp_camera_fb_get();

    canvas.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->buf);

    face_detect(fb);
    if(_config.show_camera) canvas.pushSprite(&M5.Display, 0, 0);

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
        if(jpg_buf != NULL) {
            free(jpg_buf);
            jpg_buf = NULL;
        }
        get_jpeg_frame(&jpg_buf, &jpg_buf_len);
        xSemaphoreGive(mutex);
    }

    esp_camera_fb_return(fb);
}
