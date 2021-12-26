#include "CLibretro.h"
#include "io/gl_render.h"
#include "../../3rdparty-deps/libretro.h"
#include "mudlib.h"
#include "stdafx.h"
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "../../3rdparty-deps/ini.h"

#define INLINE
using namespace std;

std::vector<BYTE> load_inputsettings(TCHAR *path, unsigned *size) {
  unsigned size_ = Mud_FileAccess::get_filesize(path);
 
  if (size_) {
    std::vector<BYTE> data1 = Mud_FileAccess::load_data(path, &size_);
    ini_t *ini = ini_load((char*)data1.data(), NULL);
    int section = ini_find_section(ini, "Input Settings", strlen("Input Settings"));
    if (section == INI_NOT_FOUND) {
      ini_destroy(ini);
      return {};
    } else {
      int index = ini_find_property(ini, section, "Data", strlen("Data"));
      char const *text = ini_property_value(ini, section, index);
      tstring ansi = Mud_String::utf8toutf16(text);
      unsigned len = 0;
      std::vector<BYTE> data2=Mud_Base64::decode(ansi.c_str(), ansi.length(), &len);
      *size = len;
      ini_destroy(ini);
      return data2;
    }
  }
  return {};
}

const char *load_coresettings(retro_variable *var, CLibretro *retro) {
  for (int i = 0; i < retro->variables.size(); i++) {
    if (strcmp(retro->variables[i].name.c_str(), var->key) == 0)
      return retro->variables[i].var.c_str();
  }
  return NULL;
}

void save_inputsettings(unsigned char *data_ptr, unsigned data_sz,
                        CLibretro *retro) {
  ini_t *ini = NULL;
  TCHAR* config_path = (TCHAR*)retro->core_config.c_str();
  unsigned size_ = Mud_FileAccess::get_filesize(config_path);
  if (size_)
  {
    std::vector<BYTE> data = Mud_FileAccess::load_data(config_path, &size_);
    ini = ini_load((char*)data.data(), NULL);
    int section = ini_find_section(ini, "Input Settings", strlen("Input Settings"));
    unsigned outbaselen = 0;
    std::wstring data2 = Mud_Base64::encode(data_ptr, data_sz, &outbaselen);
    string ansi = Mud_String::utf16toutf8(data2);
    if (section != INI_NOT_FOUND) {
      int idx = ini_find_property(ini, section, "Data", strlen("Data"));
      ini_property_value_set(ini, section, idx, ansi.c_str(), outbaselen);
    } else {
      section =
          ini_section_add(ini, "Input Settings", strlen("Input Settings"));
      ini_property_add(ini, section, "Data", strlen("Data"), ansi.c_str(),
                       outbaselen);
    }

    size_ = ini_save(ini, NULL, 0); // Find the size needed
    auto ini_data = std::make_unique<char[]>(size_);
    size_ = ini_save(ini, ini_data.get(), size_); // Actually save the file
    Mud_FileAccess::save_data((unsigned char*)ini_data.get(), size_, config_path);
    ini_destroy(ini);
  }
  
}

void save_coresettings(CLibretro *retro) {
 
  ini_t *ini = NULL;
  unsigned size_ = Mud_FileAccess::get_filesize((TCHAR*)retro->core_config.c_str());
  TCHAR* config_path = (TCHAR*)retro->core_config.c_str();
  if (size_) {
    std::vector<BYTE> data = Mud_FileAccess::load_data(config_path, &size_);
    ini = ini_load((char*)data.data(), NULL);
    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    for (int i = 0; i < retro->variables.size(); i++) {

      int idx = ini_find_property(ini, section,
                                  (char *)retro->variables[i].name.c_str(),
                                  retro->variables[i].name.length());
      ini_property_value_set(ini, section, idx, retro->variables[i].var.c_str(),
                             retro->variables[i].var.length());
    }


    size_ = ini_save(ini, NULL, 0); // Find the size needed
    auto ini_data = std::make_unique<char[]>(size_);
    size_ = ini_save(ini, ini_data.get(), size_); // Actually save the file
    Mud_FileAccess::save_data((unsigned char*)ini_data.get(), size_, config_path);
    ini_destroy(ini);
  }
  
}

