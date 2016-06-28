// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psGUIManager.h"
#include "bss-util/bss_win32_includes.h"
#include "bss-util/profiler.h"
#include "psEngine.h"
#include <Mmsystem.h>
#include <dwmapi.h>

using namespace planeshader;
using namespace bss_util;

#pragma comment(lib, "Winmm.lib")

psGUIManager::psGUIManager() : _preprocess(0, 0), _postprocess(0, 0), _firstjoystick(0), _alljoysticks(0), _maxjoy((uint8_t)FG_JOYSTICK_ID16), _quit(false)
{
  GetKeyboardState(_allkeys);
  memset(&_allbuttons, 0, sizeof(uint32_t)*NUMJOY);
  memset(&_alljoyaxis, 0, sizeof(unsigned long)*NUMJOY*NUMAXIS);
  memset(&_joydevs, 0, sizeof(JOY_DEVCAPS)*NUMJOY);

  _root.gui.element.message = (fgMessage)&Message;
  _joyupdateall();
}
psGUIManager::~psGUIManager()
{
  PROFILE_FUNC();
  ChangeDisplaySettings(NULL, 0);
  ShowCursor(TRUE);
}
psMonitor* psGUIManager::AddMonitor(psVeciu& dim, psMonitor::MODE mode, HWND__* window)
{
  _monitors.AddConstruct(this, dim, mode, window);
  return &_monitors.Back();
}

size_t psGUIManager::SetKey(uint8_t keycode, bool down, bool held, DWORD time)
{
  PROFILE_FUNC();
  GetKeyboardState(_allkeys);
  //bool held=down && _getkey(keycode);
  _allkeys[keycode] = down ? (_allkeys[keycode] | 0x08) : (_allkeys[keycode] & (~0x08)); //For sanity, ensure this matches what we just got.

  FG_Msg evt = { 0 };
  evt.type = down ? FG_KEYDOWN : FG_KEYUP;
  _lastmsgtime = time;
  evt.keycode = keycode;
  evt.sigkeys = 0;
  if(GetKey(FG_KEY_SHIFT)) evt.sigkeys = evt.sigkeys | 1; //VK_SHIFT
  if(GetKey(FG_KEY_CONTROL)) evt.sigkeys = evt.sigkeys | 2; //VK_CONTROL
  if(GetKey(FG_KEY_MENU)) evt.sigkeys = evt.sigkeys | 4; //VK_MENU
  if(held) evt.sigkeys = evt.sigkeys | 8;

  return _process(evt);
}
void psGUIManager::SetChar(int key, DWORD time)
{
  PROFILE_FUNC();
  GetKeyboardState(_allkeys);

  FG_Msg evt = { 0 };
  evt.type = FG_KEYCHAR;
  _lastmsgtime = time;
  evt.keychar = key;
  evt.sigkeys = 0;

  if(GetKey(FG_KEY_SHIFT)) evt.sigkeys = evt.sigkeys | 1; //VK_SHIFT
  if(GetKey(FG_KEY_CONTROL)) evt.sigkeys = evt.sigkeys | 2; //VK_CONTROL
  if(GetKey(FG_KEY_MENU)) evt.sigkeys = evt.sigkeys | 4; //VK_MENU
  //if(held) evt.sigkeys = evt.sigkeys|8;

  _process(evt);
}

