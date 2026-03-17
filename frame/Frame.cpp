// Copyright 2026 The anari-webgpu Authors
// SPDX-License-Identifier: Apache-2.0

#include "Frame.h"
#include "geometry/Geometry.h"
#include "material/Material.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace anari_webgpu {

Frame::Frame(WebGPUDeviceGlobalState *s) : helium::BaseFrame(s) {}
Frame::~Frame() { wait(); }

bool Frame::isValid() const { return m_valid; }

WebGPUDeviceGlobalState *Frame::deviceState() const {
  return (WebGPUDeviceGlobalState *)helium::BaseObject::m_state;
}

void Frame::commitParameters() {
  m_renderer = getParamObject<Renderer>("renderer");
  m_camera = getParamObject<Camera>("camera");
  m_world = getParamObject<World>("world");
  m_colorType = getParam<anari::DataType>("channel.color", ANARI_UNKNOWN);
  m_depthType = getParam<anari::DataType>("channel.depth", ANARI_UNKNOWN);
  m_size = getParam<uint2>("size", uint2(0u, 0u));
}

void Frame::finalize() {
  if (!m_renderer) reportMessage(ANARI_SEVERITY_WARNING, "missing 'renderer' on frame");
  if (!m_camera) reportMessage(ANARI_SEVERITY_WARNING, "missing 'camera' on frame");
  if (!m_world) reportMessage(ANARI_SEVERITY_WARNING, "missing 'world' on frame");
  const auto numPixels = m_size.x * m_size.y;
  m_perPixelBytes = 4 * (m_colorType == ANARI_FLOAT32_VEC4 ? 4 : 1);
  m_pixelBuffer.resize(numPixels * m_perPixelBytes);
  m_depthBuffer.resize(m_depthType == ANARI_FLOAT32 ? numPixels : 0);
  m_valid = m_renderer && m_camera && m_camera->isValid() && m_world && m_world->isValid();
}

bool Frame::getProperty(const std::string_view &name,
    ANARIDataType type, void *ptr, uint64_t size, uint32_t flags) {
  if (type == ANARI_FLOAT32 && name == "duration") {
    helium::writeToVoidP(ptr, m_duration);
    return true;
  }
  return 0;
}

void Frame::renderFrame() {
  auto start = std::chrono::steady_clock::now();
  auto *state = deviceState();
  state->commitBuffer.flush();

  // Fill background
  auto bgc = m_renderer ? m_renderer->background() : float4(0.f, 0.f, 0.f, 1.f);
  for (uint32_t y = 0; y < m_size.y; y++)
    for (uint32_t x = 0; x < m_size.x; x++)
      writeSample(x, y, PixelSample(bgc));

  // TODO: implement rasterizer

  auto end = std::chrono::steady_clock::now();
  m_duration = std::chrono::duration<float>(end - start).count();
}

void Frame::softwareRasterize() {
  // TODO: implement software rasterization
}

void *Frame::map(std::string_view channel, uint32_t *width,
    uint32_t *height, ANARIDataType *pixelType) {
  wait();
  *width = m_size.x; *height = m_size.y;
  if (channel == "channel.color") { *pixelType = m_colorType; return m_pixelBuffer.data(); }
  else if (channel == "channel.depth" && !m_depthBuffer.empty()) { *pixelType = ANARI_FLOAT32; return m_depthBuffer.data(); }
  *width = 0; *height = 0; *pixelType = ANARI_UNKNOWN; return nullptr;
}

void Frame::unmap(std::string_view) {}
int Frame::frameReady(ANARIWaitMask m) { if (m == ANARI_NO_WAIT) return ready(); wait(); return 1; }
void Frame::discard() {}
bool Frame::ready() const { return true; }
void Frame::wait() const {}

void Frame::writeSample(int x, int y, const PixelSample &s) {
  const auto idx = y * m_size.x + x;
  auto *color = m_pixelBuffer.data() + (idx * m_perPixelBytes);
  switch (m_colorType) {
  case ANARI_UFIXED8_VEC4: { auto c = helium::cvt_color_to_uint32(s.color); std::memcpy(color, &c, sizeof(c)); break; }
  case ANARI_UFIXED8_RGBA_SRGB: { auto c = helium::cvt_color_to_uint32_srgb(s.color); std::memcpy(color, &c, sizeof(c)); break; }
  case ANARI_FLOAT32_VEC4: { std::memcpy(color, &s.color, sizeof(s.color)); break; }
  default: break;
  }
  if (!m_depthBuffer.empty()) m_depthBuffer[idx] = s.depth;
}

} // namespace anari_webgpu

WEBGPU_ANARI_TYPEFOR_DEFINITION(anari_webgpu::Frame *);
