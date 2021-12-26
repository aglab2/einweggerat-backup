#include "CLibretro.h"
#include "../../3rdparty-deps/libretro.h"
#include "stdafx.h"

#define MUDLIB_IMPLEMENTATION
#include "io/gl_render.h"
#include "mudlib.h"
#define INI_IMPLEMENTATION
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "../../3rdparty-deps/ini.h"

#define INLINE
using namespace std;

bool CLibretro::savestate(TCHAR *filename, bool save) {
  if (isEmulating) {
    size_t size = g_retro.retro_serialize_size();
    if (size) {
      auto Memory = std::make_unique<uint8_t[]>(size);
      if (save) {
        // Get the filesize
        g_retro.retro_serialize(Memory.get(), size);
        Mud_FileAccess::save_data(Memory.get(), size, filename);
      } else {
        unsigned sz;
        std::vector<BYTE> save_data = Mud_FileAccess::load_data(filename, &sz);
        if (save_data.empty())return false;
        memcpy(Memory.get(), (BYTE*)save_data.data(), size);
        g_retro.retro_unserialize(Memory.get(), size);
      }
      return true;
    }
  }
  return false;
}

bool CLibretro::savesram(TCHAR *filename, bool save) {
  if (isEmulating) {
    size_t size = g_retro.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (size) {
      BYTE *Memory = (BYTE *)g_retro.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
      if (save)
        return Mud_FileAccess::save_data(Memory, size, filename);
      else {
        unsigned sz;
        std::vector<BYTE> save_data = Mud_FileAccess::load_data(filename, &sz);
        if (save_data.empty())return false;
        memcpy(Memory, (BYTE*)save_data.data(), size);
        return true;
      }
    }
  }
  return false;
}

void CLibretro::reset() {
  if (isEmulating)
    g_retro.retro_reset();
}

void CLibretro::core_unload() {
  isEmulating = false;
  if (g_retro.initialized) {
    g_retro.retro_unload_game();
    g_retro.retro_deinit();
  }
  if (g_retro.handle) {
    FreeLibrary(g_retro.handle);
    g_retro.handle = NULL;
  }
  memset(&g_retro, 0, sizeof(g_retro));
}

