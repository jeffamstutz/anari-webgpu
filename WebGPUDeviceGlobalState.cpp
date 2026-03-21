// Copyright 2026 The anari-webgpu Authors
// SPDX-License-Identifier: Apache-2.0

#include "WebGPUDeviceGlobalState.h"
#include <webgpu/webgpu.h>

namespace anari_webgpu {

WebGPUDeviceGlobalState::WebGPUDeviceGlobalState(ANARIDevice d)
    : helium::BaseGlobalDeviceState(d) {}

WebGPUDeviceGlobalState::~WebGPUDeviceGlobalState()
{
  if (wgpuQueue) { wgpuQueueRelease(wgpuQueue); wgpuQueue = nullptr; }
  if (wgpuDevice) { wgpuDeviceRelease(wgpuDevice); wgpuDevice = nullptr; }
  if (wgpuAdapter) { wgpuAdapterRelease(wgpuAdapter); wgpuAdapter = nullptr; }
  if (wgpuInstance) { wgpuInstanceRelease(wgpuInstance); wgpuInstance = nullptr; }
}

} // namespace anari_webgpu
