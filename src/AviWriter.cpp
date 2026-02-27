#include "AviWriter.h"
#include <math.h>
#include <string.h>

const uint8_t AviWriter::DC_BUF[4]   = {0x30, 0x30, 0x64, 0x63}; // "00dc"
const uint8_t AviWriter::IDX1_BUF[4] = {0x69, 0x64, 0x78, 0x31}; // "idx1"
const uint8_t AviWriter::ZERO_BUF[4] = {0x00, 0x00, 0x00, 0x00};

static const uint8_t kAviHeaderTemplate[AviWriter::AVI_HEADER_LEN] = {
  0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0x16, 0x01, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x6C, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x4C, 0x49, 0x53, 0x54, 0x56, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x61, 0x75, 0x64, 0x73,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x11, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x11, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x11, 0x2B, 0x00, 0x00, 0x11, 0x2B, 0x00, 0x00,
  0x02, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x4C, 0x49, 0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};

// Frame sizes copied from your snippet; index must match framesize_t enum order.
const AviWriter::FrameSize AviWriter::FRAME_SIZES[] = {
  {{0x60, 0x00}, {0x60, 0x00}}, // 96X96
  {{0xA0, 0x00}, {0x78, 0x00}}, // qqvga
  {{0x80, 0x00}, {0x80, 0x00}}, // 128X128
  {{0xB0, 0x00}, {0x90, 0x00}}, // qcif
  {{0xF0, 0x00}, {0xB0, 0x00}}, // hqvga
  {{0xF0, 0x00}, {0xF0, 0x00}}, // 240X240
  {{0x40, 0x01}, {0xF0, 0x00}}, // qvga
  {{0x40, 0x01}, {0x40, 0x01}}, // 320X320
  {{0x90, 0x01}, {0x28, 0x01}}, // cif
  {{0xE0, 0x01}, {0x40, 0x01}}, // hvga
  {{0x80, 0x02}, {0xE0, 0x01}}, // vga
  {{0x20, 0x03}, {0x58, 0x02}}, // svga
  {{0x00, 0x04}, {0x00, 0x03}}, // xga
  {{0x00, 0x05}, {0xD0, 0x02}}, // hd
  {{0x00, 0x05}, {0x00, 0x04}}, // sxga
  {{0x40, 0x06}, {0xB0, 0x04}}, // uxga
  {{0x98, 0x03}, {0x38, 0x04}}, // FHD
  {{0xD0, 0x02}, {0x00, 0x05}}, // P_HD
  {{0x60, 0x03}, {0x00, 0x06}}, // P_3MP
  {{0x00, 0x08}, {0x00, 0x06}}, // QXGA
  {{0x00, 0x0A}, {0xA0, 0x05}}, // QHD
  {{0x00, 0x0A}, {0x40, 0x06}}, // WQXGA
  {{0x38, 0x04}, {0x80, 0x07}}, // P_FHD
  {{0x00, 0x0A}, {0x80, 0x07}}, // QSXGA
  {{0x20, 0x0A}, {0x98, 0x07}}  // 5MP
};
const size_t AviWriter::FRAME_SIZES_COUNT =
    sizeof(AviWriter::FRAME_SIZES) / sizeof(AviWriter::FRAME_SIZES[0]);

AviWriter::AviWriter(uint32_t maxFrames) : m_maxFrames(maxFrames) {
  memcpy(m_aviHeader, kAviHeaderTemplate, AVI_HEADER_LEN);
}

bool AviWriter::ensureIndexAlloc() {
  if (m_idxBuf) return true;
  // (maxFrames + 2) entries (room for header and safety)
  const size_t bytes = (m_maxFrames + 2) * IDX_ENTRY;
  m_idxBuf = (uint8_t*)ps_malloc(bytes); // ESP32 PSRAM if available
  if (!m_idxBuf) {
    // fallback to normal heap if ps_malloc not available
    m_idxBuf = (uint8_t*)malloc(bytes);
  }
  return m_idxBuf != nullptr;
}

bool AviWriter::begin(fs::FS& fs, const char* path, uint8_t frameType, uint8_t targetFps) {
  abort();

  m_fs = &fs;
  m_file = m_fs->open(path, FILE_WRITE);
  if (!m_file) return false;

  m_frameType = frameType;
  m_targetFps = targetFps;
  m_frameCnt = 0;
  m_startMs = millis();

  // reset header template
  memcpy(m_aviHeader, kAviHeaderTemplate, AVI_HEADER_LEN);

  // write placeholder header
  if (m_file.write(m_aviHeader, AVI_HEADER_LEN) != AVI_HEADER_LEN) {
    abort();
    return false;
  }

  prepAviIndex();
  return true;
}

void AviWriter::prepAviIndex() {
  if (!ensureIndexAlloc()) return;

  memcpy(m_idxBuf, IDX1_BUF, 4);     // idx1
  m_idxPtr = CHUNK_HDR;              // leave 4 bytes for index size after idx1
  m_moviSize = 0;
  m_indexLen = 0;
  m_idxOffset = 4;                   // initial offset inside movi list data
}

