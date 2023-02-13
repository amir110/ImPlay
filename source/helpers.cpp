// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <romfs/romfs.hpp>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <limits.h>
#include <sysdir.h>
#include <glob.h>
#endif
#ifdef IMGUI_IMPL_OPENGL_ES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include "helpers.h"

void ImGui::HalignCenter(const char* text) {
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f);
}

void ImGui::TextCentered(const char* text, bool disabled) {
  ImGui::HalignCenter(text);
  if (disabled)
    ImGui::TextDisabled("%s", text);
  else
    ImGui::Text("%s", text);
}

void ImGui::TextEllipsis(const char* text, float maxWidth) {
  if (maxWidth == 0) maxWidth = ImGui::GetContentRegionAvail().x;
  ImGuiStyle style = ImGui::GetStyle();
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImVec2 textSize = ImGui::CalcTextSize(text);
  ImVec2 min = ImGui::GetCursorScreenPos();
  ImVec2 max = min + ImVec2(maxWidth - style.FramePadding.x, textSize.y);
  ImRect textRect(min, max);
  ImGui::ItemSize(textRect);
  if (ImGui::ItemAdd(textRect, window->GetID(text)))
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), min, max, max.x, max.x, text, nullptr, &textSize);
}

void ImGui::Hyperlink(const char* label, const char* url) {
  auto style = ImGui::GetStyle();
  ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_CheckMark]);
  ImGui::Text("%s", label ? label : url);
  if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  if (ImGui::IsItemClicked()) ImPlay::openUrl(url);
  ImGui::PopStyleColor();
}

void ImGui::HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(35 * ImGui::GetFontSize());
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

ImTextureID ImGui::LoadTexture(const char* path, int* width, int* height) {
  int w, h;
  auto icon = romfs::get(path);
  auto data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(icon.data()), icon.size(), &w, &h, NULL, 4);
  if (data == nullptr) return nullptr;

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);

  if (width != nullptr) *width = w;
  if (height != nullptr) *height = h;

  return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
}

// return the second font
ImFont* ImGui::codeFont() {
  auto fonts = ImGui::GetIO().Fonts->Fonts;
  return fonts.size() > 1 ? fonts[1] : fonts[0];
}

void ImPlay::OptionParser::parse(int argc, char** argv) {
  bool optEnd = false;
#ifdef _WIN32
  int wideArgc;
  wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
  for (int i = 1; i < wideArgc; i++) {
    std::string arg = WideToUTF8(wideArgv[i]);
#else
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
#endif
    if (arg[0] == '-' && !optEnd) {
      if (arg[1] == '\0') continue;
      if (arg[1] == '-') {
        if (arg[2] == '\0') {
          optEnd = true;
          continue;
        } else {
          arg = arg.substr(2);
        }
      } else {
        arg = arg.substr(1);
      }
      if (arg.starts_with("no-")) {
        if (arg[3] == '\0') continue;
        options.emplace(arg.substr(3), "no");
      } else if (auto n = arg.find_first_of('='); n != std::string::npos) {
        options.emplace(arg.substr(0, n), arg.substr(n + 1));
      } else {
        options.emplace(arg, "yes");
      }
    } else {
      paths.emplace_back(arg);
    }
  }
}

bool ImPlay::OptionParser::check(std::string key, std::string value) {
  auto it = options.find(key);
  return it != options.end() && it->second == value;
}

std::string ImPlay::LangData::get(std::string& key) {
  auto it = entries.find(key);
  if (it != entries.end()) return it->second;
  return key;
}

const ImWchar* ImPlay::getLangGlyphRanges() {
  static ImVector<ImWchar> glyphRanges;
  if (glyphRanges.empty()) {
    ImFontGlyphRangesBuilder builder;
    for (auto& [code, lang] : ImPlay::getLangs()) {
      builder.AddText(lang.title.c_str());
      for (auto& [key, value] : lang.entries) builder.AddText(value.c_str());
    }
    builder.BuildRanges(&glyphRanges);
  }
  return &glyphRanges[0];
}

std::map<std::string, ImPlay::LangData>& ImPlay::getLangs() {
  static std::map<std::string, ImPlay::LangData> langs;
  static bool loaded = false;
  if (loaded) return langs;

  auto list = romfs::list("lang");
  for (auto& path : list) {
    auto file = romfs::get(path);
    auto j = nlohmann::json::parse(file.data(), file.data() + file.size());
    const auto& code = j["code"];
    const auto& title = j["title"];
    const auto& entries = j["entries"];
    if (!code.is_string() && !title.is_string() && !entries.is_object()) continue;
    if (j.contains("fallback")) {
      const auto& fallback = j["fallback"];
      if (fallback.is_boolean() && fallback.get<bool>()) ImPlay::getLangFallback() = code.get<std::string>();
    }
    auto lang = ImPlay::LangData{code.get<std::string>(), title.get<std::string>()};
    for (auto& [key, value] : entries.items()) {
      if (!value.is_string()) continue;
      lang.entries[key] = value.get<std::string>();
    }
    langs.insert({lang.code, lang});
  }
  loaded = true;
  return langs;
}

std::string& ImPlay::getLangFallback() {
  static std::string fallback = "en-US";
  return fallback;
}

