#ifndef _input_h_
#define _input_h_

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>

#include "guid_container.h"
#include "dinput.h"
#include "bind_list.h"
#include "blargg_stuff.h"

class Data_Reader;
class Data_Writer;

void reset_input(HINSTANCE hInstance = NULL, HWND hWnd = NULL);

class input
{
  // needed by SetCooperativeLevel inside enum callback
  //HWND                    hWnd;
public:
  int list_count;
  input(HINSTANCE hInstance = NULL,HWND hwnd = NULL);
  ~input();
  unsigned                bits;
  LPDIRECTINPUT8          lpDI;
  TCHAR path[MAX_PATH];
  guid_container        * guids;
  dinput                * di;
  bind_list             * bl;
  HWND hwnd;
  static input* GetInstance(HINSTANCE hInstance = NULL, HWND hWnd=NULL);


  const char* open(HINSTANCE hInstance, HWND hWnd);
  void close();
  // configuration
  const char* load(Data_Reader &);
  const char* save(Data_Writer &);
  // input_i_dinput
  void poll();
  bool getbutton(int which, int16_t & value, int & retro_id, bool & isanalog);
  void readmouse(int16_t & x, int16_t & y, bool &lbutton, bool &rbutton);
  unsigned read();
  void strobe();
  int get_speed();
  void set_speed(int);
  void set_paused(bool);
  void clear_binds();
  TCHAR* getdescription(int which);
  void set_focus(bool is_focused);
  void refocus(void * hwnd);
};

#endif