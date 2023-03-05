// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <map>
#include <imgui.h>

namespace ImPlay {
struct LangData {
  std::string code;
  std::string title;
  std::vector<std::pair<std::string, int>> fonts;
  std::map<std::string, std::string> entries;

  std::string get(std::string& key);
};

class LangStr {
 public:
  explicit LangStr(std::string key);

  operator std::string() const;
  operator std::string_view() const;
  operator const char*() const;

 private:
  std::string m_str;
};

const ImWchar* getLangGlyphRanges();
std::map<std::string, LangData>& getLangs();
std::string& getLangFallback();
std::string& getLang();
std::string i18n(std::string key);
}  // namespace ImPlay