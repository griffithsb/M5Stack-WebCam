#pragma once

#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "Camera.h"

const size_t PIXFORMAT_RGB565_BUFF_SIZE = (320 * 240 * 2);

class AsyncJpegStreamResponse : public AsyncAbstractResponse {
private:
    size_t m_frameIndex;
    size_t m_jpgLenCopy;
    uint8_t m_jpgBufCopy[PIXFORMAT_RGB565_BUFF_SIZE];
    size_t m_index;
    uint64_t m_lastAsyncRequest;
    bool m_first;

    size_t _content(uint8_t *buffer, size_t maxLen, size_t index);

public:
    AsyncJpegStreamResponse();

    bool _sourceValid() const override;
    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};
