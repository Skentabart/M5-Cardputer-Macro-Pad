/*
  ============================================================
  M5 Cardputer Macro Pad V4.7
  ============================================================

  Основа: код пользователя V4.

  V4.7 UI:
  - Fn+N = create new macro
  - Fn+M = MACRO / KEYBOARD
  - Fn+; = previous macro
  - Fn+. = next macro
  - Pages of 10 macros: 0..9 / 10..19 / 20..29; SPACE runs selected macro
  - UP/DOWN = standard navigation
  - ENTER = select/open
  - ESC = back/cancel
  - Macro editor has a dedicated menu with reliable ESC/UP/DOWN handling
  - Action list has a dedicated "Add action..." item
  - Selected action opens a dedicated menu: Edit / Move Up / Move Down / Delete / Add

  Возможности:
  - USB HID клавиатура
  - До 30 макросов
  - До 24 действий в каждом макросе
  - KEY / COMBINATION / TEXT / DELAY
  - Захват физических клавиш Cardputer
  - Захват комбинаций Ctrl/Shift/Alt/Fn(Win)
  - Настраиваемый trigger для каждого макроса
  - Переименование макросов
  - Перемещение / удаление действий
  - Сохранение во Flash через Preferences
  - Административные и диагностические макросы Windows
  - Режим обычной USB-клавиатуры

  Совместимо с API пользователя:
  - M5Cardputer.update()
  - Keyboard.isChange()
  - Keyboard.isPressed()
  - Keyboard.keysState()
  - KeysState::word = std::vector<char>
  - KeysState::fn / ctrl / alt / shift
  - KeysState::hid_keys = std::vector<uint8_t>

  НЕ используются:
  - state.backspace / esc / up / down
  - Keyboard.getKey() без аргументов
  - USB host library

  ВАЖНО:
  - HID usage 0x15 (R) НЕ отправляется напрямую в USBHIDKeyboard:
    он преобразуется в ASCII 'r'.
  - HID usage 0x28 (Enter) преобразуется в KEY_RETURN.
  - Это устраняет проблему Win+R -> cmd(.
*/

#include <Arduino.h>
#include <M5Cardputer.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <Preferences.h>
#include <vector>

// ============================================================
// Hardware / storage
// ============================================================

USBHIDKeyboard usbKeyboard;
Preferences prefs;
M5Canvas canvas(&M5Cardputer.Display);

// ============================================================
// Limits
// ============================================================

#define MAX_MACROS     30
#define MAX_ACTIONS    24
#define MAX_NAME_LEN   24
#define MAX_TEXT_LEN   180
#define STORAGE_VER    5

// ============================================================
// Modifier flags
// ============================================================

#define MOD_CTRL   0x01
#define MOD_SHIFT  0x02
#define MOD_ALT    0x04
#define MOD_GUI    0x08

// ============================================================
// Action types
// ============================================================

enum ActionType : uint8_t {
  ACTION_NONE  = 0,
  ACTION_KEY   = 1,
  ACTION_COMBO = 2,
  ACTION_TEXT  = 3,
  ACTION_DELAY = 4
};

// ============================================================
// Screens
// ============================================================

enum Screen : uint8_t {
  SCREEN_MACROS = 0,
  SCREEN_MACRO_PICKER,
  SCREEN_MACRO_MENU,
  SCREEN_ACTIONS,
  SCREEN_ACTION_MENU,
  SCREEN_ACTION_TYPE,
  SCREEN_CAPTURE_KEY,
  SCREEN_CAPTURE_TRIGGER,
  SCREEN_TEXT,
  SCREEN_DELAY,
  SCREEN_RENAME,
  SCREEN_SETTINGS,
  SCREEN_CONFIRM,
  SCREEN_HELP
};

// ============================================================
// HID usage IDs
// ============================================================

#define HID_A            0x04
#define HID_B            0x05
#define HID_C            0x06
#define HID_D            0x07
#define HID_E            0x08
#define HID_F            0x09
#define HID_G            0x0A
#define HID_H            0x0B
#define HID_I            0x0C
#define HID_J            0x0D
#define HID_K            0x0E
#define HID_L            0x0F
#define HID_M            0x10
#define HID_N            0x11
#define HID_O            0x12
#define HID_P            0x13
#define HID_Q            0x14
#define HID_R            0x15
#define HID_S            0x16
#define HID_T            0x17
#define HID_U            0x18
#define HID_V            0x19
#define HID_W            0x1A
#define HID_X            0x1B
#define HID_Y            0x1C
#define HID_Z            0x1D

#define HID_1            0x1E
#define HID_2            0x1F
#define HID_3            0x20
#define HID_4            0x21
#define HID_5            0x22
#define HID_6            0x23
#define HID_7            0x24
#define HID_8            0x25
#define HID_9            0x26
#define HID_0            0x27

#define HID_ENTER        0x28
#define HID_ESC          0x29
#define HID_BACKSPACE    0x2A
#define HID_TAB          0x2B
#define HID_SPACE        0x2C
#define HID_MINUS        0x2D
#define HID_EQUAL        0x2E
#define HID_LBRACKET     0x2F
#define HID_RBRACKET     0x30
#define HID_BACKSLASH    0x31
#define HID_SEMICOLON    0x33
#define HID_APOSTROPHE   0x34
#define HID_GRAVE        0x35
#define HID_COMMA        0x36
#define HID_DOT          0x37
#define HID_SLASH        0x38
#define HID_CAPS         0x39

#define HID_F1           0x3A
#define HID_F2           0x3B
#define HID_F3           0x3C
#define HID_F4           0x3D
#define HID_F5           0x3E
#define HID_F6           0x3F
#define HID_F7           0x40
#define HID_F8           0x41
#define HID_F9           0x42
#define HID_F10          0x43
#define HID_F11          0x44
#define HID_F12          0x45

#define HID_PRINT        0x46
#define HID_SCROLL       0x47
#define HID_PAUSE        0x48
#define HID_INSERT       0x49
#define HID_HOME         0x4A
#define HID_PAGEUP       0x4B
#define HID_DELETE       0x4C
#define HID_END          0x4D
#define HID_PAGEDOWN     0x4E
#define HID_RIGHT        0x4F
#define HID_LEFT         0x50
#define HID_DOWN         0x51
#define HID_UP           0x52

// USB HID modifier usages.
#define HID_LCTRL        0xE0
#define HID_LSHIFT       0xE1
#define HID_LALT         0xE2
#define HID_LGUI         0xE3
#define HID_RCTRL        0xE4
#define HID_RSHIFT       0xE5
#define HID_RALT         0xE6
#define HID_RGUI         0xE7

// ============================================================
// Data structures
// ============================================================

struct MacroAction {
  uint8_t type;
  uint8_t key;
  uint8_t modifiers;
  uint16_t delayMs;
  String text;
};

struct Macro {
  String name;
  uint8_t triggerKey;
  uint8_t triggerModifiers;
  MacroAction actions[MAX_ACTIONS];
  uint8_t actionCount;
  bool enabled;
};

Macro macros[MAX_MACROS];
uint8_t macroCount = 0;
int selectedMacro = 0;
int selectedAction = 0;
int macroPage = 0;

const int MACROS_PER_PAGE = 10;
int editActionIndex = -1;
Screen screen = SCREEN_MACROS;

String editBuffer;
uint16_t delayEditValue = 500;
bool keyboardMode = false;
bool captureComboMode = false;
bool editCreatedNewAction = false;

int actionTypeSelection = 0;
int settingsSelection = 0;
int helpPage = 0;
int macroMenuSelection = 0;
int actionMenuSelection = 0;
String macroPickerBuffer;
String macroPickerMessage;
bool legacyStorageV4 = false;

bool confirmYes = false;
enum ConfirmAction : uint8_t {
  CONFIRM_NONE = 0,
  CONFIRM_DELETE_MACRO,
  CONFIRM_RESTORE_DEFAULTS
};
ConfirmAction confirmAction = CONFIRM_NONE;

unsigned long lastMacroRunMs = 0;
unsigned long lastRedrawMs = 0;

// Forward declaration used by UI helper functions defined below.
void redraw();
void appendUsefulDefaults();

// ============================================================
// Small helpers
// ============================================================

char lowerAscii(char c) {
  if (c >= 'A' && c <= 'Z') return char(c + ('a' - 'A'));
  return c;
}

String wordToString(const Keyboard_Class::KeysState &s) {
  String result;
  result.reserve(s.word.size());
  for (size_t i = 0; i < s.word.size(); ++i) {
    result += s.word[i];
  }
  return result;
}

bool wordAvailable(const Keyboard_Class::KeysState &s) {
  return !s.word.empty();
}

char firstWordChar(const Keyboard_Class::KeysState &s) {
  if (s.word.empty()) return 0;
  return s.word[0];
}

bool hasHIDKey(const Keyboard_Class::KeysState &s, uint8_t code) {
  for (size_t i = 0; i < s.hid_keys.size(); ++i) {
    if (s.hid_keys[i] == code) return true;
  }
  return false;
}

// ------------------------------------------------------------
// Reliable Cardputer navigation helpers
// ------------------------------------------------------------
// The Cardputer has no dedicated physical ESC / arrow keys. On the
// standard matrix, Fn+` produces ESC, Fn+; produces UP, and Fn+.
// produces DOWN. We check both KeysState HID codes and the raw key
// position API so this also works with older M5Cardputer versions.
bool isEscPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_ESC)) return true;
  return s.fn && M5Cardputer.Keyboard.isKeyPressed('`');
}

bool isEnterPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_ENTER)) return true;
  return M5Cardputer.Keyboard.isKeyPressed('\r');
}

bool isBackspacePressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_BACKSPACE)) return true;
  return M5Cardputer.Keyboard.isKeyPressed('\b');
}

bool isTabPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_TAB)) return true;
  return M5Cardputer.Keyboard.isKeyPressed('\t');
}

bool isUpPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_UP)) return true;
  return s.fn && M5Cardputer.Keyboard.isKeyPressed(';');
}

bool isDownPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_DOWN)) return true;
  return s.fn && M5Cardputer.Keyboard.isKeyPressed('.');
}

bool isLeftPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_LEFT)) return true;
  return s.fn && M5Cardputer.Keyboard.isKeyPressed(',');
}

bool isRightPressed(const Keyboard_Class::KeysState &s) {
  if (hasHIDKey(s, HID_RIGHT)) return true;
  return s.fn && M5Cardputer.Keyboard.isKeyPressed('/');
}

bool isModifierHID(uint8_t code) {
  return code >= HID_LCTRL && code <= HID_RGUI;
}

uint8_t modifiersFromState(const Keyboard_Class::KeysState &s) {
  uint8_t mods = 0;

  if (s.ctrl)  mods |= MOD_CTRL;
  if (s.shift) mods |= MOD_SHIFT;
  if (s.alt)   mods |= MOD_ALT;

  // The local Cardputer Fn modifier is represented as GUI/Win in macros.
  if (s.fn)    mods |= MOD_GUI;

  return mods;
}

