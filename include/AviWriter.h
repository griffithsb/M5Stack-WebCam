#pragma once
#include <Arduino.h>
#include <FS.h>

// Simple MJPEG-in-AVI writer (video only, no audio).
// Writes:
//  - AVI header (310 bytes) placeholder
//  - for each frame: '00dc' chunk header + jpeg data + padding
//  - idx1 chunk at end
//
// frameType must match the ESP32 camera framesize_t enum ordering
// if you want correct width/height auto-patching.
// Adapted from https://github.com/s60sc/ESP32-CAM_MJPEG2SD

class AviWriter {
public:
  static constexpr size_t AVI_HEADER_LEN = 310;

  // maxFrames controls index RAM allocation: (maxFrames + 2) * 16 bytes.
  explicit AviWriter(uint32_t maxFrames = 20000);

  // Open AVI file and write placeholder header.
  // frameType: framesize_t enum index (sensor.h) to patch width/height.
  // targetFps: used if duration is 0; otherwise actual fps is computed.
  bool begin(fs::FS& fs, const char* path, uint8_t frameType, uint8_t targetFps);

  // Add a JPEG frame. jpg must be complete JPEG bitstream.
  bool addJpegFrame(const uint8_t* jpg, size_t jpgLen);

  // Finalize index, patch header, close file.
  bool end();

  // Abort/close without finalizing correctly (optional utility).
  void abort();

  // Stats
  uint16_t frameCount() const { return _frameCnt; }

private:

  static constexpr size_t CHUNK_HDR = 8;
  static constexpr size_t IDX_ENTRY = 16;

  struct FrameSize {
    uint8_t w[2];
    uint8_t h[2];
  };

  void prepAviIndex();
  void buildAviIdx(uint32_t dataSize); // dataSize is chunk payload size (jpeg+pad)
  void finalizeAviIndex();
  size_t writeAviIndex(uint8_t* out, size_t outSize);
  void buildAviHdr(uint8_t fps, uint8_t frameType, uint16_t frameCnt);

  bool ensureIndexAlloc();

  fs::FS* _fs = nullptr;
  File _file;

  uint8_t _frameType = 0;
  uint8_t _targetFps = 0;
  uint16_t _frameCnt = 0;
  uint32_t _startMs = 0;

  // Index buffer / counters
  uint32_t _maxFrames = 0;
  uint8_t* _idxBuf = nullptr;
  size_t _idxPtr = 0;
  size_t _idxOffset = 0;
  size_t _moviSize = 0;
  size_t _indexLen = 0;

  // AVI header template (patched at end)
  uint8_t _aviHeader[AVI_HEADER_LEN];

  // Constants
  static const uint8_t DC_BUF[4];   // "00dc"
  static const uint8_t IDX1_BUF[4]; // "idx1"
  static const uint8_t ZERO_BUF[4]; // 0

  static const FrameSize FRAME_SIZES[];
  static const size_t FRAME_SIZES_COUNT;
};