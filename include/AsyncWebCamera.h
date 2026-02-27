#pragma once

#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "Camera.h"

const size_t PIXFORMAT_RGB565_BUFF_SIZE = (320 * 240 * 2);

class AsyncJpegStreamResponse : public AsyncAbstractResponse {
private:
    size_t _frame_index;
    size_t _jpg_len_copy;
    uint8_t _jpg_buf_copy[PIXFORMAT_RGB565_BUFF_SIZE];
    size_t _index;
    uint64_t lastAsyncRequest;
    bool first;

    size_t _content(uint8_t *buffer, size_t maxLen, size_t index);

public:
    AsyncJpegStreamResponse();

    bool _sourceValid() const override;
    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};