bool isCancelCapture(const Keyboard_Class::KeysState &s) {
  return s.ctrl && s.alt && isBackspacePressed(s);
}

// ============================================================
// HID -> USBHIDKeyboard
// ============================================================

uint8_t hidToKeyboardKey(uint8_t key) {
  if (key >= HID_A && key <= HID_Z) {
    return uint8_t('a' + (key - HID_A));
  }

  if (key >= HID_1 && key <= HID_9) {
    return uint8_t('1' + (key - HID_1));
  }

  if (key == HID_0) return '0';

  switch (key) {
    case HID_ENTER:     return KEY_RETURN;
    case HID_ESC:       return KEY_ESC;
    case HID_BACKSPACE: return KEY_BACKSPACE;
    case HID_TAB:       return KEY_TAB;
    case HID_SPACE:     return KEY_SPACE;
    case HID_MINUS:     return '-';
    case HID_EQUAL:     return '=';
    case HID_LBRACKET:  return '[';
    case HID_RBRACKET:  return ']';
    case HID_BACKSLASH: return '\\';
    case HID_SEMICOLON: return ';';
    case HID_APOSTROPHE:return '\'';
    case HID_GRAVE:     return '`';
    case HID_COMMA:     return ',';
    case HID_DOT:       return '.';
    case HID_SLASH:     return '/';

    case HID_F1:        return KEY_F1;
    case HID_F2:        return KEY_F2;
    case HID_F3:        return KEY_F3;
    case HID_F4:        return KEY_F4;
    case HID_F5:        return KEY_F5;
    case HID_F6:        return KEY_F6;
    case HID_F7:        return KEY_F7;
    case HID_F8:        return KEY_F8;
    case HID_F9:        return KEY_F9;
    case HID_F10:       return KEY_F10;
    case HID_F11:       return KEY_F11;
    case HID_F12:       return KEY_F12;

    case HID_INSERT:    return KEY_INSERT;
    case HID_HOME:      return KEY_HOME;
    case HID_PAGEUP:    return KEY_PAGE_UP;
    case HID_DELETE:    return KEY_DELETE;
    case HID_END:       return KEY_END;
    case HID_PAGEDOWN:  return KEY_PAGE_DOWN;
    case HID_RIGHT:     return KEY_RIGHT_ARROW;
    case HID_LEFT:      return KEY_LEFT_ARROW;
    case HID_DOWN:      return KEY_DOWN_ARROW;
    case HID_UP:        return KEY_UP_ARROW;
  }

  // CAPS/PRINT/SCROLL/PAUSE remain capturable and storable, but we do not
  // depend on optional KEY_* constants that may be absent in this USB core.
  return 0;
}

// ============================================================
// Character -> HID usage
// ============================================================

uint8_t charToHID(char c, uint8_t &mods) {
  mods = 0;

  if (c >= 'A' && c <= 'Z') {
    mods |= MOD_SHIFT;
    c = lowerAscii(c);
  }

  if (c >= 'a' && c <= 'z') {
    return uint8_t(HID_A + (c - 'a'));
  }

  if (c >= '1' && c <= '9') {
    return uint8_t(HID_1 + (c - '1'));
  }

  if (c == '0') return HID_0;

  switch (c) {
    case '\n': return HID_ENTER;
    case '\r': return HID_ENTER;
    case '\t': return HID_TAB;
    case 0x08: return HID_BACKSPACE;
    case ' ':  return HID_SPACE;

    case '!': mods |= MOD_SHIFT; return HID_1;
    case '@': mods |= MOD_SHIFT; return HID_2;
    case '#': mods |= MOD_SHIFT; return HID_3;
    case '$': mods |= MOD_SHIFT; return HID_4;
    case '%': mods |= MOD_SHIFT; return HID_5;
    case '^': mods |= MOD_SHIFT; return HID_6;
    case '&': mods |= MOD_SHIFT; return HID_7;
    case '*': mods |= MOD_SHIFT; return HID_8;
    case '(': mods |= MOD_SHIFT; return HID_9;
    case ')': mods |= MOD_SHIFT; return HID_0;
    case '_': mods |= MOD_SHIFT; return HID_MINUS;
    case '+': mods |= MOD_SHIFT; return HID_EQUAL;
    case '{': mods |= MOD_SHIFT; return HID_LBRACKET;
    case '}': mods |= MOD_SHIFT; return HID_RBRACKET;
    case '|': mods |= MOD_SHIFT; return HID_BACKSLASH;
    case ':': mods |= MOD_SHIFT; return HID_SEMICOLON;
    case '"':mods |= MOD_SHIFT; return HID_APOSTROPHE;
    case '~': mods |= MOD_SHIFT; return HID_GRAVE;
    case '<': mods |= MOD_SHIFT; return HID_COMMA;
    case '>': mods |= MOD_SHIFT; return HID_DOT;
    case '?': mods |= MOD_SHIFT; return HID_SLASH;

    case '-': return HID_MINUS;
    case '=': return HID_EQUAL;
    case '[': return HID_LBRACKET;
    case ']': return HID_RBRACKET;
    case '\\':return HID_BACKSLASH;
    case ';': return HID_SEMICOLON;
    case '\'':return HID_APOSTROPHE;
    case '`': return HID_GRAVE;
    case ',': return HID_COMMA;
    case '.': return HID_DOT;
    case '/': return HID_SLASH;
  }

  return 0;
}

// ============================================================
// Extract one key/combo from KeysState
// ============================================================

bool extractKeyCombo(const Keyboard_Class::KeysState &s,
                     uint8_t &key,
                     uint8_t &mods) {
  key = 0;
  mods = modifiersFromState(s);

  // HID keys first: arrows, Enter, Esc, F keys, etc.
  for (size_t i = 0; i < s.hid_keys.size(); ++i) {
    uint8_t h = s.hid_keys[i];

    if (h == 0 || isModifierHID(h)) continue;

    key = h;
    return true;
  }

  // Printable key path: KeysState::word is std::vector<char>.
  // Some Cardputer library versions expose Ctrl/Alt combinations as
  // uppercase letters even when Shift is NOT pressed. Lowercase letters
  // here so Ctrl+R is not accidentally stored as Ctrl+Shift+R.
  if (!s.word.empty()) {
    char c = s.word[0];
    if (c >= 'A' && c <= 'Z' && !s.shift) c = lowerAscii(c);

    uint8_t charMods = 0;
    key = charToHID(c, charMods);
    mods |= charMods;
    return key != 0;
  }

  return false;
}

bool fnHIDKey(const Keyboard_Class::KeysState &s, uint8_t code) {
  return s.fn && hasHIDKey(s, code);
}


// ============================================================
// Names
// ============================================================

String hidKeyName(uint8_t key) {
  if (key >= HID_A && key <= HID_Z) {
    String s;
    s += char('A' + key - HID_A);
    return s;
  }

  if (key >= HID_1 && key <= HID_9) {
    String s;
    s += char('1' + key - HID_1);
    return s;
  }

  if (key == HID_0) return "0";

  switch (key) {
    case HID_ENTER: return "ENTER";
    case HID_ESC: return "ESC";
    case HID_BACKSPACE: return "BACKSPACE";
    case HID_TAB: return "TAB";
    case HID_SPACE: return "SPACE";
    case HID_MINUS: return "-";
    case HID_EQUAL: return "=";
    case HID_LBRACKET: return "[";
    case HID_RBRACKET: return "]";
    case HID_BACKSLASH: return "\\";
    case HID_SEMICOLON: return ";";
    case HID_APOSTROPHE: return "'";
    case HID_GRAVE: return "`";
    case HID_COMMA: return ",";
    case HID_DOT: return ".";
    case HID_SLASH: return "/";
    case HID_CAPS: return "CAPS";
    case HID_F1: return "F1";
    case HID_F2: return "F2";
    case HID_F3: return "F3";
    case HID_F4: return "F4";
    case HID_F5: return "F5";
    case HID_F6: return "F6";
    case HID_F7: return "F7";
    case HID_F8: return "F8";
    case HID_F9: return "F9";
    case HID_F10: return "F10";
    case HID_F11: return "F11";
    case HID_F12: return "F12";
    case HID_PRINT: return "PRINT";
    case HID_SCROLL: return "SCROLL";
    case HID_PAUSE: return "PAUSE";
    case HID_INSERT: return "INSERT";
    case HID_HOME: return "HOME";
    case HID_PAGEUP: return "PGUP";
    case HID_DELETE: return "DELETE";
    case HID_END: return "END";
    case HID_PAGEDOWN: return "PGDN";
    case HID_RIGHT: return "RIGHT";
    case HID_LEFT: return "LEFT";
    case HID_DOWN: return "DOWN";
    case HID_UP: return "UP";
  }

  return "KEY?";
}

String modifiersName(uint8_t mods) {
  String s;
  if (mods & MOD_CTRL)  s += "Ctrl+";
  if (mods & MOD_SHIFT) s += "Shift+";
  if (mods & MOD_ALT)   s += "Alt+";
  if (mods & MOD_GUI)   s += "Win+";
  return s;
}

String actionSummary(const MacroAction &a) {
  switch (a.type) {
    case ACTION_KEY:
      return String("KEY ") + modifiersName(a.modifiers) + hidKeyName(a.key);

    case ACTION_COMBO:
      return String("COMBO ") + modifiersName(a.modifiers) + hidKeyName(a.key);

    case ACTION_TEXT: {
      String s = "TEXT: ";
      s += a.text;
      if (s.length() > 28) s = s.substring(0, 28) + "...";
      return s;
    }

    case ACTION_DELAY:
      return String("DELAY ") + String(a.delayMs) + " ms";
  }

  return "EMPTY";
}

// ============================================================
// Action / macro clearing
// ============================================================

void clearAction(MacroAction &a) {
  a.type = ACTION_NONE;
  a.key = 0;
  a.modifiers = 0;
  a.delayMs = 0;
  a.text = "";
}

void clearMacro(Macro &m) {
  m.name = "";
  m.triggerKey = 0;
  m.triggerModifiers = 0;
  m.actionCount = 0;
  m.enabled = true;

  for (int i = 0; i < MAX_ACTIONS; ++i) {
    clearAction(m.actions[i]);
  }
}

// ============================================================
// Storage - same key layout as user's V4
// ============================================================

void saveAll() {
  prefs.begin("macro4", false);

  prefs.putUInt("ver", STORAGE_VER);
  prefs.putUInt("count", macroCount);

  for (int i = 0; i < macroCount; ++i) {
    String base = "m" + String(i);

    // Preserve the original V4 key names.
    prefs.putString((base + "n").c_str(), macros[i].name);
    prefs.putUChar((base + "k").c_str(), macros[i].triggerKey);
    prefs.putUChar((base + "mod").c_str(), macros[i].triggerModifiers);
    prefs.putUChar((base + "cnt").c_str(), macros[i].actionCount);
    prefs.putBool((base + "en").c_str(), macros[i].enabled);

    for (int j = 0; j < macros[i].actionCount; ++j) {
      String ab = base + "a" + String(j);
      MacroAction &a = macros[i].actions[j];

      prefs.putUChar((ab + "t").c_str(), a.type);
      prefs.putUChar((ab + "k").c_str(), a.key);
      prefs.putUChar((ab + "m").c_str(), a.modifiers);
      prefs.putUInt((ab + "d").c_str(), a.delayMs);
      prefs.putString((ab + "x").c_str(), a.text);
    }
  }

  prefs.end();
}