void init_coresettings(retro_variable *var, CLibretro *retro) {
  // set up core variable information and default key settings

  TCHAR* config_path = (TCHAR*)retro->core_config.c_str();

  std::vector<CLibretro::core_vars> variables;
  while (var->value != NULL) {
      CLibretro::core_vars vars_struct;
      vars_struct.name = var->key;
      vars_struct.config_visible = true;
      std::string description = var->value;
      std::string::size_type pos = description.find(';');
      vars_struct.description = description.substr(0, pos);
      pos += 2; // skip ; and space to first variable
      //get all variables
      vars_struct.usevars = description.substr(pos, string::npos);
      description = description.substr(pos, string::npos);
      //find first option in list, it is default
      pos = description.find('|');
      if (pos != std::string::npos)
          description = description.substr(0, pos);
      vars_struct.var = description;
      variables.push_back(vars_struct);
      var++;
  }
  
  

  int size_ = Mud_FileAccess::get_filesize(config_path);
  if (!size_) {
      redo:
      retro->variables=variables;
      
    // create a new file with defaults
    //create cache of core options
    ini_t *ini = ini_create(NULL);
    int section =
        ini_section_add(ini, "Core Settings", strlen("Core Settings"));
    for (int i = 0; i < retro->variables.size(); i++) {
      ini_property_add(ini, section, (char *)retro->variables[i].name.c_str(),
                       retro->variables[i].name.length(),
                       (char *)retro->variables[i].var.c_str(),
                      retro->variables[i].var.length());
      
    }

    for (int i = 0; i < retro->variables.size(); i++)
    {
        std::string variable_name = retro->variables[i].name+ "_usedvars_";
        ini_property_add(ini, section, variable_name.c_str(), variable_name.length(),
            (char*)retro->variables[i].usevars.c_str(), retro->variables[i].usevars.length());
        variable_name = retro->variables[i].name+ "_description_";
        ini_property_add(ini, section, variable_name.c_str(), variable_name.length(),
            (char*)retro->variables[i].description.c_str(), retro->variables[i].description.length());
    }

    char lengthstr[33] = { 0 };
    itoa(retro->variables.size(), lengthstr, 10);
    ini_property_add(ini, section, "usedvars_num", strlen("usedvars_num"), lengthstr, strlen(lengthstr));

    int size = ini_save(ini, NULL, 0); // Find the size needed
    auto ini_data = std::make_unique<char[]>(size);
    size = ini_save(ini, ini_data.get(), size); // Actually save the file
    ini_destroy(ini);
    Mud_FileAccess::save_data((unsigned char*)ini_data.get(), size, config_path);
  } else {
    //load
    std::vector<BYTE> data = Mud_FileAccess::load_data(config_path, (unsigned int*)&size_);
    ini_t *ini = ini_load((char*)data.data(), NULL);


    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    int idx = ini_find_property(ini, section, "usedvars_num",strlen("usedvars_num"));
    const char* numvars = ini_property_value(ini, section, idx);
    int vars_infile =atoi(numvars);
    if (variables.size() != vars_infile)
    {
        //rebuild cache.
        ini_destroy(ini);
        goto redo;
    }
    bool save = false;
    //cache is ok, load from cache
    for (int i = 0; i < vars_infile; i++) {
      CLibretro::core_vars vars_struct;
      std::string name=  ini_property_name(ini, section, i);
      std::string value = ini_property_value(ini, section, i);
      std::string description = name+ "_description_";
      std::string usedvars = name+ "_usedvars_";
      int idx_description =ini_find_property(ini, section, (char *)description.c_str(),description.length());
      int idx_usedvars = ini_find_property(ini,section,(char*)usedvars.c_str(),usedvars.length());
      const char* description_val = ini_property_value(ini, section, idx_description);
      const char* usedvars_val = ini_property_value(ini, section, idx_usedvars);
      vars_struct.name = name;
      vars_struct.var = value;
      vars_struct.description = description_val;
      vars_struct.usevars = usedvars_val;
      retro->variables.push_back(vars_struct);
    }
    if (save) {
      int size = ini_save(ini, NULL, 0); // Find the size needed
      auto ini_data = std::make_unique<char[]>(size);
      size = ini_save(ini, ini_data.get(), size); // Actually save the file
      Mud_FileAccess::save_data((unsigned char*)ini_data.get(), size, config_path);
    }
    ini_destroy(ini);
  }
}

static void core_audio_sample(int16_t left, int16_t right) {
  CLibretro *lib = CLibretro::GetInstance();
  if (lib->isEmulating) {
    int16_t buf[2] = {left, right};
    audio_mix(buf, 1);
  }
}
static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
  CLibretro *lib = CLibretro::GetInstance();
  if (lib->isEmulating) {
    audio_mix(data, frames);
  }
  return frames;
}

