#pragma once
#include <map>
#include "../mpv.h"
#include "about.h"
#include "command_palette.h"

namespace ImPlay::Views {
class ContextMenu {
 public:
  enum class Action {
    ABOUT,
    PALETTE,
    OPEN_FILE,
    OPEN_SUB,
  };

  ContextMenu(Mpv *mpv);
  ~ContextMenu();

  void draw();
  void setAction(Action action, std::function<void()> callback) { actionHandlers[action] = callback; }

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };

  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist();
  void drawPlaylist();
  void drawProfilelist();

  void action(Action action);
  void setTheme(Theme theme);

  Mpv *mpv = nullptr;
  Theme theme;
  bool demo = false;

  std::map<Action, std::function<void()>> actionHandlers;
};
}  // namespace ImPlay::Views