void psGUIManager::SetMouse(POINTS* points, unsigned short type, unsigned char button, size_t wparam, DWORD time)
{
  PROFILE_FUNC();
  FG_Msg evt = { 0 };
  _lastmsgtime = time;
  evt.type = type;
  evt.button = button;
  evt.x = _root.mouse.x; // Set these up here for any mouse events that don't contain coordinates.
  evt.y = _root.mouse.y;

  if(type == FG_MOUSELEAVE)
  {
    _process(evt);
    return;
  }

  if(wparam != (size_t)-1) //if wparam is -1 it signals that it is invalid, so we simply leave our assignments at their last known value.
  {
    uint8_t bt = 0; //we must keep these bools accurate at all times
    bt |= FG_MOUSELBUTTON&(-((wparam&MK_LBUTTON) != 0));
    bt |= FG_MOUSERBUTTON&(-((wparam&MK_RBUTTON) != 0));
    bt |= FG_MOUSEMBUTTON&(-((wparam&MK_MBUTTON) != 0));
    bt |= FG_MOUSEXBUTTON1&(-((wparam&MK_XBUTTON1) != 0));
    bt |= FG_MOUSEXBUTTON2&(-((wparam&MK_XBUTTON2) != 0));
    _root.mouse.buttons = bt;
    _allkeys[FG_KEY_LBUTTON] = ((_root.mouse.buttons&FG_MOUSELBUTTON) != 0) * 0x80;
    _allkeys[FG_KEY_RBUTTON] = ((_root.mouse.buttons&FG_KEY_RBUTTON) != 0) * 0x80;
    _allkeys[FG_KEY_MBUTTON] = ((_root.mouse.buttons&FG_KEY_MBUTTON) != 0) * 0x80;
    _allkeys[FG_KEY_XBUTTON1] = ((_root.mouse.buttons&FG_KEY_XBUTTON1) != 0) * 0x80;
    _allkeys[FG_KEY_XBUTTON2] = ((_root.mouse.buttons&FG_KEY_XBUTTON2) != 0) * 0x80;
  }

  if(type != FG_MOUSESCROLL) //The WM_MOUSEWHEEL event does not send mousecoord data
  {
    evt.x = points->x;
    evt.y = points->y;
    evt.allbtn = _root.mouse.buttons;
    _root.mouse.x = points->x;
    _root.mouse.y = points->y;
  }

  switch(type)
  {
  case FG_MOUSESCROLL:
    evt.scrolldelta = points->y;
    evt.scrollhdelta = points->x;
    break;
  case FG_MOUSEDBLCLICK: //L down
  case FG_MOUSEDOWN: //L down
    evt.allbtn |= evt.button; // Ensure the correct button position is reflected in the allbtn parameter regardless of the current state of the mouse
    break;
  case FG_MOUSEUP: //L up
    evt.allbtn &= ~evt.button;
    break;
  }

  _process(evt);
}
// Shows/hides the hardware cursor
void psGUIManager::ShowCursor(bool show)
{
  ::ShowCursor(show ? 1 : 0);
}
size_t FG_FASTCALL psGUIManager::Message(fgRoot* self, const FG_Msg* m)
{
  return fgRoot_Message(self, m);
}

void psGUIManager::_updaterootarea()
{
  fgMonitor* cur = _root.monitors;
  AbsRect& primary = cur->coverage;
  CRect area { 0, 0, 0, 0, primary.right - primary.left, 0, primary.bottom - primary.top, 0 };
  while((cur = cur->mnext) != 0)
  {
    AbsRect& coverage = cur->coverage;
    if(coverage.left < area.left.abs) area.left.abs = coverage.left;
    if(coverage.top < area.top.abs) area.top.abs = coverage.top;
    if(coverage.right > area.right.abs) area.right.abs = coverage.right;
    if(coverage.bottom > area.bottom.abs) area.bottom.abs = coverage.bottom;
  }

  _root.gui.element.SetArea(area);
}

char psGUIManager::CaptureAllJoy(HWND__* hwnd)
{
  PROFILE_FUNC();
  char r = 0;
  _alljoysticks = 0;
  for(uint16_t i = 0; i < _maxjoy; ++i)
  {
    if(joySetCapture(hwnd, i, 0, TRUE) == JOYERR_NOERROR)
    {
      ++r;
      _alljoysticks |= (1 << i);
      if(_firstjoystick == FG_JOYSTICK_INVALID)
        _firstjoystick = (i << 8);
    }
  }
  return r;
}

void psGUIManager::FlushMessages()
{
  PROFILE_FUNC();
  MSG msg;

  while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
  {
    LRESULT r = DispatchMessageW(&msg);

    switch(msg.message)
    {
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      case WM_KEYDOWN:
        if(!r) // if the return value is zero, we already processed the keydown message successfully, so DON'T turn it into a character.
          break;
      default:
        TranslateMessage(&msg);
        break;
      case WM_QUIT:
        Quit();
        PSLOGV(3, "Quit message recieved, setting _quit to true");
        return;
    }
  }

  _joyupdateall();
  _exactmousecalc(); // Recalculate mouse AFTER the messages, so that all events get mouse coordinates based on where the mouse was when they happened, but we end up with accurate coordinates.
}