bool loadAll() {
  prefs.begin("macro4", false);

  uint32_t version = prefs.getUInt("ver", 0);
  uint32_t count32 = prefs.getUInt("count", 0);

  legacyStorageV4 = (version == 4);

  if ((version != 4 && version != STORAGE_VER) || count32 > MAX_MACROS) {
    prefs.end();
    legacyStorageV4 = false;
    return false;
  }

  macroCount = uint8_t(count32);

  // A valid database may intentionally contain zero macros.
  // Do not recreate defaults just because the user deleted everything.

  for (int i = 0; i < macroCount; ++i) {
    clearMacro(macros[i]);

    String base = "m" + String(i);

    macros[i].name = prefs.getString((base + "n").c_str(), "Macro");
    macros[i].triggerKey = prefs.getUChar((base + "k").c_str(), 0);
    macros[i].triggerModifiers = prefs.getUChar((base + "mod").c_str(), 0);
    macros[i].actionCount = prefs.getUChar((base + "cnt").c_str(), 0);
    macros[i].enabled = prefs.getBool((base + "en").c_str(), true);

    if (macros[i].actionCount > MAX_ACTIONS) {
      macros[i].actionCount = MAX_ACTIONS;
    }

    for (int j = 0; j < macros[i].actionCount; ++j) {
      String ab = base + "a" + String(j);
      MacroAction &a = macros[i].actions[j];

      a.type = prefs.getUChar((ab + "t").c_str(), ACTION_NONE);
      a.key = prefs.getUChar((ab + "k").c_str(), 0);
      a.modifiers = prefs.getUChar((ab + "m").c_str(), 0);
      a.delayMs = uint16_t(prefs.getUInt((ab + "d").c_str(), 0));
      a.text = prefs.getString((ab + "x").c_str(), "");
    }
  }

  prefs.end();
  return true;
}

// ============================================================
// Default macro construction
// ============================================================

void addKeyAction(Macro &m, uint8_t key, uint8_t modifiers = 0) {
  if (m.actionCount >= MAX_ACTIONS) return;

  MacroAction &a = m.actions[m.actionCount++];
  clearAction(a);

  a.type = modifiers ? ACTION_COMBO : ACTION_KEY;
  a.key = key;
  a.modifiers = modifiers;
}

void addTextAction(Macro &m, const char *text) {
  if (m.actionCount >= MAX_ACTIONS) return;

  MacroAction &a = m.actions[m.actionCount++];
  clearAction(a);

  a.type = ACTION_TEXT;
  a.text = text;
}

void addDelayAction(Macro &m, uint16_t ms) {
  if (m.actionCount >= MAX_ACTIONS) return;

  MacroAction &a = m.actions[m.actionCount++];
  clearAction(a);

  a.type = ACTION_DELAY;
  a.delayMs = ms;
}

void makeRunMacro(Macro &m,
                  const char *name,
                  uint8_t triggerKey,
                  uint8_t triggerModifiers,
                  const char *command) {
  clearMacro(m);

  m.name = name;
  m.triggerKey = triggerKey;
  m.triggerModifiers = triggerModifiers;
  m.enabled = true;

  addKeyAction(m, HID_R, MOD_GUI);
  addDelayAction(m, 600);
  addTextAction(m, command);
  addKeyAction(m, HID_ENTER);
}

void createDefaults() {
  macroCount = 0;

  // Default triggers use Win/GUI+F1..F12.
  // Fn+N/M/;/ . are reserved for UI control.
  // UI does NOT reserve those combinations, so triggers remain usable.
  makeRunMacro(macros[macroCount++], "CMD",            HID_F1,  MOD_GUI, "cmd");
  makeRunMacro(macros[macroCount++], "PowerShell",     HID_F2,  MOD_GUI, "powershell");

  clearMacro(macros[macroCount]);
  macros[macroCount].name = "Task Manager";
  macros[macroCount].triggerKey = HID_F3;
  macros[macroCount].triggerModifiers = MOD_GUI;
  addKeyAction(macros[macroCount], HID_ESC, MOD_CTRL | MOD_SHIFT);
  macroCount++;

  makeRunMacro(macros[macroCount++], "Device Manager",  HID_F4,  MOD_GUI, "devmgmt.msc");
  makeRunMacro(macros[macroCount++], "Services",        HID_F5,  MOD_GUI, "services.msc");
  makeRunMacro(macros[macroCount++], "Event Viewer",    HID_F6,  MOD_GUI, "eventvwr.msc");
  makeRunMacro(macros[macroCount++], "Computer Mgmt",   HID_F7,  MOD_GUI, "compmgmt.msc");
  makeRunMacro(macros[macroCount++], "Disk Management", HID_F8,  MOD_GUI, "diskmgmt.msc");
  makeRunMacro(macros[macroCount++], "Network",         HID_F9,  MOD_GUI, "ncpa.cpl");
  makeRunMacro(macros[macroCount++], "Control Panel",   HID_F10, MOD_GUI, "control");
  makeRunMacro(macros[macroCount++], "System Info",     HID_F11, MOD_GUI, "msinfo32");
  makeRunMacro(macros[macroCount++], "WhoAmI",          HID_F12, MOD_GUI, "cmd /k whoami");

  makeRunMacro(macros[macroCount++], "IPConfig",        HID_1, MOD_GUI, "cmd /k ipconfig /all");
  makeRunMacro(macros[macroCount++], "Netstat",         HID_2, MOD_GUI, "cmd /k netstat -ano");
  makeRunMacro(macros[macroCount++], "TaskList",        HID_3, MOD_GUI, "cmd /k tasklist");
  makeRunMacro(macros[macroCount++], "SystemInfo",      HID_4, MOD_GUI, "cmd /k systeminfo");
  makeRunMacro(macros[macroCount++], "Route",            HID_5, MOD_GUI, "cmd /k route print");
  makeRunMacro(macros[macroCount++], "ARP",              HID_6, MOD_GUI, "cmd /k arp -a");
  makeRunMacro(macros[macroCount++], "Firewall",         HID_7, MOD_GUI, "wf.msc");
  makeRunMacro(macros[macroCount++], "PS Services",     HID_8, MOD_GUI, "powershell Get-Service");

  // ----------------------------------------------------------
  // 21..30. Extra practical IT / diagnostics macros.
  // Triggers use Shift+Fn+F1..F10 so they do not collide with the
  // main Fn shortcuts or Ctrl+Alt editor shortcuts.
  // ----------------------------------------------------------
  makeRunMacro(macros[macroCount++], "Resource Monitor",    HID_F1, MOD_GUI | MOD_SHIFT, "resmon.exe");
  makeRunMacro(macros[macroCount++], "Reliability Monitor", HID_F2, MOD_GUI | MOD_SHIFT, "perfmon /rel");
  makeRunMacro(macros[macroCount++], "Task Scheduler",     HID_F3, MOD_GUI | MOD_SHIFT, "taskschd.msc");
  makeRunMacro(macros[macroCount++], "System Configuration",HID_F4, MOD_GUI | MOD_SHIFT, "msconfig.exe");
  makeRunMacro(macros[macroCount++], "System Properties",   HID_F5, MOD_GUI | MOD_SHIFT, "sysdm.cpl");
  makeRunMacro(macros[macroCount++], "Battery Report",      HID_F6, MOD_GUI | MOD_SHIFT, "cmd /k powercfg /batteryreport /output \"%USERPROFILE%\\Desktop\\battery-report.html\"");
  makeRunMacro(macros[macroCount++], "WiFi Report",         HID_F7, MOD_GUI | MOD_SHIFT, "cmd /k netsh wlan show wlanreport");
  makeRunMacro(macros[macroCount++], "SFC Scan",            HID_F8, MOD_GUI | MOD_SHIFT, "cmd /k sfc /scannow");
  makeRunMacro(macros[macroCount++], "DISM CheckHealth",    HID_F9, MOD_GUI | MOD_SHIFT, "cmd /k DISM /Online /Cleanup-Image /CheckHealth");
  makeRunMacro(macros[macroCount++], "Windows Update",      HID_F10, MOD_GUI | MOD_SHIFT, "ms-settings:windowsupdate");

  saveAll();
}

// Add only the new V4.7 library entries to an existing V4 database.
// Existing user-created macros are never overwritten.
void appendUsefulDefaults() {
  if (macroCount >= MAX_MACROS) return;

  auto existsByName = [](const char *name) -> bool {
    for (int i = 0; i < macroCount; ++i) {
      if (macros[i].name == name) return true;
    }
    return false;
  };

  auto append = [&](const char *name, uint8_t key, uint8_t mods, const char *cmd) {
    if (macroCount >= MAX_MACROS || existsByName(name)) return;
    makeRunMacro(macros[macroCount++], name, key, mods, cmd);
  };

  append("Resource Monitor",     HID_F1, MOD_GUI | MOD_SHIFT, "resmon.exe");
  append("Reliability Monitor",  HID_F2, MOD_GUI | MOD_SHIFT, "perfmon /rel");
  append("Task Scheduler",       HID_F3, MOD_GUI | MOD_SHIFT, "taskschd.msc");
  append("System Configuration",  HID_F4, MOD_GUI | MOD_SHIFT, "msconfig.exe");
  append("System Properties",     HID_F5, MOD_GUI | MOD_SHIFT, "sysdm.cpl");
  append("Battery Report",        HID_F6, MOD_GUI | MOD_SHIFT, "cmd /k powercfg /batteryreport /output \"%USERPROFILE%\\Desktop\\battery-report.html\"");
  append("WiFi Report",           HID_F7, MOD_GUI | MOD_SHIFT, "cmd /k netsh wlan show wlanreport");
  append("SFC Scan",              HID_F8, MOD_GUI | MOD_SHIFT, "cmd /k sfc /scannow");
  append("DISM CheckHealth",      HID_F9, MOD_GUI | MOD_SHIFT, "cmd /k DISM /Online /Cleanup-Image /CheckHealth");
  append("Windows Update",        HID_F10, MOD_GUI | MOD_SHIFT, "ms-settings:windowsupdate");
}

// ============================================================
// USB HID output
// ============================================================

void releaseAllKeyboard() {
  usbKeyboard.releaseAll();
  delay(25);
}

