#include "input.h"

static const GUID g_signature = {
    0x925c561e,
    0xfdfe,
    0x40b3,
    {0x9a, 0xe9, 0xbf, 0x82, 0x85, 0x86, 0x4b, 0xb5}};


input *input::GetInstance(HINSTANCE hInstance, HWND hWnd) {
    static input* instance = new input(hInstance,hWnd);
    return instance;
}

TCHAR *input::getdescription(int which) { return bl->getdescription(which); }

void reset_input(HINSTANCE hInstance, HWND hWnd) {
  input *input_device = input::GetInstance(hInstance, hWnd);
  if (input_device) {
    input_device->close();
    input_device->open(hInstance, hWnd);
    input_device->hwnd = hWnd;
  }
}

void input::close() {
  if (bl) {
    delete bl;
    bl = 0;
  }

  if (di) {
    delete di;
    di = 0;
  }

  if (guids) {
    delete guids;
    guids = 0;
  }

  if (lpDI) {
    lpDI->Release();
    lpDI = 0;
  }
}

input::input(HINSTANCE hInstance, HWND hWnd) {
  list_count = 0;
  bits = 0;
  lpDI = 0;
  guids = 0;
  di = 0;
  bl = 0;
  if (hwnd)
  {
    open(hInstance, hWnd);
  }
}

input::~input() { close(); }

const char *input::open(HINSTANCE hInstance, HWND hWnd) {
  close();
  if (DirectInput8Create(hInstance, DIRECTINPUT_VERSION,
#ifdef UNICODE
                         IID_IDirectInput8W
#else
                         IID_IDirectInput8A
#endif
                         ,
                         (void **)&lpDI, 0) != DI_OK)
    return "Creating DirectInput object";

  guids = create_guid_container();

  di = create_dinput();

  const char *err = di->open(lpDI, hWnd, guids);
  if (err)
    return err;

  bl = create_bind_list(guids);

  return 0;
}

const char *input::load(Data_Reader &in) {
  const char *err;
  GUID check;
  err = in.read(&check, sizeof(check));
  if (!err) {
    if (check != g_signature)
      err = "Invalid signature";
  }
  if (!err)
    err = bl->load(in);

  return err;
}

const char *input::save(Data_Writer &out) {
  const char *err;

  err = out.write(&g_signature, sizeof(g_signature));
  if (!err)
    err = bl->save(out);

  return err;
}
/*
virtual void configure( void * hinstance, void * hwnd )
{
  bind_list * bl = this->bl->copy();

  if ( do_input_config( lpDI, hinstance, hwnd, guids, bl ) )
  {
    delete this->bl;
    this->bl = bl;
  }
  else delete bl;
}*/

void input::poll() {
  di->poll_mouse();
  if (bl)
    bl->process(di->read(true));
}

bool input::getbutton(int which, int16_t &value, int &retro_id,
                      bool &isanalog) {
  return bl->getbutton(which, value, retro_id, isanalog);
}

void input::readmouse(int16_t &x, int16_t &y, bool &lbutton, bool &rbutton) {
  di->readmouse(x, y, lbutton, rbutton);
}

unsigned input::read() {
  unsigned ret;

  ret = bl->read();
  return ret;
}

void input::strobe() { bl->strobe(0); }

int input::get_speed() {
  int ret;

  ret = bl->get_speed();

  return ret;
}

void input::set_speed(int speed) { bl->set_speed(speed); }

void input::set_paused(bool paused) { bl->set_paused(paused); }

void input::set_focus(bool is_focused) { di->set_focus(is_focused); }

void input::refocus(void *hwnd) { di->refocus(hwnd); }