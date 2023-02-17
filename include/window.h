// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "player.h"
#include "config.h"
#include "dispatch.h"

namespace ImPlay {
class Window {
 public:
  Window();
  ~Window();

  bool run(OptionParser &optionParser);

 private:
  void render();
  void requestRender();
  void updateCursor();

  void loadFonts();
  void initGLFW(const char *title);
  void initImGui();
  void exitGLFW();
  void exitImGui();

  Config config;
  Dispatch dispatch;
  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Player *player = nullptr;
  int width = 1280;
  int height = 720;

  const int defaultTimeout = 50;  // ms

  std::mutex renderMutex;
  std::condition_variable renderCond;
  std::atomic_int waitTimeout = defaultTimeout;
  bool ownCursor = false;
  bool wantRender = true;
  double lastRenderAt = 0;
  double lastInputAt = 0;
  double lastMousePressAt = 0;
};
}  // namespace ImPlay