bool CLibretro::core_load(TCHAR *sofile, bool gamespecificoptions,
                          TCHAR *game_filename) {
  tstring game_filename_;
  tstring einweg_dir;
  g_retro.handle = LoadLibrary(sofile);
  if (!g_retro.handle)
    return false;
#define die()                                                                  \
  do {                                                                         \
    FreeLibrary(g_retro.handle);                                               \
    return false;                                                              \
  } while (0)
#define libload(name) GetProcAddress(g_retro.handle, name)
#define load_sym(V, name)                                                      \
  if (!(*(void **)(&V) = (void *)libload(#name)))                              \
  die()
#define load_retro_sym(S) load_sym(g_retro.S, S)
  
  load_retro_sym(retro_init);
  load_retro_sym(retro_deinit);
  load_retro_sym(retro_api_version);
  load_retro_sym(retro_get_system_info);
  load_retro_sym(retro_get_system_av_info);
  load_retro_sym(retro_set_controller_port_device);
  load_retro_sym(retro_reset);
  load_retro_sym(retro_run);
  load_retro_sym(retro_load_game);
  load_retro_sym(retro_unload_game);
  load_retro_sym(retro_serialize);
  load_retro_sym(retro_unserialize);
  load_retro_sym(retro_serialize_size);
  load_retro_sym(retro_get_memory_size);
  load_retro_sym(retro_get_memory_data);
  

  game_filename_ = game_filename;
  //strip path
  game_filename_.erase(0, game_filename_.find_last_of(L"\\/") +1);
  //strip extension
  game_filename_.erase(game_filename_.rfind('.'));
  einweg_dir = Mud_Misc::ExePath() + L"\\system";
  sys_name = save_name = sram_name = einweg_dir;
  sram_name += L"\\" + game_filename_ + L".sram";
  core_config = sofile;
  core_config = core_config.erase(core_config.rfind('.'));
  if (gamespecificoptions)
      core_config += L"\\" + game_filename_ + L".ini";
  else
      core_config += L".ini";

  // set libretro func pointers
  ::load_envsymb(g_retro.handle);
  g_retro.retro_init();
  g_retro.initialized = true;
  return true;
}

static void noop() {}

//////////////////////////////////////////////////////////////////////////////////////////
CLibretro *CLibretro::GetInstance(HWND hwnd) {
  static CLibretro* instance = new CLibretro(hwnd);
  return instance;
}

bool CLibretro::running() { return isEmulating; }

CLibretro::CLibretro(HWND hwnd) {
  memset(&g_retro, 0, sizeof(g_retro));
  isEmulating = false;
  if (hwnd)init(hwnd);
}

CLibretro::~CLibretro(void) {
  if (isEmulating)
    isEmulating = false;
  kill();
}

void CLibretro::set_inputdevice(int dev) {
  if (isEmulating) {
    g_retro.retro_set_controller_port_device(0, dev);
  }
}

bool CLibretro::loadfile(TCHAR *filename, TCHAR *core_filename,
                         bool gamespecificoptions, int dev_option) {

  if (isEmulating) {
    kill();
    isEmulating = false;
  }
  system("cls");

  double refreshr = 0;
  struct retro_system_info system = {0};
  retro_system_av_info av = {0};
  variables.clear();
  variables_changed = false;

  g_video = {0};
  g_video.hw.version_major = 4;
  g_video.hw.version_minor = 5;
  g_video.hw.context_type = RETRO_HW_CONTEXT_NONE;
  g_video.hw.context_reset = NULL;
  g_video.hw.context_destroy = NULL;

  if (!core_load(core_filename, gamespecificoptions, filename)) {
    printf("FAILED TO LOAD CORE!!!!!!!!!!!!!!!!!!");
    return false;
  }

  string ansi = Mud_String::utf16toutf8(filename);
  const char *rompath = ansi.c_str();
  info = {rompath, 0};
  info.path = rompath;
  info.data = NULL;
  info.size = Mud_FileAccess::get_filesize(filename);
  info.meta = "";
  g_retro.retro_get_system_info(&system);
  if (!system.need_fullpath) {
    FILE *inputfile = _wfopen(filename, L"rb");
    if (!inputfile) {
    fail:
      printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
      return false;
    }
    info.data = malloc(info.size);
    if (!info.data)
      goto fail;
    size_t read = fread((void *)info.data, 1, info.size, inputfile);
    fclose(inputfile);
    inputfile = NULL;
  }
  core_name = Mud_String::utf8toutf16(system.library_name);
  if (!g_retro.retro_load_game(&info)) {
    printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
    return false;
  }
  g_retro.retro_get_system_av_info(&av);
  g_retro.retro_set_controller_port_device(0, dev_option);
  DEVMODE lpDevMode;
  memset(&lpDevMode, 0, sizeof(DEVMODE));
  lpDevMode.dmSize = sizeof(DEVMODE);
  lpDevMode.dmDriverExtra = 0;
  if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode) == 0) {
    refreshr = 60; // default value if cannot retrieve from user settings.
  }
  refreshr = lpDevMode.dmDisplayFrequency;
  ::video_init(&av.geometry, emulator_hwnd);
  audio_init(refreshr, av.timing.sample_rate, av.timing.fps);
  lastTime = (double)mudtime.milliseconds_now() / 1000;
  isEmulating = true;
  savesram((TCHAR*)sram_name.c_str(), false);
  return true;
}

void CLibretro::run() {
  static int nbFrames = 0;
  if (!g_video.software_rast) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  g_retro.retro_run();

  double currentTime = (double)mudtime.milliseconds_now() / 1000;
  if (currentTime - lastTime >=
      0.5) { // If last prinf() was more than 1 sec ago
             // printf and reset timer
    TCHAR buffer[200] = {0};
    int len =
        swprintf(buffer, 200, L"%s - %2f ms/frame\n, min %d VPS",
                 core_name.c_str(), 1000.0 / double(nbFrames), nbFrames);
    SetWindowText(emulator_hwnd, buffer);
    nbFrames = 0;
    lastTime += 1.0;
  }
  nbFrames++;
}

bool CLibretro::init(HWND hwnd) {
  isEmulating = false;
  emulator_hwnd = hwnd;
  return true;
}

void CLibretro::kill() {

  if (isEmulating) {
    savesram((TCHAR*)sram_name.c_str(), true);
    core_unload();
    isEmulating = false;
    if (info.data)
      free((void *)info.data);
    audio_destroy();
    video_deinit();
  }
}
