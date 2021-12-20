#ifndef CEMULATOR_H
#define CEMULATOR_H
#include <list>
#include "io/input.h"
#include "io/audio.h"
#define MUDLIB_IMPLEMENTATION
#include "mudlib.h"
#include <mutex>
#include "../../3rdparty-deps/libretro.h"
namespace std
{
	typedef wstring        tstring;
	typedef wistringstream tistringstream;
	typedef wostringstream tostringstream;
}



class CLibretro
{
private:	
	Mud_Timing mudtime;
	double lastTime;
	struct retro_game_info info;
	HWND emulator_hwnd;
	bool init(HWND hwnd);
	bool core_load(TCHAR* sofile, bool specifics, TCHAR* filename);
	void core_unload();
public:
	CLibretro(HWND hwnd = NULL);
	~CLibretro();
	static CLibretro* GetInstance(HWND hwnd = NULL);

	struct core_vars
	{
		std::string name;
		std::string var;
		std::string description;
		std::string usevars;
		bool config_visible;
	};

	bool running();
	bool loadfile(TCHAR* filename, TCHAR* core_filename, bool gamespecificoptions,int dev_option);
	void run();
	void reset();
	bool savestate(TCHAR* filename, bool save = false);
	bool savesram(TCHAR* filename, bool save = false);
	void kill();
	void set_inputdevice(int dev);

	struct {
		HMODULE handle;
		bool initialized;
		void(*retro_init)(void);
		void(*retro_deinit)(void);
		unsigned(*retro_api_version)(void);
		void(*retro_get_system_info)(struct retro_system_info* info);
		void(*retro_get_system_av_info)(struct retro_system_av_info* info);
		void(*retro_set_controller_port_device)(unsigned port, unsigned device);
		void(*retro_reset)(void);
		void(*retro_run)(void);
		size_t(*retro_serialize_size)(void);
		bool(*retro_serialize)(void* data, size_t size);
		bool(*retro_unserialize)(const void* data, size_t size);
		bool(*retro_load_game)(const struct retro_game_info* game);
		void* (*retro_get_memory_data)(unsigned id);
		size_t(*retro_get_memory_size)(unsigned id);
		void(*retro_unload_game)(void);
	} g_retro;

	
	std::vector<core_vars> variables;

	tstring sys_name;
	tstring save_name;
	tstring sram_name;
	tstring core_config;
	tstring core_name;
	bool variables_changed;
	BOOL isEmulating;
};

#ifdef __cplusplus
extern "C" {
#endif
	void load_envsymb(HMODULE handle);
	void save_coresettings(CLibretro *retro);
	std::vector<BYTE> load_inputsettings(TCHAR* path, unsigned* size);
	void save_inputsettings(unsigned char* data_ptr, unsigned data_sz, CLibretro* retro);
#ifdef __cplusplus
}
#endif

#endif CEMULATOR_H
