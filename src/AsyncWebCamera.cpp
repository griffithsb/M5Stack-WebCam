#include "AsyncWebCamera.h"

#define PART_BOUNDARY "123456789000000000000987654321"

static const char* STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART =
    "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char * JPG_CONTENT_TYPE = "image/jpeg";


AsyncJpegStreamResponse::AsyncJpegStreamResponse() {
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

bool AsyncJpegStreamResponse::_sourceValid() const {
    return true;
}

size_t AsyncJpegStreamResponse::_fillBuffer(uint8_t *buf, size_t maxLen) {
    size_t ret = _content(buf, maxLen, _index);
    if (ret != RESPONSE_TRY_AGAIN) {
        _index += ret;
    }
    return ret;
}

size_t AsyncJpegStreamResponse::_content(uint8_t *buffer, size_t maxLen, size_t index) {
    Camera& camera = Camera::getInstance();

    if (first || _frame_index == _jpg_len_copy) {
        first = false;

        if (index) {
            uint64_t end = (uint64_t)micros();
            lastAsyncRequest = end;
        }

        if (maxLen < (strlen(STREAM_BOUNDARY) +
                      strlen(STREAM_PART) +
                      strlen(JPG_CONTENT_TYPE) + 8)) {
            return RESPONSE_TRY_AGAIN;
        }

        _frame_index = 0;
        camera.getJpegFrameCopy(_jpg_buf_copy, &_jpg_len_copy);

        // Send boundary
        size_t blen = 0;
        if (index) {
            blen = strlen(STREAM_BOUNDARY);
            memcpy(buffer, STREAM_BOUNDARY, blen);
            buffer += blen;
        }

        // Send header
        size_t hlen = sprintf((char *)buffer, STREAM_PART,
                              JPG_CONTENT_TYPE, _jpg_len_copy);
        buffer += hlen;

        // Send frame chunk
        hlen = maxLen - hlen - blen;
        if (hlen > _jpg_len_copy) {
            maxLen -= hlen - _jpg_len_copy;
            hlen = _jpg_len_copy;
        }

        memcpy(buffer, _jpg_buf_copy, hlen);
        _frame_index += hlen;

        return maxLen;
    }

    // Continue sending frame
    size_t available = _jpg_len_copy - _frame_index;
    if (maxLen > available) {
        maxLen = available;
    }

    memcpy(buffer, _jpg_buf_copy + _frame_index, maxLen);
    _frame_index += maxLen;

    return maxLen;
}