void sendCombination(uint8_t modifiers, uint8_t key) {
  if (modifiers & MOD_CTRL)  usbKeyboard.press(KEY_LEFT_CTRL);
  if (modifiers & MOD_SHIFT) usbKeyboard.press(KEY_LEFT_SHIFT);
  if (modifiers & MOD_ALT)   usbKeyboard.press(KEY_LEFT_ALT);
  if (modifiers & MOD_GUI)   usbKeyboard.press(KEY_LEFT_GUI);

  delay(60);

  uint8_t outputKey = hidToKeyboardKey(key);

  if (outputKey != 0) {
    usbKeyboard.press(outputKey);
    delay(80);
    usbKeyboard.release(outputKey);
  }

  delay(35);
  releaseAllKeyboard();
  delay(50);
}

void sendTextToPC(const String &text) {
  if (text.length() == 0) return;
  usbKeyboard.print(text);
  delay(80);
}

void executeAction(const MacroAction &a) {
  switch (a.type) {
    case ACTION_KEY:
    case ACTION_COMBO:
      if (a.key != 0) sendCombination(a.modifiers, a.key);
      break;

    case ACTION_TEXT:
      sendTextToPC(a.text);
      break;

    case ACTION_DELAY:
      delay(a.delayMs);
      break;

    default:
      break;
  }
}

void executeMacro(int index) {
  if (index < 0 || index >= macroCount) return;
  if (!macros[index].enabled) return;
  if (macros[index].actionCount == 0) return;

  unsigned long now = millis();
  if (now - lastMacroRunMs < 250) return;
  lastMacroRunMs = now;

  for (uint8_t i = 0; i < macros[index].actionCount; ++i) {
    executeAction(macros[index].actions[i]);
  }

  releaseAllKeyboard();
}


// ============================================================
// Macro operations
// ============================================================

void addEmptyMacro() {
  if (macroCount >= MAX_MACROS) return;

  clearMacro(macros[macroCount]);
  macros[macroCount].name = String("Macro ") + String(macroCount + 1);
  macros[macroCount].enabled = true;
  macros[macroCount].triggerKey = 0;
  macros[macroCount].triggerModifiers = 0;
  macros[macroCount].actionCount = 0;

  macroCount++;
  selectedMacro = macroCount - 1;
  selectedAction = 0;
  macroPage = selectedMacro / MACROS_PER_PAGE;
  saveAll();
}

void deleteSelectedMacro() {
  if (macroCount == 0) return;
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;

  for (int i = selectedMacro; i < macroCount - 1; ++i) {
    macros[i] = macros[i + 1];
  }

  clearMacro(macros[macroCount - 1]);
  macroCount--;

  if (macroCount == 0) {
    selectedMacro = 0;
    selectedAction = 0;
    macroPage = 0;
  } else {
    if (selectedMacro >= macroCount) selectedMacro = macroCount - 1;
    selectedAction = 0;
    macroPage = selectedMacro / MACROS_PER_PAGE;
  }

  saveAll();
}

void moveSelectedMacro(int delta) {
  if (macroCount < 2) return;
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;

  int target = selectedMacro + delta;
  if (target < 0 || target >= macroCount) return;

  Macro tmp = macros[selectedMacro];
  macros[selectedMacro] = macros[target];
  macros[target] = tmp;
  selectedMacro = target;
  selectedAction = 0;
  macroPage = selectedMacro / MACROS_PER_PAGE;
  saveAll();
}

void deleteSelectedAction() {
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;

  Macro &m = macros[selectedMacro];
  if (m.actionCount == 0) return;
  if (selectedAction < 0 || selectedAction >= m.actionCount) return;

  for (int i = selectedAction; i < m.actionCount - 1; ++i) {
    m.actions[i] = m.actions[i + 1];
  }

  clearAction(m.actions[m.actionCount - 1]);
  m.actionCount--;

  if (m.actionCount == 0) {
    selectedAction = 0;
  } else if (selectedAction >= m.actionCount) {
    selectedAction = m.actionCount - 1;
  }

  saveAll();
}

void moveSelectedAction(int delta) {
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;

  Macro &m = macros[selectedMacro];
  if (m.actionCount < 2) return;

  int target = selectedAction + delta;
  if (target < 0 || target >= m.actionCount) return;

  MacroAction temp = m.actions[selectedAction];
  m.actions[selectedAction] = m.actions[target];
  m.actions[target] = temp;
  selectedAction = target;

  saveAll();
}

void addAction(uint8_t type) {
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;

  Macro &m = macros[selectedMacro];
  if (m.actionCount >= MAX_ACTIONS) return;

  MacroAction &a = m.actions[m.actionCount];
  clearAction(a);
  a.type = type;

  if (type == ACTION_DELAY) a.delayMs = 500;

  selectedAction = m.actionCount;
  m.actionCount++;
  editCreatedNewAction = true;

  if (type == ACTION_KEY || type == ACTION_COMBO) {
    captureComboMode = (type == ACTION_COMBO);
    editActionIndex = selectedAction;
    screen = SCREEN_CAPTURE_KEY;
    return;
  }

  if (type == ACTION_TEXT) {
    editActionIndex = selectedAction;
    editBuffer = "";
    screen = SCREEN_TEXT;
    return;
  }

  if (type == ACTION_DELAY) {
    editActionIndex = selectedAction;
    delayEditValue = 500;
    screen = SCREEN_DELAY;
  }
}

void beginEditAction() {
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;
  editCreatedNewAction = false;

  Macro &m = macros[selectedMacro];
  if (selectedAction < 0 || selectedAction >= m.actionCount) return;

  MacroAction &a = m.actions[selectedAction];
  editActionIndex = selectedAction;

  switch (a.type) {
    case ACTION_KEY:
      captureComboMode = false;
      screen = SCREEN_CAPTURE_KEY;
      break;

    case ACTION_COMBO:
      captureComboMode = true;
      screen = SCREEN_CAPTURE_KEY;
      break;

    case ACTION_TEXT:
      editBuffer = a.text;
      screen = SCREEN_TEXT;
      break;

    case ACTION_DELAY:
      delayEditValue = a.delayMs;
      screen = SCREEN_DELAY;
      break;

    default:
      break;
  }
}

void beginRename() {
  if (selectedMacro < 0 || selectedMacro >= macroCount) return;
  editBuffer = macros[selectedMacro].name;
  screen = SCREEN_RENAME;
}

void beginTriggerCapture() {
  screen = SCREEN_CAPTURE_TRIGGER;
}

// ============================================================
// Macro editor / picker helpers
// ============================================================

const int MACRO_MENU_COUNT = 10;

void openMacroEditor() {
  if (macroCount == 0) return;
  if (selectedMacro < 0) selectedMacro = 0;
  if (selectedMacro >= macroCount) selectedMacro = macroCount - 1;
  macroPage = selectedMacro / MACROS_PER_PAGE;
  macroMenuSelection = 0;
  selectedAction = 0;
  screen = SCREEN_MACRO_MENU;
}

void openMacroPicker() {
  macroPickerBuffer = "";
  macroPickerMessage = "Type 1-30 and press ENTER";
  screen = SCREEN_MACRO_PICKER;
}

void closeMacroPicker() {
  macroPickerBuffer = "";
  macroPickerMessage = "";
  screen = SCREEN_MACROS;
}

void moveMacroMenu(int delta) {
  macroMenuSelection += delta;
  if (macroMenuSelection < 0) macroMenuSelection = MACRO_MENU_COUNT - 1;
  if (macroMenuSelection >= MACRO_MENU_COUNT) macroMenuSelection = 0;
}

void moveMacroPickerBy(int delta) {
  if (macroCount == 0) return;
  selectedMacro += delta;
  while (selectedMacro < 0) selectedMacro += macroCount;
  while (selectedMacro >= macroCount) selectedMacro -= macroCount;
  macroPickerBuffer = "";
  macroPickerMessage = String("Selected ") + String(selectedMacro + 1) + "/" + String(macroCount);
}

bool pickerNumberValid(int &numberOut) {
  if (macroPickerBuffer.length() == 0) return false;
  int n = macroPickerBuffer.toInt();
  if (n < 1 || n > macroCount) return false;
  numberOut = n;
  return true;
}

void acceptMacroPicker() {
  if (macroCount == 0) {
    closeMacroPicker();
    return;
  }

  if (macroPickerBuffer.length() > 0) {
    int n = 0;
    if (pickerNumberValid(n)) {
      selectedMacro = n - 1;
      selectedAction = 0;
      macroPage = selectedMacro / MACROS_PER_PAGE;
      closeMacroPicker();
      return;
    }
    macroPickerMessage = String("Valid: 1-") + String(macroCount);
    redraw();
    return;
  }

  closeMacroPicker();
}

// ============================================================
// UI helpers
// ============================================================
// Forward declaration for keyboard pass-through used by the main menu.
void sendKeyboardPassThrough(const Keyboard_Class::KeysState &s);


bool shortcutWord(const Keyboard_Class::KeysState &s,
                  bool ctrl,
                  bool alt,
                  bool shift,
                  char wanted) {
  if (s.fn) return false;
  if (s.ctrl != ctrl) return false;
  if (s.alt != alt) return false;
  if (s.shift != shift) return false;
  if (!wordAvailable(s)) return false;
  return lowerAscii(firstWordChar(s)) == lowerAscii(wanted);
}

bool shortcutKey(const Keyboard_Class::KeysState &s,
                 bool ctrl,
                 bool alt,
                 bool shift,
                 uint8_t key) {
  if (s.fn) return false;
  if (s.ctrl != ctrl) return false;
  if (s.alt != alt) return false;
  if (s.shift != shift) return false;
  return hasHIDKey(s, key);
}

bool shortcutKeyWithFn(const Keyboard_Class::KeysState &s,
                       bool ctrl,
                       bool alt,
                       bool shift,
                       uint8_t key) {
  if (!s.fn) return false;
  if (s.ctrl != ctrl) return false;
  if (s.alt != alt) return false;
  if (s.shift != shift) return false;
  return hasHIDKey(s, key);
}

// Fn shortcuts need to work across M5Cardputer library variants.
// Some versions expose the base key through word/hid_keys, while current
// M5Stack firmware maps Fn+letters through the fn layer and exposes the
// pressed physical key through Keyboard.isKeyPressed(char).
bool fnWordShortcut(const Keyboard_Class::KeysState &s, char wanted) {
  if (!s.fn || s.ctrl || s.alt || s.shift) return false;

  if (wordAvailable(s) && lowerAscii(firstWordChar(s)) == lowerAscii(wanted)) {
    return true;
  }

  uint8_t wantedHid = 0;
  switch (lowerAscii(wanted)) {
    case 'm': wantedHid = HID_M; break;
    case 'n': wantedHid = HID_N; break;
    default: break;
  }

  if (wantedHid != 0 && hasHIDKey(s, wantedHid)) {
    return true;
  }

  return M5Cardputer.Keyboard.isKeyPressed(wanted);
}