void AviWriter::buildAviIdx(uint32_t dataSize) {
  if (!m_idxBuf) return;

  // bounds check
  const size_t cap = (m_maxFrames + 2) * IDX_ENTRY;
  if (m_idxPtr + IDX_ENTRY > cap) {
    // index overflow: stop adding frames safely
    return;
  }

  m_moviSize += dataSize;

  // entry:
  // 0: "00dc"
  // 4: flags (0)
  // 8: offset
  // 12: size
  memcpy(m_idxBuf + m_idxPtr, DC_BUF, 4);
  memcpy(m_idxBuf + m_idxPtr + 4, ZERO_BUF, 4);
  memcpy(m_idxBuf + m_idxPtr + 8, &m_idxOffset, 4);
  memcpy(m_idxBuf + m_idxPtr + 12, &dataSize, 4);

  m_idxOffset += (size_t)dataSize + CHUNK_HDR;
  m_idxPtr += IDX_ENTRY;
}

void AviWriter::finalizeAviIndex() {
  if (!m_idxBuf) return;
  const uint32_t sizeOfIndex = (uint32_t)m_frameCnt * IDX_ENTRY;
  memcpy(m_idxBuf + 4, &sizeOfIndex, 4);
  m_indexLen = (size_t)sizeOfIndex + CHUNK_HDR; // idx1 + size + entries
  m_idxPtr = 0;
}

size_t AviWriter::writeAviIndex(uint8_t* out, size_t outSize) {
  if (!m_idxBuf || m_idxPtr >= m_indexLen) {
    m_idxPtr = 0;
    return 0;
  }
  const size_t remain = m_indexLen - m_idxPtr;
  const size_t n = (remain > outSize) ? outSize : remain;
  memcpy(out, m_idxBuf + m_idxPtr, n);
  m_idxPtr += n;
  return n;
}

void AviWriter::buildAviHdr(uint8_t fps, uint8_t frameType, uint16_t frameCnt) {
  // AVI content size used in RIFF size field (file size - 8)
  const size_t aviSize = m_moviSize + AVI_HEADER_LEN + ((CHUNK_HDR + IDX_ENTRY) * frameCnt);

  memcpy(m_aviHeader + 4, &aviSize, 4);

  const uint32_t usecs = (uint32_t)lround(1000000.0f / (float)fps);
  memcpy(m_aviHeader + 0x20, &usecs, 4);

  memcpy(m_aviHeader + 0x30, &frameCnt, 2);
  memcpy(m_aviHeader + 0x8C, &frameCnt, 2);

  memcpy(m_aviHeader + 0x84, &fps, 1);

  const uint32_t dataSize = (uint32_t)(m_moviSize + (frameCnt * CHUNK_HDR) + 4);
  memcpy(m_aviHeader + 0x12E, &dataSize, 4);

  // frame size patch (only if in range)
  if (frameType < FRAME_SIZES_COUNT) {
    memcpy(m_aviHeader + 0x40, FRAME_SIZES[frameType].w, 2);
    memcpy(m_aviHeader + 0xA8, FRAME_SIZES[frameType].w, 2);
    memcpy(m_aviHeader + 0x44, FRAME_SIZES[frameType].h, 2);
    memcpy(m_aviHeader + 0xAC, FRAME_SIZES[frameType].h, 2);
  }

  // audio size set to 0 (video-only)
  memcpy(m_aviHeader + 0x100, ZERO_BUF, 4);

  // reset state for potential reuse
  m_moviSize = 0;
  m_idxPtr = 0;
  m_idxOffset = 4;
}

bool AviWriter::addJpegFrame(const uint8_t* jpg, size_t jpgLen) {
  if (!m_file) return false;
  if (!jpg || jpgLen == 0) return false;

  // pad to 4-byte boundary
  const uint16_t filler = (uint16_t)((4 - (jpgLen & 0x3)) & 0x3);
  const uint32_t paddedSize = (uint32_t)jpgLen + filler;

  uint8_t hdr[CHUNK_HDR];
  memcpy(hdr, DC_BUF, 4);
  memcpy(hdr + 4, &paddedSize, 4);

  if (m_file.write(hdr, CHUNK_HDR) != CHUNK_HDR) return false;
  if (m_file.write(jpg, jpgLen) != jpgLen) return false;

  for (uint16_t i = 0; i < filler; i++) {
    if (m_file.write((uint8_t)0x00) != 1) return false;
  }

  buildAviIdx(paddedSize);

  m_frameCnt++;
  return true;
}

bool AviWriter::end() {
  if (!m_file) return false;

  // append idx1
  finalizeAviIndex();

  uint8_t out[4096];
  while (true) {
    const size_t n = writeAviIndex(out, sizeof(out));
    if (!n) break;
    if (m_file.write(out, n) != n) {
      abort();
      return false;
    }
  }

  // compute fps and patch header
  const uint32_t durMs = millis() - m_startMs;
  const float actualFps = (durMs > 0) ? (1000.0f * (float)m_frameCnt) / (float)durMs : (float)m_targetFps;
  const uint8_t actualFpsInt = (uint8_t)lroundf(actualFps > 1.0f ? actualFps : (float)m_targetFps);

  buildAviHdr(actualFpsInt, m_frameType, m_frameCnt);

  // overwrite header
  m_file.seek(0);
  if (m_file.write(m_aviHeader, AVI_HEADER_LEN) != AVI_HEADER_LEN) {
    abort();
    return false;
  }

  m_file.close();
  return true;
}

void AviWriter::abort() {
  if (m_file) m_file.close();
  m_fs = nullptr;
  m_frameCnt = 0;
  m_startMs = 0;
  m_frameType = 0;
  m_targetFps = 0;
  m_idxPtr = 0;
  m_idxOffset = 0;
  m_moviSize = 0;
  m_indexLen = 0;
}