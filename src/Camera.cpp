#include "Camera.h"

Camera& Camera::getInstance() {
    static Camera instance;
    return instance;
}

Camera::Camera(): m_display(), m_canvas(&m_display), m_config(Config::getInstance()), face_detector(0.2f, 0.3f, 2, 0.3f) {}

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
    m_display.begin();
    m_canvas.createSprite(320, 240); //FRAMESIZE_QVGA // 320 x 240.

    m_mutex = xSemaphoreCreateMutex();

    sensor_t* s = esp_camera_sensor_get();
    s->set_hmirror(s, 0);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
}

void Camera::faceDetect(camera_fb_t * fb)
{
        int x, y, w, h;

        std::list<dl::detect::result_t> &results = face_detector.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});

        m_faceCnt = results.size();

        for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++)
        {
            x = (int)prediction->box[0];
            y = (int)prediction->box[1];
            w = (int)prediction->box[2] - x + 1;
            h = (int)prediction->box[3] - y + 1;
            m_canvas.setColor(TFT_GREEN);
            m_canvas.drawRect(x, y, w, h);
        }
}

bool Camera::getJpegFrame(uint8_t ** jpg_buf, size_t * jpg_len) {
    return fmt2jpg(
        (uint8_t*)m_canvas.getBuffer(),
        m_canvas.width() * m_canvas.height() * 2,  // buffer length (RGB565 = 2 bytes/pixel)
        m_canvas.width(),
        m_canvas.height(),
        PIXFORMAT_RGB565,
        80, // JPEG quality (0–100)
        jpg_buf,
        jpg_len
    );
}

void Camera::getJpegFrameCopy(uint8_t *jpg_buf_copy, size_t * jpg_len_copy)
{
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE)
    {
        memcpy(jpg_buf_copy, m_jpgBuf, m_jpgBufLen);
        *jpg_len_copy = m_jpgBufLen;
        xSemaphoreGive(m_mutex);
    }
}

void Camera::update()
{
    m_fb = esp_camera_fb_get();

    m_canvas.pushImage(0, 0, m_fb->width, m_fb->height, (uint16_t *)m_fb->buf);

    faceDetect(m_fb);
    
    esp_camera_fb_return(m_fb);
}

void Camera::updateCameraJpg()
{
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE)
    {
        if(m_jpgBuf != NULL) {
            free(m_jpgBuf);
        }
        getJpegFrame(&m_jpgBuf, &m_jpgBufLen);
        xSemaphoreGive(m_mutex);
    }
}

void Camera::updateDisplay()
{
    m_canvas.pushSprite(&M5.Display, 0, 0);
}