bool fnPunctuationShortcut(const Keyboard_Class::KeysState &s, char wanted, uint8_t hid) {
  if (!s.fn || s.ctrl || s.alt || s.shift) return false;

  // User's older library may expose the base key directly.
  if (wordAvailable(s) && firstWordChar(s) == wanted) return true;
  if (hasHIDKey(s, hid)) return true;

  // Current M5Cardputer fn-layer maps: Fn+; => UP, Fn+. => DOWN.
  if (wanted == ';' && isUpPressed(s)) return true;
  if (wanted == '.' && isDownPressed(s)) return true;

  // Fallback for libraries that expose the physical base key via isKeyPressed().
  return M5Cardputer.Keyboard.isKeyPressed(wanted);
}

void drawHeader(const String &title) {
  canvas.fillScreen(TFT_BLACK);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(4, 2);
  canvas.print(title);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawFastHLine(0, 14, 240, TFT_DARKGREY);
}

void drawFooter(const String &text) {
  canvas.drawFastHLine(0, 117, 240, TFT_DARKGREY);
  canvas.setCursor(3, 121);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.print(text.substring(0, 39));
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawListCursor(int y, bool selected, int height = 14) {
  if (selected) {
    canvas.fillRect(0, y - 1, 240, height, TFT_DARKCYAN);
  }
}

String triggerSummary(const Macro &m) {
  if (m.triggerKey == 0) return "none";
  String s = modifiersName(m.triggerModifiers);
  s += hidKeyName(m.triggerKey);
  return s;
}

// ============================================================
// Main macro pages
// ============================================================

int macroPageCount() {
  if (macroCount == 0) return 1;
  return (int(macroCount) + MACROS_PER_PAGE - 1) / MACROS_PER_PAGE;
}

void syncMacroPage() {
  if (macroCount == 0) {
    macroPage = 0;
    selectedMacro = 0;
    return;
  }

  int pages = macroPageCount();
  if (macroPage < 0) macroPage = 0;
  if (macroPage >= pages) macroPage = pages - 1;

  int pageFirst = macroPage * MACROS_PER_PAGE;
  int pageLast = min(pageFirst + MACROS_PER_PAGE - 1, int(macroCount) - 1);

  if (selectedMacro < pageFirst) selectedMacro = pageFirst;
  if (selectedMacro > pageLast) selectedMacro = pageLast;
}

void setMacroPage(int page, bool selectFirst) {
  int pages = macroPageCount();
  if (page < 0) page = pages - 1;
  if (page >= pages) page = 0;
  macroPage = page;

  if (macroCount > 0) {
    int first = macroPage * MACROS_PER_PAGE;
    int last = min(first + MACROS_PER_PAGE - 1, int(macroCount) - 1);
    if (selectFirst || selectedMacro < first || selectedMacro > last) {
      selectedMacro = first;
    }
  }
}

void nextMacroPage() {
  setMacroPage(macroPage + 1, true);
}

void previousMacroPage() {
  setMacroPage(macroPage - 1, true);
}

void moveMacroSelection(int delta) {
  if (macroCount == 0) return;

  int next = selectedMacro + delta;

  if (next < 0) {
    setMacroPage(macroPageCount() - 1, true);
    selectedMacro = macroCount - 1;
    macroPage = selectedMacro / MACROS_PER_PAGE;
    return;
  }

  if (next >= macroCount) {
    setMacroPage(0, true);
    selectedMacro = 0;
    macroPage = 0;
    return;
  }

  selectedMacro = next;
  macroPage = selectedMacro / MACROS_PER_PAGE;
}


void selectMacroSlot(int slot) {
  if (slot < 0 || slot >= MACROS_PER_PAGE) return;
  int index = macroPage * MACROS_PER_PAGE + slot;
  if (index >= macroCount) return;
  selectedMacro = index;
}

void drawMacroFooter() {
  canvas.drawFastHLine(0, 109, 240, TFT_DARKGREY);
  canvas.setCursor(3, 111);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.print("UP/DN Select  SPACE Run  ENTER Edit");
  canvas.setCursor(3, 123);
  canvas.print("0-9 Slot  TAB Page  Fn+N New  Fn+M Mode");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ============================================================
// Main macro list
// ============================================================

void drawMacroList() {
  syncMacroPage();

  int pages = macroPageCount();
  int first = macroPage * MACROS_PER_PAGE;
  int last = min(first + MACROS_PER_PAGE - 1, int(macroCount) - 1);

  String title = keyboardMode ? "USB KEYBOARD" : "MACROS PAGE ";
  if (!keyboardMode) {
    title += String(macroPage + 1);
    title += "/";
    title += String(pages);
    title += "  [";
    title += String(first);
    title += "-";
    title += String(max(first, last));
    title += "]";
  }
  drawHeader(title);

  if (macroCount == 0) {
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setCursor(5, 35);
    canvas.print("No macros");
    canvas.setCursor(5, 52);
    canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas.print("Fn+N create new macro");
    canvas.setCursor(5, 68);
    canvas.print("0-9 select slot on page");
    canvas.setCursor(5, 84);
    canvas.print("TAB / Shift+TAB change page");
    drawMacroFooter();
    canvas.pushSprite(0, 0);
    return;
  }

  for (int row = 0; row < MACROS_PER_PAGE; ++row) {
    int idx = first + row;
    int y = 17 + row * 9;
    if (idx >= macroCount) {
      canvas.setCursor(3, y);
      canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
      canvas.print(".");
      continue;
    }

    bool selected = idx == selectedMacro;
    drawListCursor(y, selected, 9);

    canvas.setCursor(3, y);
    canvas.setTextColor(selected ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(selected ? ">" : " ");
    canvas.print(String(idx));
    canvas.print(" ");

    String name = macros[idx].name;
    if (name.length() > 18) name = name.substring(0, 18);
    canvas.print(name);

    canvas.setCursor(216, y);
    canvas.print(macros[idx].enabled ? "ON" : "--");
  }

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(145, 101);
  canvas.print("TRG:");
  canvas.print(triggerSummary(macros[selectedMacro]).substring(0, 14));
  drawMacroFooter();
  canvas.pushSprite(0, 0);
}


// ============================================================
// Macro number picker
// ============================================================

void drawMacroPicker() {
  drawHeader("SELECT MACRO");

  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(5, 22);
  canvas.print("Enter macro number:");

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.setCursor(8, 40);
  canvas.print(macroPickerBuffer.length() ? macroPickerBuffer : String(selectedMacro + 1));
  canvas.setTextSize(1);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(5, 70);
  canvas.print("Current: ");
  canvas.print(selectedMacro + 1);
  canvas.print("/");
  canvas.print(macroCount);

  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.setCursor(5, 87);
  canvas.print("UP/DN changes current");

  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.setCursor(5, 103);
  canvas.print(macroPickerMessage.substring(0, 38));

  drawFooter("0-9 choose slot  TAB page  ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Macro editor menu
// ============================================================

void drawMacroMenu() {
  if (macroCount == 0 || selectedMacro < 0 || selectedMacro >= macroCount) {
    screen = SCREEN_MACROS;
    drawMacroList();
    return;
  }

  Macro &m = macros[selectedMacro];
  String title = "EDIT MACRO ";
  title += String(selectedMacro + 1);
  title += "/";
  title += String(macroCount);
  drawHeader(title);

  canvas.setCursor(4, 17);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.print(m.name.substring(0, 28));

  canvas.setCursor(4, 30);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.print("Trigger: ");
  canvas.print(triggerSummary(m).substring(0, 25));

  const char *items[] = {
    "Actions",
    "Set trigger",
    "Rename",
    "Enabled",
    "Run macro",
    "Move macro up",
    "Move macro down",
    "Delete macro",
    "Save",
    "Back"
  };

  const int firstY = 43;
  const int rowH = 11;
  const int rows = 7;
  int start = macroMenuSelection - 3;
  if (start < 0) start = 0;
  if (start > MACRO_MENU_COUNT - rows) start = MACRO_MENU_COUNT - rows;
  if (start < 0) start = 0;

  for (int row = 0; row < rows; ++row) {
    int idx = start + row;
    if (idx >= MACRO_MENU_COUNT) break;

    int y = firstY + row * rowH;
    bool sel = idx == macroMenuSelection;
    drawListCursor(y, sel, rowH);

    canvas.setCursor(4, y);
    canvas.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(sel ? "> " : "  ");
    canvas.print(items[idx]);

    if (idx == 3) {
      canvas.setCursor(200, y);
      canvas.print(m.enabled ? "ON" : "OFF");
    } else if (idx == 0) {
      canvas.setCursor(205, y);
      canvas.print(m.actionCount);
    }
  }

  drawFooter("UP/DN Select  ENTER Open  ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Action list
// ============================================================

void drawActionList() {
  Macro &m = macros[selectedMacro];
  String title = "ACTIONS ";
  title += String(selectedMacro + 1);
  title += ": ";
  title += m.name.substring(0, 18);
  drawHeader(title);

  canvas.setCursor(3, 17);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.print("Trigger: ");
  canvas.print(triggerSummary(m).substring(0, 25));

  // selectedAction == actionCount means the synthetic "Add action..." row.
  const int totalRows = int(m.actionCount) + 1;
  int rows = 7;
  int startIndex = selectedAction - 3;
  if (startIndex < 0) startIndex = 0;
  if (startIndex > totalRows - rows) startIndex = max(0, totalRows - rows);

  for (int row = 0; row < rows && startIndex + row < totalRows; ++row) {
    int idx = startIndex + row;
    int y = 31 + row * 12;
    bool selected = idx == selectedAction;
    drawListCursor(y, selected, 12);

    canvas.setCursor(3, y);
    canvas.setTextColor(selected ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(selected ? ">" : " ");

    if (idx < m.actionCount) {
      canvas.print(String(idx + 1));
      canvas.print(" ");
      String summary = actionSummary(m.actions[idx]);
      if (summary.length() > 30) summary = summary.substring(0, 30);
      canvas.print(summary);
    } else {
      canvas.print("+");
      canvas.print(" Add action...");
    }
  }

  drawFooter("UP/DN Select  ENTER Open  ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Selected action context menu
// ============================================================

void drawActionMenu() {
  if (selectedMacro < 0 || selectedMacro >= macroCount) {
    screen = SCREEN_MACROS;
    drawMacroList();
    return;
  }

  Macro &m = macros[selectedMacro];
  if (m.actionCount == 0 || selectedAction < 0 || selectedAction >= m.actionCount) {
    screen = SCREEN_ACTIONS;
    drawActionList();
    return;
  }

  String title = "EDIT ACTION ";
  title += String(selectedAction + 1);
  title += "/";
  title += String(m.actionCount);
  drawHeader(title);

  canvas.setCursor(4, 17);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  String summary = actionSummary(m.actions[selectedAction]);
  if (summary.length() > 35) summary = summary.substring(0, 35);
  canvas.print(summary);

  const char *items[] = {
    "Edit action",
    "Move up",
    "Move down",
    "Delete action",
    "Add action",
    "Back"
  };
  const int itemCount = 6;

  for (int i = 0; i < itemCount; ++i) {
    int y = 35 + i * 13;
    bool selected = (i == actionMenuSelection);
    drawListCursor(y, selected, 12);
    canvas.setCursor(6, y);
    canvas.setTextColor(selected ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(selected ? "> " : "  ");
    canvas.print(items[i]);

    if (i == 1 && selectedAction == 0) {
      canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
      canvas.setCursor(205, y);
      canvas.print("-");
    }
    if (i == 2 && selectedAction >= m.actionCount - 1) {
      canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
      canvas.setCursor(205, y);
      canvas.print("-");
    }
  }

  drawFooter("UP/DN Select  ENTER Open  ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Action type menu - fully navigable
// ============================================================

void drawActionType() {
  drawHeader("ADD ACTION");

  const char *items[] = {
    "KEY",
    "COMBINATION",
    "TEXT",
    "DELAY"
  };

  for (int i = 0; i < 4; ++i) {
    int y = 25 + i * 18;
    drawListCursor(y, i == actionTypeSelection, 17);
    canvas.setCursor(8, y);
    canvas.setTextColor(i == actionTypeSelection ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(i == actionTypeSelection ? "> " : "  ");
    canvas.print(items[i]);
  }

  drawFooter("UP/DN Select Enter Choose ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Capture screens
// ============================================================

void drawCaptureKey() {
  drawHeader(captureComboMode ? "CAPTURE COMBO" : "CAPTURE KEY");
  canvas.setCursor(7, 30);
  canvas.print("Press key on Cardputer");
  canvas.setCursor(7, 47);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.print("Ctrl / Shift / Alt / Fn supported");
  canvas.setCursor(7, 64);
  canvas.print("Fn is stored as Win/GUI");
  canvas.setCursor(7, 84);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.print("ESC = cancel");
  canvas.setCursor(7, 98);
  canvas.print("Ctrl+Alt+Backspace = cancel");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter("Capture a real PC key or combination");
  canvas.pushSprite(0, 0);
}

void drawCaptureTrigger() {
  drawHeader("TRIGGER KEY");
  canvas.setCursor(7, 31);
  canvas.print("Press trigger key/combo");
  canvas.setCursor(7, 49);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.print("Example: Fn+F1 / Ctrl+Alt+T");
  canvas.setCursor(7, 68);
  canvas.print("The same key later runs macro");
  canvas.setCursor(7, 88);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.print("ESC = cancel");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter("Press one key or one combination");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Text / delay / rename editors
// ============================================================

void drawTextEditor(const String &title) {
  drawHeader(title);
  canvas.setCursor(4, 23);
  canvas.print("Text:");

  String visible = editBuffer;
  if (visible.length() > 31) visible = visible.substring(visible.length() - 31);

  canvas.setCursor(4, 45);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.print(visible);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  canvas.setCursor(4, 70);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.print("Backspace = remove");
  canvas.setCursor(4, 86);
  canvas.print("Enter = save   ESC = cancel");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  String footer = "Len ";
  footer += String(editBuffer.length());
  footer += "/";
  footer += String(title == "RENAME MACRO" ? MAX_NAME_LEN : MAX_TEXT_LEN);
  drawFooter(footer);
  canvas.pushSprite(0, 0);
}

void drawDelayEditor() {
  drawHeader("EDIT DELAY");
  canvas.setCursor(6, 28);
  canvas.print("Milliseconds:");
  canvas.setCursor(6, 51);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.print(delayEditValue);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(6, 78);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.print("Digits edit the value");
  canvas.setCursor(6, 93);
  canvas.print("Backspace removes one digit");
  drawFooter("Enter Save  ESC Cancel  0..65535 ms");
  canvas.pushSprite(0, 0);
}

void drawRename() {
  drawTextEditor("RENAME MACRO");
}

// ============================================================
// Settings menu
// ============================================================

void drawSettings() {
  drawHeader("SETTINGS");

  const char *items[] = {
    "Keyboard mode",
    "Macro enabled",
    "Save to Flash",
    "Restore defaults",
    "Help"
  };

  for (int i = 0; i < 5; ++i) {
    int y = 21 + i * 18;
    drawListCursor(y, i == settingsSelection, 17);
    canvas.setCursor(6, y);
    canvas.setTextColor(i == settingsSelection ? TFT_WHITE : TFT_LIGHTGREY, TFT_BLACK);
    canvas.print(i == settingsSelection ? "> " : "  ");
    canvas.print(items[i]);

    if (i == 0) {
      canvas.setCursor(190, y);
      canvas.print(keyboardMode ? "ON" : "OFF");
    } else if (i == 1 && macroCount > 0) {
      canvas.setCursor(190, y);
      canvas.print(macros[selectedMacro].enabled ? "ON" : "OFF");
    }
  }

  drawFooter("↑↓ Select  Enter Action  ESC Back");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Confirmation dialog
// ============================================================

void drawConfirm() {
  drawHeader("CONFIRM");

  String question;
  if (confirmAction == CONFIRM_DELETE_MACRO) {
    question = "Delete selected macro?";
  } else if (confirmAction == CONFIRM_RESTORE_DEFAULTS) {
    question = "Restore 30 defaults?";
  } else {
    question = "Are you sure?";
  }

  canvas.setCursor(8, 30);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.print(question);

  const char *yes = "YES";
  const char *no  = "NO";

  drawListCursor(56, confirmYes, 22);
  drawListCursor(82, !confirmYes, 22);

  canvas.setCursor(24, 61);
  canvas.print(confirmYes ? "> " : "  ");
  canvas.print(yes);

  canvas.setCursor(24, 87);
  canvas.print(!confirmYes ? "> " : "  ");
  canvas.print(no);

  drawFooter("UP/DN or LT/RT Select Enter OK ESC");
  canvas.pushSprite(0, 0);
}

// ============================================================
// Help screen
// ============================================================

void drawHelp() {
  drawHeader(helpPage == 0 ? "HELP 1/2" : "HELP 2/2");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  if (helpPage == 0) {
    canvas.setCursor(4, 21);  canvas.print("MAIN MENU");
    canvas.setCursor(4, 35);  canvas.print("UP/DOWN   select macro");
    canvas.setCursor(4, 48);  canvas.print("0-9       select slot");
    canvas.setCursor(4, 61);  canvas.print("SPACE     run selected");
    canvas.setCursor(4, 74);  canvas.print("ENTER     edit selected");
    canvas.setCursor(4, 87);  canvas.print("TAB       next page");
    canvas.setCursor(4, 100); canvas.print("FN+; / .  prev / next");
    canvas.setCursor(4, 113); canvas.print("FN+N/M    new / macro-mode");
  } else {
    canvas.setCursor(4, 21);  canvas.print("EDITOR");
    canvas.setCursor(4, 35);  canvas.print("UP/DOWN   navigate");
    canvas.setCursor(4, 48);  canvas.print("ENTER     select");
    canvas.setCursor(4, 61);  canvas.print("ESC       back / cancel");
    canvas.setCursor(4, 74);  canvas.print("CTRL+N    add action");
    canvas.setCursor(4, 87);  canvas.print("CTRL+D    delete action");
    canvas.setCursor(4, 100); canvas.print("CTRL+UP/DN move action");
    canvas.setCursor(4, 113); canvas.print("SPACE     run selected");
  }

  drawFooter("UP/DN or LT/RT page  ESC Back");
  canvas.pushSprite(0, 0);
}

void redraw() {
  switch (screen) {
    case SCREEN_MACROS:          drawMacroList(); break;
    case SCREEN_MACRO_PICKER:    drawMacroPicker(); break;
    case SCREEN_MACRO_MENU:      drawMacroMenu(); break;
    case SCREEN_ACTIONS:         drawActionList(); break;
    case SCREEN_ACTION_MENU:     drawActionMenu(); break;
    case SCREEN_ACTION_TYPE:     drawActionType(); break;
    case SCREEN_CAPTURE_KEY:     drawCaptureKey(); break;
    case SCREEN_CAPTURE_TRIGGER: drawCaptureTrigger(); break;
    case SCREEN_TEXT:            drawTextEditor("EDIT TEXT"); break;
    case SCREEN_DELAY:           drawDelayEditor(); break;
    case SCREEN_RENAME:          drawRename(); break;
    case SCREEN_SETTINGS:        drawSettings(); break;
    case SCREEN_CONFIRM:         drawConfirm(); break;
    case SCREEN_HELP:            drawHelp(); break;
  }

  lastRedrawMs = millis();
}

// ============================================================
// Confirmation helpers
// ============================================================

void openConfirm(ConfirmAction action) {
  confirmAction = action;
  confirmYes = false;
  screen = SCREEN_CONFIRM;
  redraw();
}

void finishConfirm(bool accept) {
  if (accept) {
    if (confirmAction == CONFIRM_DELETE_MACRO) {
      deleteSelectedMacro();
      screen = SCREEN_MACROS;
    } else if (confirmAction == CONFIRM_RESTORE_DEFAULTS) {
      createDefaults();
      selectedMacro = 0;
      selectedAction = 0;
      macroPage = 0;
      screen = SCREEN_MACROS;
    }
  } else {
    if (confirmAction == CONFIRM_DELETE_MACRO) {
      screen = SCREEN_MACRO_MENU;
      macroMenuSelection = 7;
    } else {
      screen = SCREEN_SETTINGS;
    }
  }

  confirmAction = CONFIRM_NONE;
  redraw();
}

// ============================================================
// Trigger detection
// ============================================================

bool stateMatchesTrigger(const Keyboard_Class::KeysState &s,
                         const Macro &m) {
  if (m.triggerKey == 0) return false;

  uint8_t key = 0;
  uint8_t mods = 0;
  if (!extractKeyCombo(s, key, mods)) return false;

  return key == m.triggerKey && mods == m.triggerModifiers;
}

bool checkMacroTrigger(const Keyboard_Class::KeysState &s) {
  if (screen != SCREEN_MACROS || keyboardMode) return false;

  for (int i = 0; i < macroCount; ++i) {
    if (!macros[i].enabled) continue;
    if (stateMatchesTrigger(s, macros[i])) {
      executeMacro(i);
      selectedMacro = i;
      return true;
    }
  }
  return false;
}

// ============================================================
// Text editor
// ============================================================

void cancelCurrentActionEdit() {
  if (editCreatedNewAction) {
    if (selectedMacro >= 0 && selectedMacro < macroCount) {
      Macro &m = macros[selectedMacro];
      if (selectedAction >= 0 && selectedAction < m.actionCount) {
        for (int i = selectedAction; i < m.actionCount - 1; ++i) {
          m.actions[i] = m.actions[i + 1];
        }
        clearAction(m.actions[m.actionCount - 1]);
        m.actionCount--;
        if (selectedAction >= m.actionCount) selectedAction = max(0, int(m.actionCount) - 1);
      }
    }
    editCreatedNewAction = false;
  }
  editActionIndex = -1;
  saveAll();
}

void handleTextEditor(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    if (screen == SCREEN_RENAME) {
      screen = SCREEN_MACROS;
    } else {
      cancelCurrentActionEdit();
      screen = SCREEN_ACTIONS;
    }
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    if (screen == SCREEN_RENAME) {
      if (editBuffer.length() == 0) editBuffer = "Macro";
      editBuffer = editBuffer.substring(0, MAX_NAME_LEN);
      macros[selectedMacro].name = editBuffer;
      saveAll();
      screen = SCREEN_MACROS;
    } else {
      if (editActionIndex >= 0 && editActionIndex < macros[selectedMacro].actionCount) {
        macros[selectedMacro].actions[editActionIndex].text = editBuffer.substring(0, MAX_TEXT_LEN);
        saveAll();
      }
      editCreatedNewAction = false;
      editActionIndex = -1;
      screen = SCREEN_ACTIONS;
    }

    redraw();
    return;
  }

  if (isBackspacePressed(s)) {
    if (editBuffer.length() > 0) editBuffer.remove(editBuffer.length() - 1);
    redraw();
    return;
  }

  if (!wordAvailable(s)) return;

  String incoming = wordToString(s);
  size_t limit = screen == SCREEN_RENAME ? MAX_NAME_LEN : MAX_TEXT_LEN;

  for (size_t i = 0; i < incoming.length(); ++i) {
    char c = incoming[i];
    if (c >= 32 && c <= 126 && editBuffer.length() < limit) editBuffer += c;
  }

  redraw();
}

// ============================================================
// Delay editor
// ============================================================

void handleDelayEditor(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    if (editCreatedNewAction) cancelCurrentActionEdit();
    else editActionIndex = -1;
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    if (editActionIndex >= 0 && editActionIndex < macros[selectedMacro].actionCount) {
      macros[selectedMacro].actions[editActionIndex].delayMs = delayEditValue;
      saveAll();
    }
    editCreatedNewAction = false;
    editActionIndex = -1;
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  if (isBackspacePressed(s)) {
    delayEditValue /= 10;
    redraw();
    return;
  }

  if (!wordAvailable(s)) return;

  for (size_t i = 0; i < s.word.size(); ++i) {
    char c = s.word[i];
    if (c >= '0' && c <= '9') {
      uint32_t next = uint32_t(delayEditValue) * 10UL + uint32_t(c - '0');
      if (next > 65535UL) next = 65535UL;
      delayEditValue = uint16_t(next);
    }
  }

  redraw();
}

// ============================================================
// Capture handlers
// ============================================================

void handleKeyCapture(const Keyboard_Class::KeysState &s) {
  if (isCancelCapture(s) || isEscPressed(s)) {
    if (editCreatedNewAction) cancelCurrentActionEdit();
    else editActionIndex = -1;
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  uint8_t key = 0;
  uint8_t mods = 0;
  if (!extractKeyCombo(s, key, mods)) return;
  if (key == HID_ESC) return;

  if (editActionIndex < 0 || editActionIndex >= macros[selectedMacro].actionCount) {
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  MacroAction &a = macros[selectedMacro].actions[editActionIndex];
  a.key = key;
  a.modifiers = captureComboMode ? mods : 0;
  a.type = captureComboMode ? ACTION_COMBO : ACTION_KEY;
  a.delayMs = 0;
  a.text = "";

  editCreatedNewAction = false;
  saveAll();
  screen = SCREEN_ACTIONS;
  editActionIndex = -1;
  redraw();
}

void handleTriggerCapture(const Keyboard_Class::KeysState &s) {
  if (isCancelCapture(s) || isEscPressed(s)) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  uint8_t key = 0;
  uint8_t mods = 0;
  if (!extractKeyCombo(s, key, mods)) return;

  macros[selectedMacro].triggerKey = key;
  macros[selectedMacro].triggerModifiers = mods;
  saveAll();

  screen = SCREEN_MACROS;
  redraw();
}

// ============================================================
// Action type menu
// ============================================================

void handleActionType(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    actionTypeSelection--;
    if (actionTypeSelection < 0) actionTypeSelection = 3;
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    actionTypeSelection++;
    if (actionTypeSelection > 3) actionTypeSelection = 0;
    redraw();
    return;
  }

  // Quick Fn+1..4 shortcuts remain available, but are not required.
  if (shortcutKeyWithFn(s, false, false, false, HID_F1)) { actionTypeSelection = 0; addAction(ACTION_KEY); return; }
  if (shortcutKeyWithFn(s, false, false, false, HID_F2)) { actionTypeSelection = 1; addAction(ACTION_COMBO); return; }
  if (shortcutKeyWithFn(s, false, false, false, HID_F3)) { actionTypeSelection = 2; addAction(ACTION_TEXT); return; }
  if (shortcutKeyWithFn(s, false, false, false, HID_F4)) { actionTypeSelection = 3; addAction(ACTION_DELAY); return; }

  if (isEnterPressed(s)) {
    switch (actionTypeSelection) {
      case 0: addAction(ACTION_KEY); break;
      case 1: addAction(ACTION_COMBO); break;
      case 2: addAction(ACTION_TEXT); break;
      case 3: addAction(ACTION_DELAY); break;
    }
  }
}

// ============================================================
// Macro list handlers
// ============================================================

void handleMacroScreen(const Keyboard_Class::KeysState &s) {
  // ----------------------------------------------------------
  // Requested Fn shortcuts
  // ----------------------------------------------------------
  if (fnWordShortcut(s, 'm')) {
    keyboardMode = !keyboardMode;
    redraw();
    return;
  }

  if (fnWordShortcut(s, 'n')) {
    keyboardMode = false;
    uint8_t before = macroCount;
    addEmptyMacro();
    if (macroCount != before) {
      macroPage = selectedMacro / MACROS_PER_PAGE;
      beginRename();
    }
    redraw();
    return;
  }

  if (macroCount > 0 && fnPunctuationShortcut(s, ';', HID_SEMICOLON)) {
    keyboardMode = false;
    moveMacroSelection(-1);
    redraw();
    return;
  }

  if (macroCount > 0 && fnPunctuationShortcut(s, '.', HID_DOT)) {
    keyboardMode = false;
    moveMacroSelection(+1);
    redraw();
    return;
  }

  // Fn+[ / Fn+] and TAB / Shift+TAB switch pages.
  if (s.fn && !s.ctrl && !s.alt && !s.shift &&
      wordAvailable(s) && firstWordChar(s) == '[') {
    previousMacroPage();
    redraw();
    return;
  }

  if (s.fn && !s.ctrl && !s.alt && !s.shift &&
      wordAvailable(s) && firstWordChar(s) == ']') {
    nextMacroPage();
    redraw();
    return;
  }

  if (!s.fn && !s.ctrl && !s.alt && hasHIDKey(s, HID_TAB)) {
    if (s.shift) previousMacroPage();
    else nextMacroPage();
    redraw();
    return;
  }

  // In keyboard mode all normal keys are forwarded to the PC.
  if (keyboardMode) {
    if (isEscPressed(s)) {
      keyboardMode = false;
      redraw();
      return;
    }
    sendKeyboardPassThrough(s);
    return;
  }

  if (macroCount == 0) return;

  // Ctrl+Alt actions on the main macro screen.
  if (shortcutWord(s, true, true, false, 'd')) {
    openConfirm(CONFIRM_DELETE_MACRO);
    return;
  }

  if (shortcutWord(s, true, true, false, 'r')) {
    beginRename();
    redraw();
    return;
  }

  if (shortcutWord(s, true, true, false, 't')) {
    beginTriggerCapture();
    redraw();
    return;
  }

  if (shortcutWord(s, true, true, false, 's')) {
    settingsSelection = 0;
    screen = SCREEN_SETTINGS;
    redraw();
    return;
  }

  if (shortcutKey(s, true, true, false, HID_UP)) {
    moveSelectedMacro(-1);
    macroPage = selectedMacro / MACROS_PER_PAGE;
    redraw();
    return;
  }

  if (shortcutKey(s, true, true, false, HID_DOWN)) {
    moveSelectedMacro(+1);
    macroPage = selectedMacro / MACROS_PER_PAGE;
    redraw();
    return;
  }

  // SPACE executes the selected macro without opening the editor.
  if (hasHIDKey(s, HID_SPACE) || (wordAvailable(s) && firstWordChar(s) == ' ')) {
    executeMacro(selectedMacro);
    redraw();
    return;
  }

  // Ctrl+Enter also executes the selected macro.
  if (shortcutKey(s, true, false, false, HID_ENTER)) {
    executeMacro(selectedMacro);
    redraw();
    return;
  }

  // Page movement. Cardputer has no physical PageUp/PageDown, so TAB
  // is the primary page key (handled above). Keep HID page keys as a
  // fallback for external/modified keyboard mappings.
  if (hasHIDKey(s, HID_PAGEUP)) {
    previousMacroPage();
    redraw();
    return;
  }

  if (hasHIDKey(s, HID_PAGEDOWN)) {
    nextMacroPage();
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    moveMacroSelection(-1);
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    moveMacroSelection(+1);
    redraw();
    return;
  }

  // 0..9 selects the corresponding slot on the current page.
  // This is selection only; Ctrl+Enter runs the macro.
  if (!s.fn && !s.ctrl && !s.alt && wordAvailable(s)) {
    char c = firstWordChar(s);
    if (c >= '0' && c <= '9') {
      selectMacroSlot(c - '0');
      redraw();
      return;
    }
  }

  if (isEnterPressed(s)) {
    openMacroEditor();
    redraw();
    return;
  }
}


// ============================================================
// Macro picker handler
// ============================================================

void handleMacroPicker(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    closeMacroPicker();
    redraw();
    return;
  }

  if (fnPunctuationShortcut(s, ';', HID_SEMICOLON)) {
    moveMacroPickerBy(-1);
    redraw();
    return;
  }

  if (fnPunctuationShortcut(s, '.', HID_DOT)) {
    moveMacroPickerBy(+1);
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    moveMacroPickerBy(-1);
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    moveMacroPickerBy(+1);
    redraw();
    return;
  }

  if (hasHIDKey(s, HID_PAGEUP)) {
    moveMacroPickerBy(-5);
    redraw();
    return;
  }

  if (hasHIDKey(s, HID_PAGEDOWN)) {
    moveMacroPickerBy(+5);
    redraw();
    return;
  }

  if (isBackspacePressed(s)) {
    if (macroPickerBuffer.length() > 0) {
      macroPickerBuffer.remove(macroPickerBuffer.length() - 1);
      macroPickerMessage = "Type 1-30 and press ENTER";
    }
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    acceptMacroPicker();
    return;
  }

  if (wordAvailable(s) && !s.fn && !s.ctrl && !s.alt) {
    bool changed = false;
    for (size_t i = 0; i < s.word.size(); ++i) {
      char c = s.word[i];
      if (c >= '0' && c <= '9' && macroPickerBuffer.length() < 2) {
        macroPickerBuffer += c;
        changed = true;
      }
    }
    if (changed) {
      macroPickerMessage = String("Number: ") + macroPickerBuffer;
      redraw();
    }
  }
}

// ============================================================
// Macro editor menu handler
// ============================================================

void handleMacroMenu(const Keyboard_Class::KeysState &s) {
  if (selectedMacro < 0 || selectedMacro >= macroCount) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  if (isEscPressed(s)) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    moveMacroMenu(-1);
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    moveMacroMenu(+1);
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    switch (macroMenuSelection) {
      case 0: // Actions
        selectedAction = 0;
        screen = SCREEN_ACTIONS;
        break;

      case 1: // Trigger
        beginTriggerCapture();
        break;

      case 2: // Rename
        beginRename();
        break;

      case 3: // Enabled
        macros[selectedMacro].enabled = !macros[selectedMacro].enabled;
        saveAll();
        break;

      case 4: // Run
        executeMacro(selectedMacro);
        break;

      case 5: // Move up
        moveSelectedMacro(-1);
        break;

      case 6: // Move down
        moveSelectedMacro(+1);
        break;

      case 7: // Delete
        openConfirm(CONFIRM_DELETE_MACRO);
        return;

      case 8: // Save
        saveAll();
        break;

      case 9: // Back
        screen = SCREEN_MACROS;
        break;
    }
    redraw();
    return;
  }

  // Legacy quick shortcuts are retained inside the editor.
  if (shortcutWord(s, true, false, false, 'n')) {
    actionTypeSelection = 0;
    screen = SCREEN_ACTION_TYPE;
    redraw();
    return;
  }

  if (shortcutWord(s, true, false, false, 's')) {
    saveAll();
    redraw();
    return;
  }
}

// ============================================================
// Selected action context menu handler
// ============================================================

void handleActionMenu(const Keyboard_Class::KeysState &s) {
  if (selectedMacro < 0 || selectedMacro >= macroCount) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  Macro &m = macros[selectedMacro];
  if (m.actionCount == 0 || selectedAction < 0 || selectedAction >= m.actionCount) {
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  if (isEscPressed(s)) {
    screen = SCREEN_ACTIONS;
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    actionMenuSelection--;
    if (actionMenuSelection < 0) actionMenuSelection = 5;
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    actionMenuSelection++;
    if (actionMenuSelection > 5) actionMenuSelection = 0;
    redraw();
    return;
  }

  if (!isEnterPressed(s)) return;

  switch (actionMenuSelection) {
    case 0: // Edit action
      beginEditAction();
      redraw();
      return;

    case 1: // Move up
      if (selectedAction > 0) {
        moveSelectedAction(-1);
      }
      redraw();
      return;

    case 2: // Move down
      if (selectedAction < m.actionCount - 1) {
        moveSelectedAction(+1);
      }
      redraw();
      return;

    case 3: // Delete action
      deleteSelectedAction();
      screen = SCREEN_ACTIONS;
      if (selectedAction > m.actionCount) selectedAction = m.actionCount;
      redraw();
      return;

    case 4: // Add action
      actionTypeSelection = 0;
      screen = SCREEN_ACTION_TYPE;
      redraw();
      return;

    case 5: // Back
      screen = SCREEN_ACTIONS;
      redraw();
      return;
  }
}

// ============================================================
// Action list handlers
// ============================================================

void handleActionsScreen(const Keyboard_Class::KeysState &s) {
  if (selectedMacro < 0 || selectedMacro >= macroCount) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  Macro &m = macros[selectedMacro];
  int totalItems = int(m.actionCount) + 1; // + Add action...

  if (isEscPressed(s)) {
    screen = SCREEN_MACRO_MENU;
    macroMenuSelection = 0;
    redraw();
    return;
  }

  // Add action shortcut.
  if (shortcutWord(s, true, false, false, 'n') || hasHIDKey(s, HID_INSERT)) {
    actionTypeSelection = 0;
    screen = SCREEN_ACTION_TYPE;
    redraw();
    return;
  }

  // Quick action shortcuts remain available, but the same operations are
  // also available in the normal action context menu opened by ENTER.
  if ((shortcutWord(s, true, false, false, 'd') || hasHIDKey(s, HID_DELETE)) &&
      selectedAction < m.actionCount) {
    deleteSelectedAction();
    if (selectedAction > m.actionCount) selectedAction = m.actionCount;
    redraw();
    return;
  }

  if (shortcutKey(s, true, false, false, HID_UP) && selectedAction < m.actionCount) {
    moveSelectedAction(-1);
    redraw();
    return;
  }

  if (shortcutKey(s, true, false, false, HID_DOWN) && selectedAction < m.actionCount) {
    moveSelectedAction(+1);
    redraw();
    return;
  }

  if (shortcutWord(s, true, false, false, 's')) {
    saveAll();
    redraw();
    return;
  }

  if (shortcutWord(s, true, false, false, 't')) {
    beginTriggerCapture();
    return;
  }

  if (shortcutKey(s, true, false, false, HID_ENTER)) {
    executeMacro(selectedMacro);
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    selectedAction--;
    if (selectedAction < 0) selectedAction = totalItems - 1;
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    selectedAction++;
    if (selectedAction >= totalItems) selectedAction = 0;
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    if (selectedAction == m.actionCount) {
      actionTypeSelection = 0;
      screen = SCREEN_ACTION_TYPE;
      redraw();
      return;
    }

    if (selectedAction >= 0 && selectedAction < m.actionCount) {
      actionMenuSelection = 0;
      screen = SCREEN_ACTION_MENU;
      redraw();
      return;
    }
  }
}

// ============================================================
// Settings handlers
// ============================================================

void handleSettingsScreen(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    screen = SCREEN_MACROS;
    redraw();
    return;
  }

  if (isUpPressed(s)) {
    settingsSelection--;
    if (settingsSelection < 0) settingsSelection = 4;
    redraw();
    return;
  }

  if (isDownPressed(s)) {
    settingsSelection++;
    if (settingsSelection > 4) settingsSelection = 0;
    redraw();
    return;
  }

  if (!isEnterPressed(s)) return;

  switch (settingsSelection) {
    case 0:
      keyboardMode = !keyboardMode;
      redraw();
      break;

    case 1:
      if (macroCount > 0) {
        macros[selectedMacro].enabled = !macros[selectedMacro].enabled;
        saveAll();
      }
      redraw();
      break;

    case 2:
      saveAll();
      redraw();
      break;

    case 3:
      openConfirm(CONFIRM_RESTORE_DEFAULTS);
      break;

    case 4:
      helpPage = 0;
      screen = SCREEN_HELP;
      redraw();
      break;
  }
}

// ============================================================
// Confirmation handler
// ============================================================

void handleConfirmScreen(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    finishConfirm(false);
    return;
  }

  if (isUpPressed(s) || isDownPressed(s) ||
      isLeftPressed(s) || isRightPressed(s)) {
    confirmYes = !confirmYes;
    redraw();
    return;
  }

  if (isEnterPressed(s)) {
    finishConfirm(confirmYes);
    return;
  }
}

// ============================================================
// Help handler
// ============================================================

void handleHelpScreen(const Keyboard_Class::KeysState &s) {
  if (isEscPressed(s)) {
    screen = SCREEN_SETTINGS;
    redraw();
    return;
  }

  if (isUpPressed(s) || isLeftPressed(s)) {
    helpPage = 0;
    redraw();
    return;
  }

  if (isDownPressed(s) || isRightPressed(s)) {
    helpPage = 1;
    redraw();
    return;
  }
}

// ============================================================
// Keyboard pass-through
// ============================================================

void sendKeyboardPassThrough(const Keyboard_Class::KeysState &s) {
  if (!s.fn && !s.ctrl && !s.alt && s.hid_keys.empty() && !s.word.empty()) {
    String text = wordToString(s);
    if (text.length() > 0) {
      usbKeyboard.print(text);
      delay(5);
      return;
    }
  }

  uint8_t key = 0;
  uint8_t mods = 0;

  if (extractKeyCombo(s, key, mods)) {
    sendCombination(mods, key);
  }
}

// ============================================================
// Top-level keyboard handling
// ============================================================

void handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange()) return;
  if (!M5Cardputer.Keyboard.isPressed()) return;

  Keyboard_Class::KeysState s = M5Cardputer.Keyboard.keysState();

  // Trigger macros before normal menu handling. The trigger checker is
  // restricted to the main macro screen, so editor navigation remains
  // independent and reliable.
  if (checkMacroTrigger(s)) {
    redraw();
    return;
  }

  switch (screen) {
    case SCREEN_MACROS:
      handleMacroScreen(s);
      break;

    case SCREEN_MACRO_PICKER:
      handleMacroPicker(s);
      break;

    case SCREEN_MACRO_MENU:
      handleMacroMenu(s);
      break;

    case SCREEN_ACTIONS:
      handleActionsScreen(s);
      break;

    case SCREEN_ACTION_MENU:
      handleActionMenu(s);
      break;

    case SCREEN_ACTION_TYPE:
      handleActionType(s);
      break;

    case SCREEN_CAPTURE_KEY:
      handleKeyCapture(s);
      break;

    case SCREEN_CAPTURE_TRIGGER:
      handleTriggerCapture(s);
      break;

    case SCREEN_TEXT:
    case SCREEN_RENAME:
      handleTextEditor(s);
      break;

    case SCREEN_DELAY:
      handleDelayEditor(s);
      break;

    case SCREEN_SETTINGS:
      handleSettingsScreen(s);
      break;

    case SCREEN_CONFIRM:
      handleConfirmScreen(s);
      break;

    case SCREEN_HELP:
      handleHelpScreen(s);
      break;

    default:
      screen = SCREEN_MACROS;
      redraw();
      break;
  }
}

// ============================================================
// Setup / loop
// ============================================================

void setup() {
  auto cfg = M5.config();

  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setBrightness(100);
  M5Cardputer.Display.setTextSize(1);

  canvas.createSprite(
    M5Cardputer.Display.width(),
    M5Cardputer.Display.height()
  );

  USB.begin();
  usbKeyboard.begin();

  delay(300);

  if (!loadAll()) {
    createDefaults();
  } else if (legacyStorageV4) {
    // One-time upgrade: keep existing macros and append the new useful
    // diagnostic/admin library entries. Subsequent boots will not add
    // anything again, even if the user later deletes a macro.
    if (macroCount > 0 && macroCount < MAX_MACROS) {
      appendUsefulDefaults();
    }
    saveAll();
    legacyStorageV4 = false;
  }

  selectedMacro = 0;
  selectedAction = 0;
  macroPage = 0;
  actionTypeSelection = 0;
  settingsSelection = 0;
  macroMenuSelection = 0;
  macroPickerBuffer = "";
  macroPickerMessage = "";
  screen = SCREEN_MACROS;
  editActionIndex = -1;
  editCreatedNewAction = false;
  confirmAction = CONFIRM_NONE;

  redraw();
}

void loop() {
  M5Cardputer.update();
  handleKeyboard();

  if (millis() - lastRedrawMs > 5000) {
    redraw();
  }

  delay(2);
}