static void core_log(enum retro_log_level level, const char *fmt, ...) {
  char buffer[4096] = {0};
  static const char *levelstr[] = {"dbg", "inf", "wrn", "err"};
  va_list va;
  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  va_end(va);
  if (level == 0)
    return;
  fprintf(stdout, "[%s] %s", levelstr[level], buffer);
}

static bool core_environment(unsigned cmd, void *data) {
  bool *bval;
  CLibretro *retro = CLibretro::GetInstance();
  input *input_device = input::GetInstance();
  switch (cmd) {
  case RETRO_ENVIRONMENT_SET_MESSAGE: {
    struct retro_message *cb = (struct retro_message *)data;
    return true;
  }
  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
    struct retro_log_callback *cb = (struct retro_log_callback *)data;
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: {
      struct retro_core_option_display* cb = (struct retro_core_option_display*)data;
      for (int i = 0; i < retro->variables.size(); i++)
      {
          if (strcmp(retro->variables[i].name.c_str(), cb->key) == 0)
          {
              retro->variables[i].config_visible = cb->visible;
              return true;
          }
         
      }
      return false;
      break;
  }

  case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
    *(bool *)data = false;
    return true;
  } break;

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval = (bool *)data;
    *bval = true;
    return true;
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
  {
    static char *sys_path = NULL;
    if (!sys_path) {
      string ansi = Mud_String::utf16toutf8(retro->sys_name);
      sys_path = strdup(ansi.c_str());
    }
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }

  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: // 31
  {
    static char *sys_path = NULL;
    if (!sys_path) {
      string ansi = Mud_String::utf16toutf8(retro->save_name);
      sys_path = strdup(ansi.c_str());
    }
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
  } break;

  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 31
  {
      struct retro_input_descriptor* var =
          (struct retro_input_descriptor*)data;
      struct ra_bind {
          char description[64];
          unsigned retroid;
      };
      vector<ra_bind> bind_ra;
      
      int i = 0;
      bind_ra.clear();
      while (var->description != NULL && var->port == 0) {
          ra_bind binds = { 0 };
          strcpy(binds.description, var->description);
          if (var->device == RETRO_DEVICE_ANALOG ||
              (var->device == RETRO_DEVICE_JOYPAD)) {
              if (var->device == RETRO_DEVICE_ANALOG) {
                  int retro_id =
                      (var->index == RETRO_DEVICE_INDEX_ANALOG_LEFT)
                      ? (var->id == RETRO_DEVICE_ID_ANALOG_X ? 16 : 17)
                      : (var->id == RETRO_DEVICE_ID_ANALOG_X ? 18 : 19);
                  binds.retroid = retro_id;
                  bind_ra.push_back(binds);
              }
              else {
                  binds.retroid = var->id;
                  bind_ra.push_back(binds);
              }
          }
          var++;
      }

    unsigned sz = 0;
    std::vector<BYTE> config = load_inputsettings((TCHAR*)retro->core_config.c_str(), &sz);
    if (!config.empty()) {
      Mem_File_Reader out((BYTE*)config.data(), sz);
      const char *err = input_device->load(out);
      if (!err) {
        int i = 0;
        while (i<input_device->bl->get_count()) {
          std::string str = Mud_String::utf16toutf8(input_device->bl->getdescription(i));
          if (strcmp(bind_ra[i].description, str.c_str()))
            goto init;
          i++;
        }
      }

    } else {
    init:
      input_device->bl->clear();
      int i = 0;
      while (i<bind_ra.size()) {
        dinput::di_event keyboard;
        keyboard.type = dinput::di_event::ev_none;
        keyboard.key.type = dinput::di_event::key_none;
        keyboard.key.which = NULL;
        wstring str = Mud_String::ansitoutf16(bind_ra[i].description);
        input_device->bl->add(keyboard, i, (TCHAR*)str.c_str(), bind_ra[i].retroid);
        i++;
      }
      Mem_Writer out2;
      input_device->save(out2);
      save_inputsettings((unsigned char *)out2.data(), out2.size(), retro);
    }
    return true;
  }

  break;

  case RETRO_ENVIRONMENT_SET_VARIABLES: {
    struct retro_variable *var = (struct retro_variable *)data;
    init_coresettings(var, retro);
    return true;
  } break;

  case RETRO_ENVIRONMENT_GET_VARIABLE: {
    struct retro_variable *variable = (struct retro_variable *)data;
    variable->value = load_coresettings(variable, retro);
    return true;
  } break;

  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
    *(bool *)data = retro->variables_changed;
    retro->variables_changed = false;
    return true;
  } break;

  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
      return false;
      break;
  }

  case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
      return false;
  }

  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
    const enum retro_pixel_format *fmt = (const enum retro_pixel_format *)data;
    return video_set_pixel_format(*fmt);
  }
  case RETRO_ENVIRONMENT_SET_HW_RENDER: {
    struct retro_hw_render_callback *hw =
        (struct retro_hw_render_callback *)data;
    if (hw->context_type == RETRO_HW_CONTEXT_VULKAN)
      return false;
    hw->get_current_framebuffer = core_get_current_framebuffer;
    hw->get_proc_address = (retro_hw_get_proc_address_t)get_proc;
    g_video.hw = *hw;
    return true;
  }
  default:
    core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
    return false;
  }
  return false;
}