std::string& ImPlay::getLang() {
  static std::string lang = "en-US";
  return lang;
}

std::string ImPlay::i18n(std::string key) {
  auto langs = ImPlay::getLangs();
  std::string value;
  auto it = langs.find(getLang());
  if (it != langs.end()) value = it->second.get(key);
  if (value.empty() || value == key) {
    it = langs.find(ImPlay::getLangFallback());
    if (it != langs.end()) value = it->second.get(key);
  }
  return value;
}

namespace ImPlay {
struct NFDWrapper {
  NFDWrapper() { init = check(NFD::Init()) == NFD_OKAY; }
  ~NFDWrapper() {
    if (init) NFD::Quit();
  }
  nfdresult_t check(nfdresult_t result) {
    if (result == NFD_ERROR) throw nfd_error(NFD::GetError());
    return result;
  }
  bool init = false;
};
}  // namespace ImPlay

void ImPlay::openFile(std::vector<std::pair<std::string, std::string>> filters,
                      std::function<void(std::string)> callback) {
  ImPlay::NFDWrapper w;
  nfdchar_t* outPath;
  std::vector<nfdu8filteritem_t> items;
  for (auto& [n, s] : filters) items.emplace_back(nfdu8filteritem_t{n.c_str(), s.c_str()});
  auto result = NFD::OpenDialog(outPath, items.data(), items.size());
  if (w.check(result) == NFD_OKAY) {
    callback(outPath);
    NFD::FreePath(outPath);
  }
}

void ImPlay::openFiles(std::vector<std::pair<std::string, std::string>> filters,
                       std::function<void(std::string, int)> callback) {
  ImPlay::NFDWrapper w;
  const nfdpathset_t* outPaths;
  std::vector<nfdu8filteritem_t> items;
  for (auto& [n, s] : filters) items.emplace_back(nfdu8filteritem_t{n.c_str(), s.c_str()});
  auto result = NFD::OpenDialogMultiple(outPaths, items.data(), items.size());
  if (w.check(result) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      callback(path, i);
    }
    NFD::PathSet::Free(outPaths);
  }
}

void ImPlay::openFolder(std::function<void(std::string)> callback) {
  ImPlay::NFDWrapper w;
  nfdchar_t* outPath;
  auto result = NFD::PickFolder(outPath);
  if (w.check(result) == NFD_OKAY) {
    callback(outPath);
    NFD::FreePath(outPath);
  }
}

int ImPlay::openUrl(std::string url) {
#ifdef __APPLE__
  return system(format("open '{}'", url).c_str());
#elif defined(_WIN32) || defined(__CYGWIN__)
  return ShellExecuteW(0, 0, UTF8ToWide(url).c_str(), 0, 0, SW_SHOW) > (HINSTANCE)32 ? 0 : 1;
#else
  char command[256];
  return system(format("xdg-open '{}'", url).c_str());
#endif
}

void ImPlay::revealInFolder(std::string path) {
#ifdef __APPLE__
  system(format("open -R '{}'", path).c_str());
#elif defined(_WIN32) || defined(__CYGWIN__)
  std::string arg = format("/select,\"{}\"", path);
  ShellExecuteW(0, 0, L"explorer", UTF8ToWide(arg).c_str(), 0, SW_SHOW);
#else
  auto fp = std::filesystem::path(reinterpret_cast<char8_t*>(path.data()));
  auto status = std::filesystem::status(fp);
  auto target = std::filesystem::is_directory(status) ? path : fp.parent_path().string();
  system(format("xdg-open '{}'", target).c_str());
#endif
}

std::filesystem::path ImPlay::dataPath() {
  std::string dataDir;
#ifdef _WIN32
  std::wstring path(MAX_PATH, '\0');
  if (GetModuleFileNameW(nullptr, path.data(), path.length()) > 0) {
    auto dir = std::filesystem::path(path).parent_path() / "portable_config";
    if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) return dir;
  }
  wchar_t* dir = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &dir))) {
    dataDir = WideToUTF8(dir);
    CoTaskMemFree(dir);
  }
#elif defined(__APPLE__)
  char path[PATH_MAX];
  auto state = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
  while ((state = sysdir_get_next_search_path_enumeration(state, path)) != 0) {
    glob_t g;
    if (glob(path, GLOB_TILDE, nullptr, &g) == 0) {
      dataDir = g.gl_pathv[0];
      globfree(&g);
    }
    break;
  }
#else
  char* home = getenv("HOME");
  char* xdg_dir = getenv("XDG_CONFIG_HOME");
  if (xdg_dir != nullptr)
    dataDir = xdg_dir;
  else if (home != nullptr)
    dataDir = format("{}/.config", home);
#endif
  return std::filesystem::path(dataDir) / "implay";
}

std::vector<std::string> ImPlay::split(const std::string& str, const std::string& sep) {
  std::vector<std::string> v;
  std::string::size_type pos1 = 0, pos2 = 0;
  while ((pos2 = str.find(sep, pos1)) != std::string::npos) {
    v.push_back(str.substr(pos1, pos2 - pos1));
    pos1 = pos2 + sep.size();
  }
  if (pos1 != str.length()) v.push_back(str.substr(pos1));
  return v;
}