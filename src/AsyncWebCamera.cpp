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
    m_index = 0;
    m_lastAsyncRequest = 0;
    m_first = true;
    m_frameIndex = 0;
}

bool AsyncJpegStreamResponse::sourceValid() const {
    return true;
}

size_t AsyncJpegStreamResponse::fillBuffer(uint8_t *buf, size_t maxLen) {
    size_t ret = _content(buf, maxLen, m_index);
    if (ret != RESPONSE_TRY_AGAIN) {
        m_index += ret;
    }
    return ret;
}

size_t AsyncJpegStreamResponse::_content(uint8_t *buffer, size_t maxLen, size_t index) {
    Camera& camera = Camera::getInstance();

    if (m_first || m_frameIndex == m_jpgLenCopy) {
        m_first = false;

        if (index) {
            uint64_t end = (uint64_t)micros();
            m_lastAsyncRequest = end;
        }

        if (maxLen < (strlen(STREAM_BOUNDARY) +
                      strlen(STREAM_PART) +
                      strlen(JPG_CONTENT_TYPE) + 8)) {
            return RESPONSE_TRY_AGAIN;
        }

        m_frameIndex = 0;
        camera.getJpegFrameCopy(m_jpgBufCopy, &m_jpgLenCopy);

        // Send boundary
        size_t blen = 0;
        if (index) {
            blen = strlen(STREAM_BOUNDARY);
            memcpy(buffer, STREAM_BOUNDARY, blen);
            buffer += blen;
        }

        // Send header
        size_t hlen = sprintf((char *)buffer, STREAM_PART,
                              JPG_CONTENT_TYPE, m_jpgLenCopy);
        buffer += hlen;

        // Send frame chunk
        hlen = maxLen - hlen - blen;
        if (hlen > m_jpgLenCopy) {
            maxLen -= hlen - m_jpgLenCopy;
            hlen = m_jpgLenCopy;
        }

        memcpy(buffer, m_jpgBufCopy, hlen);
        m_frameIndex += hlen;

        return maxLen;
    }

    // Continue sending frame
    size_t available = m_jpgLenCopy - m_frameIndex;
    if (maxLen > available) {
        maxLen = available;
    }

    memcpy(buffer, m_jpgBufCopy + m_frameIndex, maxLen);
    m_frameIndex += maxLen;

    return maxLen;
}