static void core_video_refresh(const void *data, unsigned width,
                               unsigned height, size_t pitch) {
    CLibretro* retro = CLibretro::GetInstance();
    if (retro->isEmulating)
  video_refresh(data, width, height, pitch);
}

static void core_input_poll(void) {
  input *input_device = input::GetInstance();
  if (!input_device)
    return;
  input_device->poll();
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index,
                                unsigned id) {
  if (port != 0)
    return 0;
  input *input_device = input::GetInstance();
  if (!input_device)
    return 0;

  if (device == RETRO_DEVICE_MOUSE) {
    int16_t x = 0, y = 0;
    bool left = false, right = false;
    input_device->readmouse(x, y, left, right);
    switch (id) {
    case RETRO_DEVICE_ID_MOUSE_X:
      return x;
    case RETRO_DEVICE_ID_MOUSE_Y:
      return y;
    case RETRO_DEVICE_ID_MOUSE_LEFT:
      return left;
    case RETRO_DEVICE_ID_MOUSE_RIGHT:
      return right;
    }
    return 0;
  }

  if (device == RETRO_DEVICE_ANALOG || device == RETRO_DEVICE_JOYPAD) {
    for (unsigned int i = 0; i < input_device->bl->get_count(); i++) {
      int retro_id = 0;
      int16_t value = 0;
      bool isanalog = false;
      input_device->getbutton(i, value, retro_id, isanalog);
      if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT &&
          device == RETRO_DEVICE_ANALOG) {
        if ((id == RETRO_DEVICE_ID_ANALOG_X && retro_id == 16) ||
            (id == RETRO_DEVICE_ID_ANALOG_Y && retro_id == 17)) {
          if (value <= -0x8000)
            value = -0x7fff;
          return isanalog ? -value : value;
        }
      } else if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT &&
                 device == RETRO_DEVICE_ANALOG) {
        if ((id == RETRO_DEVICE_ID_ANALOG_X && retro_id == 18) ||
            (id == RETRO_DEVICE_ID_ANALOG_Y && retro_id == 19)) {
          if (value <= -0x8000)
            value = -0x7fff;
          return isanalog ? -value : value;
        }
      } else {
        if (retro_id == id) {
          value = abs(value);
          return value;
        }
      }
    }
  }
  return 0;
}

void load_envsymb(HMODULE handle) {
#define libload(name) GetProcAddress(handle, name)
#define load_sym(V, name) if (!(*(void **)(&V) = (void *)libload(#name)))
  void (*set_environment)(retro_environment_t) = NULL;
  void (*set_video_refresh)(retro_video_refresh_t) = NULL;
  void (*set_input_poll)(retro_input_poll_t) = NULL;
  void (*set_input_state)(retro_input_state_t) = NULL;
  void (*set_audio_sample)(retro_audio_sample_t) = NULL;
  void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
  load_sym(set_environment, retro_set_environment);
  load_sym(set_video_refresh, retro_set_video_refresh);
  load_sym(set_input_poll, retro_set_input_poll);
  load_sym(set_input_state, retro_set_input_state);
  load_sym(set_audio_sample, retro_set_audio_sample);
  load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);
  set_environment(core_environment);
  set_video_refresh(core_video_refresh);
  set_input_poll(core_input_poll);
  set_input_state(core_input_state);
  set_audio_sample(core_audio_sample);
  set_audio_sample_batch(core_audio_sample_batch);
}
