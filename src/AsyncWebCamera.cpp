#include <M5Unified.h>
#include "esp_camera.h"
#include "ESPAsyncWebServer.h"
#include "model_zoo/human_face_detect_msr01.hpp"
#include "Camera.h"
// https://gist.github.com/me-no-dev/d34fba51a8f059ac559bf62002e61aa3

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char * JPG_CONTENT_TYPE = "image/jpeg";
static const char * BMP_CONTENT_TYPE = "image/x-windows-bmp";

class AsyncJpegStreamResponse: public AsyncAbstractResponse {
    private:

        size_t _frame_index;
        size_t _jpg_len_copy;
        uint8_t _jpg_buf_copy[320*240*2];
        size_t _index;
        uint64_t lastAsyncRequest;
        bool first;
    public:
        AsyncJpegStreamResponse(){
            _callback = nullptr;
            _code = 200;
            _contentLength = 0;
            _contentType = STREAM_CONTENT_TYPE;
            _sendContentLength = false;
            _chunked = true;
            _index = 0;
            lastAsyncRequest = 0;
            first = true;
            _frame_index = 0;
        }

        bool _sourceValid() const {
            return true;
        }

        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }

        size_t _content(uint8_t *buffer, size_t maxLen, size_t index) {
            Camera& camera = Camera::getInstance();
            if(first || _frame_index == _jpg_len_copy){
                first = false;

                if(index){
                    uint64_t end = (uint64_t)micros();
                    int fp = (end - lastAsyncRequest) / 1000;
                    //M5_LOGI("Size: %uKB, Time: %ums (%.1ffps)\n", _jpg_len_copy/1024, fp);
                    lastAsyncRequest = end;
                }

                if(maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)){
                    return RESPONSE_TRY_AGAIN;
                }

                _frame_index = 0;

                camera.get_jpeg_frame_copy(_jpg_buf_copy, &_jpg_len_copy);

                //send boundary
                size_t blen = 0;
                if(index){
                    blen = strlen(STREAM_BOUNDARY);
                    memcpy(buffer, STREAM_BOUNDARY, blen);
                    buffer += blen;
                }
                //send header
                size_t hlen = sprintf((char *)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_len_copy);
                buffer += hlen;

                //send frame
                hlen = maxLen - hlen - blen;
                if(hlen > _jpg_len_copy){
                    maxLen -= hlen - _jpg_len_copy;
                    hlen = _jpg_len_copy;
                }
                memcpy(buffer, _jpg_buf_copy, hlen);

                _frame_index += hlen;

                return maxLen;
            }

            //send frame
            size_t available = _jpg_len_copy - _frame_index;
            if(maxLen > available){
                maxLen = available;
            }
            memcpy(buffer, _jpg_buf_copy+_frame_index, maxLen);
            _frame_index += maxLen;

            return maxLen;
        }

};

void streamJpg(AsyncWebServerRequest *request){
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
    if(!response){
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void sendIndex(AsyncWebServerRequest *request){
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<style>"
        "html,body{margin:0;padding:0;height:100%;background:black;}"
        "img{position:fixed;top:0;left:0;width:100vw;height:100vh;object-fit:cover;}"
        "</style>"
        "</head>"
        "<body>"
        "<img src=\"/stream\">"
        "</body>"
        "</html>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
}