psVeciu psGUIManager::GetMonitorDPI(int num)
{
  UINT xdpi;
  UINT ydpi;
  HDC hdc = GetDC(0); // Before windows 8.1, windows returns the same DPI for all monitors, so the DC here is irrelevent.
  xdpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
  ydpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSY);
  //GetDpiForMonitor(0, MDT_Effective_DPI, &xdpi, &ydpi);
  return psVeciu(xdpi, ydpi);
}

void psGUIManager::_joyupdateall()
{
  PROFILE_FUNC();
  JOYINFOEX info;
  info.dwSize = sizeof(JOYINFOEX);
  info.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNCENTERED | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV;

  for(uint16_t i = 0; i < _maxjoy; ++i)
  {
    if(!(_alljoysticks & (1 << i))) continue;
    if(joyGetPosEx(i, &info) == JOYERR_NOERROR)
    {
      if(_allbuttons[i] != info.dwButtons)
      {
        FG_Msg evt = { 0 };
        _lastmsgtime = GetTickCount();
        uint32_t old = _allbuttons[i];
        _allbuttons[i] = info.dwButtons;
        uint32_t diff = (old^info.dwButtons);
        uint32_t k;
        for(int j = 0; j < 32; ++j) //go through the bits and generate BUTTONUP or BUTTONDOWN events
        {
          k = (1 << j);
          if((diff&k) != 0)
          {
            evt.joydown = (info.dwButtons&k) != 0;
            evt.type = (evt.joydown != 0) ? FG_JOYBUTTONDOWN : FG_JOYBUTTONUP;
            evt.joybutton = (i << 8) | j;
            _process(evt);
          }
        }
      }

      uint8_t numaxes = _joydevs[i].numaxes;
      if(memcmp(&_alljoyaxis[i][FG_JOYAXIS_X], &info.dwXpos, sizeof(unsigned long)*numaxes) != 0)
      {
        unsigned long old[NUMAXIS];
        memcpy(old, &_alljoyaxis[i][FG_JOYAXIS_X], sizeof(unsigned long)*numaxes);
        memcpy(&_alljoyaxis[i][FG_JOYAXIS_X], &info.dwXpos, sizeof(unsigned long)*numaxes);

        for(int j = 0; j < numaxes; ++j)
        {
          if(old[j] != _alljoyaxis[i][j])
          {
            FG_Msg evt = { 0 };
            _lastmsgtime = GetTickCount();
            evt.type = FG_JOYAXIS;
            evt.joyaxis = (i << 8) | j;
            evt.joyvalue = _translatejoyaxis(evt.joyaxis);
            _process(evt);
          }
        }
      }
    }
    else
      _alljoysticks &= (~(1 << i)); // If it failed, remove joystick from active list
  }
}

float psGUIManager::_translatejoyaxis(uint16_t axis) const
{
  uint8_t ID = (axis >> 8);
  uint8_t a = (axis & 0xFF);
  return (((long)_alljoyaxis[ID][a]) - _joydevs[ID].offset[a]) / _joydevs[ID].range[a];
}
size_t psGUIManager::_process(FG_Msg& m)
{
  if(!_preprocess.IsEmpty() && _preprocess(m) != 0)
    return FG_ACCEPT;

  if(fgRoot_Inject(&_root, &m))
    return FG_ACCEPT;

  if(!_postprocess.IsEmpty() && _postprocess(m) != 0)
    return FG_ACCEPT;

  return 0;
}

void psGUIManager::_exactmousecalc()
{
  PROFILE_FUNC();
  POINT p;
  //GetCursorPos(&p); //This fails for large addresses
  {
    CURSORINFO ci;
    ci.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&ci);
    p = ci.ptScreenPos;
  }

  assert(_monitors.Length() > 0);
  ScreenToClient(_monitors[0].GetWindow(), &p);
  _root.mouse.x = p.x;
  _root.mouse.y = p.y;
}
