// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "helpers.h"
#include "views/view.h"

namespace ImPlay::Views {
View::View(Config *config, Mpv *mpv) {
  this->config = config;
  this->mpv = mpv;
}
}  // namespace ImPlay::Views