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

#define MAKELPPOINTS(l)       ((POINTS FAR *)&(l))
#pragma comment(lib, "Winmm.lib")

void psGUIManager::_lockcursor(HWND hWnd, bool lock)
{
  if(!lock || GetActiveWindow()!=hWnd)
    ClipCursor(0);
  else
  {
    RECT holdrect;
    memset(&holdrect, 0, sizeof(RECT));
    AdjustWindowRect(&holdrect, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    RECT winrect;
    GetWindowRect(hWnd, &winrect);
    winrect.left-=holdrect.left;
    winrect.top-=holdrect.top;
    winrect.right-=holdrect.right;
    winrect.bottom-=holdrect.bottom;
    ClipCursor(&winrect);
  }
}

POINTS* __stdcall psGUIManager::_STCpoints(HWND hWnd, POINTS* target)
{
  POINT pp={ target->x, target->y };
  ScreenToClient(hWnd, &pp);
  target->x=(SHORT)pp.x;
  target->y=(SHORT)pp.y;
  return target;
}

LRESULT __stdcall psGUIManager::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PROFILE_FUNC();
  static tagTRACKMOUSEEVENT _trackingstruct ={ sizeof(tagTRACKMOUSEEVENT), TME_LEAVE, 0, 0 };
  WPARAM wpcopy=wParam;
  psGUIManager* self = psEngine::Instance();
  if(!self)
    return DefWindowProcW(hWnd, message, wParam, lParam);
  
  switch(message)
  {
    //case WM_PAINT:
    //cEngine::Instance()->Render();
    //return 0;
    //break;
  case WM_DESTROY:
    psEngine::Instance()->Quit();
    PostQuitMessage(0);
    break;
  case WM_MOUSEWHEEL:
  {
    POINTS pointstemp;
    pointstemp.x = GET_WHEEL_DELTA_WPARAM(wpcopy);
    psEngine::Instance()->SetMouse(&pointstemp, GUI_MOUSESCROLL, wpcopy, GetMessageTime());
  }
  break;
  case WM_NCMOUSEMOVE:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam)); //This is a bit weird but it works because we get passed lParam by value, so this function actually ends up modifying lParam directly.
  case WM_MOUSEMOVE:
    if(!(self->_guiflags&PSGUIMANAGER_ISINSIDE))
    {
      _trackingstruct.hwndTrack = hWnd;
      BOOL result = TrackMouseEvent(&_trackingstruct);
      self->_guiflags += PSGUIMANAGER_ISINSIDE;
    }
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEMOVE, wpcopy, GetMessageTime());
    break;
  case WM_NCLBUTTONDOWN:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_LBUTTONDOWN:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDOWN | (GUI_L_BUTTON << 4), wpcopy, GetMessageTime());
    SetCapture(hWnd);
    break;
  case WM_NCLBUTTONUP:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_LBUTTONUP:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEUP | (GUI_L_BUTTON << 4), wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_LBUTTONDBLCLK:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDBLCLICK | (GUI_L_BUTTON << 4), wpcopy, GetMessageTime());
    break;
  case WM_NCRBUTTONDOWN:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_RBUTTONDOWN:
    SetCapture(hWnd);
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDOWN | (GUI_R_BUTTON << 4), wpcopy, GetMessageTime());
    break;
  case WM_NCRBUTTONUP:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_RBUTTONUP:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEUP | (GUI_R_BUTTON << 4), wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_RBUTTONDBLCLK:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDBLCLICK | (GUI_R_BUTTON << 4), wpcopy, GetMessageTime());
    break;
  case WM_NCMBUTTONDOWN:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_MBUTTONDOWN:
    SetCapture(hWnd);
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDOWN | (GUI_M_BUTTON << 4), wpcopy, GetMessageTime());
    break;
  case WM_NCMBUTTONUP:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_MBUTTONUP:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEUP | (GUI_M_BUTTON << 4), wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_MBUTTONDBLCLK:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDBLCLICK | (GUI_M_BUTTON << 4), wpcopy, GetMessageTime());
    break;
  case WM_NCXBUTTONDOWN:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONDOWN:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDOWN | (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? (GUI_X_BUTTON1 << 4) : (GUI_X_BUTTON2 << 4)), wpcopy, GetMessageTime());
    break;
  case WM_NCXBUTTONUP:
    if(!(self->_guiflags&PSGUIMANAGER_HOOKNC)) break;
    wpcopy = (unsigned int)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONUP:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEUP | (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? (GUI_X_BUTTON1 << 4) : (GUI_X_BUTTON2 << 4)), wpcopy, GetMessageTime());
    break;
  case WM_XBUTTONDBLCLK:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSEDBLCLICK | (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? (GUI_X_BUTTON1 << 4) : (GUI_X_BUTTON2 << 4)), wpcopy, GetMessageTime());
    break;
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:
    //if(wParam==VK_MENU) //if we don't handle the alt key press, it freezes our program :C
    //{
    //  BYTE allKeys[256]; 
    //GetKeyboardState(allKeys);
    //  (cEngine::Instance()->*winhook_setkey)((unsigned char)wParam,0, 0, message==WM_SYSKEYDOWN, allKeys);
    //  return 0;
    //} //do not break to allow syskeys to drop down
  case WM_KEYUP:
  case WM_KEYDOWN:
  {
    psEngine::Instance()->SetKey((unsigned char)wParam, message == WM_KEYDOWN || message == WM_SYSKEYDOWN, (lParam & 0x40000000) != 0, GetMessageTime());
    //  BYTE allKeys[256]; //Note that the virtual key codes do not exceed 256, so we can actually convert wParam to unsigned char without data loss
    //  WORD asciikey=0;
    //  wchar_t unicodekey[4];
    //GetKeyboardState(allKeys);
    //ToAscii((UINT)wParam,(UINT)lParam,allKeys,&asciikey,0);
    //int retval = ToUnicode((UINT)wParam,(UINT)lParam,allKeys,unicodekey,4,0);
    //  
    //  if(retval<=0)
    //    (cEngine::Instance()->*winhook_setkey)((unsigned char)wParam,(char)asciikey, (wchar_t)0, message==WM_KEYDOWN||message==WM_SYSKEYDOWN, allKeys);
    //  else
    //    for(int i = 0; i < retval; ++i) //iterate through each key written and produce a seperate event for it
    //      (cEngine::Instance()->*winhook_setkey)((unsigned char)wParam,(char)asciikey, (wchar_t)unicodekey[i], message==WM_KEYDOWN||message==WM_SYSKEYDOWN, allKeys);
  }
  return 0;
  case WM_SYSCOMMAND:
    if((wParam & 0xFFF0) == SC_SCREENSAVE || (wParam & 0xFFF0) == SC_MONITORPOWER)
      return 0; //No screensavers!
    break;
  case WM_MOUSELEAVE:
    psEngine::Instance()->SetMouse(MAKELPPOINTS(lParam), GUI_MOUSELEAVE, 0, GetMessageTime());
    self->_guiflags -= PSGUIMANAGER_ISINSIDE;
    break;
  case WM_ACTIVATE:
    if(self->_guiflags&PSGUIMANAGER_AUTOMINIMIZE && LOWORD(wParam) == WA_INACTIVE)
      ShowWindow(hWnd, SW_SHOWMINIMIZED);
    if(self->_guiflags&PSGUIMANAGER_LOCKCURSOR) {
      switch(LOWORD(wParam))
      {
      case WA_ACTIVE:
      case WA_CLICKACTIVE:
        _lockcursor(hWnd, true);
        break;
      case WA_INACTIVE:
        _lockcursor(hWnd, false);
        break;
      }
    }
    break;
  case WM_NCHITTEST:
    if(self->_guiflags&PSGUIMANAGER_OVERRIDEHITTEST) return HTCAPTION;
    break;
  case WM_UNICHAR:
    if(wParam==UNICODE_NOCHAR) return TRUE;
  case WM_CHAR:
    psEngine::Instance()->SetChar((int)wParam, GetMessageTime());
    return 0;
    //case 124:
    //case 125:
    //case 127:
    //case 174:
    //case 12:
    //case 32:
    //  break;
    //default:
    //  OutputDebugString(cStr("\n%i",message));
    //  message=message;
    //  break;
  case WM_SIZE:
    if(self->_guiflags&PSGUIMANAGER_LOCKCURSOR)
      self->_dolockcursor(hWnd);
    //self->_onresize(LOWORD(lParam), HIWORD(lParam));
    break;
  }

  return DefWindowProcW(hWnd, message, wParam, lParam);
}

typedef HRESULT(STDAPICALLTYPE *DWMCOMPENABLE)(BOOL*);
typedef HRESULT(STDAPICALLTYPE *DWMEXTENDFRAME)(HWND, const MARGINS*);
typedef HRESULT(STDAPICALLTYPE *DWMBLURBEHIND)(HWND, const DWM_BLURBEHIND*);
DWMEXTENDFRAME dwmextend = 0;
DWMBLURBEHIND dwmblurbehind = 0;

//PS_COMP_NOFRAME_CLICKTHROUGH = 2,
//PS_COMP_NOFRAME_NOMOVE = 3,
//PS_COMP_NOFRAME_OPAQUE_CLICK = 4,

HWND psGUIManager::WndCreate(HINSTANCE instance, psVeciu dim, char mode, const wchar_t* icon, HICON iconrc)
{
  PROFILE_FUNC();
  HINSTANCE hInstance = GetModuleHandleW(0);

  if(instance)
    hInstance = instance;

  //Register class
  WNDCLASSEXW wcex    ={ sizeof(WNDCLASSEXW),              // cbSize
    CS_HREDRAW | CS_VREDRAW, //| CS_DBLCLKS, // Double clicking usually just causes problems
    (WNDPROC)WndProc,                      // lpfnWndProc
    NULL,                            // cbClsExtra
    NULL,                            // cbWndExtra
    hInstance,                       // hInstance
    iconrc, // hIcon
    NULL,     // hCursor
    NULL,                            // hbrBackground
    NULL,                            // lpszMenuName
    L"PlaneShaderWindow",                    // lpszClassName
    iconrc };// hIconSm

  if(icon)
    wcex.hIcon = (HICON)LoadImageW(hInstance, icon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

  RegisterClassExW(&wcex);

  //Get size
  RECT rsize;
  rsize.top = 0;
  rsize.left = 0;
  rsize.right = dim.x;
  rsize.bottom = dim.y;

  unsigned long style = WS_POPUP;

  if(mode == PSINIT::MODE_WINDOWED)
    style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX;
  if(mode >= PSINIT::MODE_BORDERLESS)
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
  
  AdjustWindowRect(&rsize, style, FALSE);
  int rwidth = rsize.right - rsize.left;
  int rheight = rsize.bottom - rsize.top;
  int wleft = (GetSystemMetrics(SM_CXSCREEN) - rwidth) / 2;
  int wtop = (GetSystemMetrics(SM_CYSCREEN) - rheight) / 2;
  bool clickthrough = mode == PSINIT::MODE_COMPOSITE_CLICKTHROUGH;
  bool nomove = mode == PSINIT::MODE_COMPOSITE_NOMOVE;
  bool opaqueclick = mode == PSINIT::MODE_COMPOSITE_OPAQUE_CLICK;

  HWND hWnd;
  if(mode >= PSINIT::MODE_COMPOSITE)
    hWnd = CreateWindowExW((clickthrough || opaqueclick)?WS_EX_TRANSPARENT|WS_EX_COMPOSITED|WS_EX_LAYERED: WS_EX_COMPOSITED,
      L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);
  else
    hWnd = CreateWindowW(L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);

  if(mode >= PSINIT::MODE_COMPOSITE)
  {
    //MARGINS margins = { -1,-1,-1,-1 };
    //(*dwmextend)(hWnd, &margins); //extends glass effect
    HRGN region = CreateRectRgn(-1,-1,0,0);
    DWM_BLURBEHIND blurbehind = { DWM_BB_ENABLE|DWM_BB_BLURREGION|DWM_BB_TRANSITIONONMAXIMIZED, TRUE, region, FALSE };
    (*dwmblurbehind)(hWnd, &blurbehind);
  }

  HWND top = HWND_TOP;
  switch(mode)
  {
  case PSINIT::MODE_BORDERLESS: top = HWND_NOTOPMOST; break;
  case PSINIT::MODE_COMPOSITE_CLICKTHROUGH:
  case PSINIT::MODE_COMPOSITE_NOMOVE:
  case PSINIT::MODE_BORDERLESS_TOPMOST: top = HWND_TOPMOST; break;
  case PSINIT::MODE_FULLSCREEN: top = HWND_TOP; break;
  }

  ShowWindow(hWnd, clickthrough?SW_SHOWNOACTIVATE:SW_SHOW);
  UpdateWindow(hWnd);
  SetWindowPos(hWnd, top, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOACTIVATE);

  if(clickthrough || opaqueclick) SetLayeredWindowAttributes(hWnd, 0, 0xFF, LWA_ALPHA);
  else
  {
    SetActiveWindow(hWnd);
    SetForegroundWindow(hWnd);
  }
  SetCursor(LoadCursor(NULL, IDC_ARROW));

  return hWnd;
}

psGUIManager::psGUIManager() : _receiver(0,0), _firstjoystick(0), _alljoysticks(0), _maxjoy(JOYSTICK_ID16), _window(0)
{
  _mousedata.scrolldelta = 0;
  GetKeyboardState(_allkeys);
  memset(&_allbuttons, 0, sizeof(uint32_t)*NUMJOY);
  memset(&_alljoyaxis, 0, sizeof(unsigned long)*NUMJOY*NUMAXIS);
  memset(&_joydevs, 0, sizeof(JOY_DEVCAPS)*NUMJOY);

  _joyupdateall();
  _exactmousecalc();
}
psGUIManager::~psGUIManager()
{
  PROFILE_FUNC();
  ChangeDisplaySettings(NULL, 0);
  ShowCursor(TRUE);

  if(_window)
    DestroyWindow(_window);

  UnregisterClassW(L"PlaneShaderWindow", GetModuleHandle(0));
}
void psGUIManager::SetKey(unsigned char keycode, bool down, bool held, DWORD time)
{
  PROFILE_FUNC();
  GetKeyboardState(_allkeys);
  //bool held=down && _getkey(keycode);
  _allkeys[keycode]=down?(_allkeys[keycode]|0x08):(_allkeys[keycode]&(~0x08)); //For sanity, ensure this matches what we just got.

  psGUIEvent evt;
  evt.type = down?GUI_KEYDOWN:GUI_KEYUP;
  evt.time=time;
  evt.keycode = keycode;
  evt.sigkeys = 0;
  if(GetKey(KEY_SHIFT)) evt.sigkeys = evt.sigkeys|1; //VK_SHIFT
  if(GetKey(KEY_CONTROL)) evt.sigkeys = evt.sigkeys|2; //VK_CONTROL
  if(GetKey(KEY_MENU)) evt.sigkeys = evt.sigkeys|4; //VK_MENU
  if(held) evt.sigkeys = evt.sigkeys|8;
  //_root->OnEventTrigger(evt);

  if(!_receiver.IsEmpty())
    _receiver(evt);
}
void psGUIManager::SetChar(int key, DWORD time)
{
  PROFILE_FUNC();
  GetKeyboardState(_allkeys);

  psGUIEvent evt;
  evt.type = GUI_KEYCHAR;
  evt.time=time;
  evt.keychar = key;
  evt.sigkeys = 0;

  if(GetKey(KEY_SHIFT)) evt.sigkeys = evt.sigkeys|1; //VK_SHIFT
  if(GetKey(KEY_CONTROL)) evt.sigkeys = evt.sigkeys|2; //VK_CONTROL
  if(GetKey(KEY_MENU)) evt.sigkeys = evt.sigkeys|4; //VK_MENU
  //if(held) evt.sigkeys = evt.sigkeys|8;

  //_root->OnEventTrigger(evt);

  if(!_receiver.IsEmpty())
    _receiver(evt);
}

void psGUIManager::SetMouse(POINTS* points, unsigned char click, size_t wparam, DWORD time)
{
  PROFILE_FUNC();
  psGUIEvent evt;
  evt.time=time;
  evt.type = (GUI_EVENT)(click&15);
  evt.button = (GUI_EVENT)(click>>4);
  evt.x = _mousedata.relcoord.x; // Set these up here for any mouse events that don't contain coordinates.
  evt.y = _mousedata.relcoord.y;

  if(click==GUI_MOUSELEAVE)
  {
    //_root->LeftWindow();
    if(!_receiver.IsEmpty())
      _receiver(evt);
    return;
  }

  if(wparam!=(size_t)-1) //if wparam is -1 it signals that it is invalid, so we simply leave our assignments at their last known value.
  {
    unsigned char bt=0; //we must keep these bools accurate at all times
    bt |= GUI_L_BUTTON&(-((wparam&MK_LBUTTON)!=0));
    bt |= GUI_R_BUTTON&(-((wparam&MK_RBUTTON)!=0));
    bt |= GUI_M_BUTTON&(-((wparam&MK_MBUTTON)!=0));
    bt |= GUI_X_BUTTON1&(-((wparam&MK_XBUTTON1)!=0));
    bt |= GUI_X_BUTTON2&(-((wparam&MK_XBUTTON2)!=0));
    _mousedata.button=bt;
    _allkeys[KEY_LBUTTON]=_mousedata.button[GUI_L_BUTTON]*0x80;
    _allkeys[KEY_RBUTTON]=_mousedata.button[GUI_R_BUTTON]*0x80;
    _allkeys[KEY_MBUTTON]=_mousedata.button[GUI_M_BUTTON]*0x80;
    _allkeys[KEY_XBUTTON1]=_mousedata.button[GUI_X_BUTTON1]*0x80;
    _allkeys[KEY_XBUTTON2]=_mousedata.button[GUI_X_BUTTON2]*0x80;
  }

  if(click != GUI_MOUSESCROLL) //The WM_MOUSEWHEEL event does not send mousecoord data
  {
    evt.x=points->x;
    evt.y=points->y;
    evt.allbtn=_mousedata.button;
    _mousedata.relcoord.x = points->x;
    _mousedata.relcoord.y = points->y;
  }

  switch(click)
  {
  case GUI_MOUSESCROLL:
    evt.scrolldelta = ((((wparam) >> 16) & 0xffff)); //HIWORD()
    break;
  case GUI_MOUSEDOWN: //L down
    evt.allbtn |= evt.button; // Ensure the correct button position is reflected in the allbtn parameter regardless of the current state of the mouse
    break;
  case GUI_MOUSEUP: //L up
    evt.allbtn &= ~evt.button;
    break;
  }

  if(!_receiver.IsEmpty())
    _receiver(evt);
}
// Locks the cursor
void psGUIManager::LockCursor(bool lock)
{
  PROFILE_FUNC();
  _guiflags[PSGUIMANAGER_LOCKCURSOR]=lock;
  _lockcursor(_window, lock);
}
// Shows/hides the hardware cursor
void psGUIManager::ShowCursor(bool show)
{
  ::ShowCursor(show?1:0);
}

char psGUIManager::CaptureAllJoy(HWND__* hwnd)
{
  PROFILE_FUNC();
  char r=0;
  _alljoysticks=0;
  for(unsigned short i = 0; i < _maxjoy; ++i)
  {
    if(joySetCapture(hwnd, i, 0, TRUE)==JOYERR_NOERROR)
    {
      ++r;
      _alljoysticks|=(1<<i);
      if(_firstjoystick==JOYSTICK_INVALID)
        _firstjoystick=(i<<8);
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
    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    if(msg.message == WM_QUIT)
    {
      psEngine::Instance()->Quit();
      PSLOGV(3, "Quit message recieved, setting _quit to true");
      return;
    }
  }

  _joyupdateall();
  _exactmousecalc(); // Recalculate mouse AFTER the messages, so that all events get mouse coordinates based on where the mouse was when they happened, but we end up with accurate coordinates.
}

void psGUIManager::SetWindowTitle(const char* caption)
{
  PROFILE_FUNC();
  SetWindowText(_window, caption);
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
  info.dwSize=sizeof(JOYINFOEX);
  info.dwFlags=JOY_RETURNBUTTONS|JOY_RETURNCENTERED|JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_RETURNR|JOY_RETURNU|JOY_RETURNV;

  for(unsigned short i = 0; i < _maxjoy; ++i)
  {
    if(!(_alljoysticks & (1<<i))) continue;
    if(joyGetPosEx(i, &info)==JOYERR_NOERROR)
    {
      if(_allbuttons[i]!=info.dwButtons)
      {
        psGUIEvent evt;
        evt.time=GetTickCount();
        unsigned int old=_allbuttons[i];
        _allbuttons[i]=info.dwButtons;
        unsigned int diff=(old^info.dwButtons);
        unsigned int k;
        if(!_receiver.IsEmpty()) //Only bother with events if we have something to send them to
        {
          for(int j = 0; j < 32; ++j) //go through the bits and generate BUTTONUP or BUTTONDOWN events
          {
            k=(1<<j);
            if((diff&k)!=0)
            {
              evt.joydown=(info.dwButtons&k)!=0;
              evt.type=(evt.joydown!=0)?GUI_JOYBUTTONDOWN:GUI_JOYBUTTONUP;
              evt.joybutton=(i<<8)|j;
              _receiver(evt);
            }
          }
        }
      }

      unsigned char numaxes=_joydevs[i].numaxes;
      if(memcmp(&_alljoyaxis[i][JOYAXIS_X], &info.dwXpos, sizeof(unsigned long)*numaxes)!=0)
      {
        unsigned long old[NUMAXIS];
        memcpy(old, &_alljoyaxis[i][JOYAXIS_X], sizeof(unsigned long)*numaxes);
        memcpy(&_alljoyaxis[i][JOYAXIS_X], &info.dwXpos, sizeof(unsigned long)*numaxes);

        if(!_receiver.IsEmpty()) //Only bother with events if we have something to send them to
        {
          for(int j = 0; j < numaxes; ++j)
          {
            if(old[j]!=_alljoyaxis[i][j])
            {
              psGUIEvent evt;
              evt.time=GetTickCount();
              evt.type=GUI_JOYAXIS;
              evt.joyaxis=(i<<8)|j;
              evt.joyvalue=_translatejoyaxis(evt.joyaxis);
              _receiver(evt);
            }
          }
        }
      }
    } else
      _alljoysticks &= (~(1<<i)); // If it failed, remove joystick from active list
  }
}

float psGUIManager::_translatejoyaxis(unsigned short axis) const
{
  unsigned char ID = (axis>>8);
  unsigned char a = (axis&0xFF);
  return (((long)_alljoyaxis[ID][a])-_joydevs[ID].offset[a])/_joydevs[ID].range[a];
}

psVeciu psGUIManager::_create(psVeciu dim, char mode, HWND window)
{
  PROFILE_FUNC();
  // Check for desktop composition
  HMODULE dwm=LoadLibraryW(L"dwmapi.dll");
  if(dwm)
  {
    DWMCOMPENABLE dwmcomp = (DWMCOMPENABLE)GetProcAddress(dwm, "DwmIsCompositionEnabled");
    if(!dwmcomp) { FreeLibrary(dwm); dwm=0; } else
    {
      BOOL res;
      (*dwmcomp)(&res);
      if(res==FALSE) { FreeLibrary(dwm); dwm=0; } //fail
    }
    dwmextend = (DWMEXTENDFRAME)GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
    dwmblurbehind = (DWMBLURBEHIND)GetProcAddress(dwm, "DwmEnableBlurBehindWindow");

    if(!dwmextend || !dwmblurbehind)
    { 
      FreeLibrary(dwm);
      dwm = 0;
      dwmextend = 0;
      dwmblurbehind = 0;
    }
  }

  if(!dwm && mode >= PSINIT::MODE_COMPOSITE) mode = PSINIT::MODE_WINDOWED; //can't do composite if its not supported
  _guiflags[PSGUIMANAGER_AUTOMINIMIZE] = mode == PSINIT::MODE_BORDERLESS_TOPMOST || mode == PSINIT::MODE_FULLSCREEN;
  _guiflags[PSGUIMANAGER_LOCKCURSOR] = mode == PSINIT::MODE_BORDERLESS || mode == PSINIT::MODE_BORDERLESS_TOPMOST || mode == PSINIT::MODE_FULLSCREEN;
  _window=window;
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  if(!_window) _window = WndCreate(0, mode == PSINIT::MODE_BORDERLESS ? screen : dim, mode, 0, 0);

  return dim;
}

void psGUIManager::_exactmousecalc()
{
  PROFILE_FUNC();
  POINT p;
  //GetCursorPos(&p); //This fails for large addresses
  {
    CURSORINFO ci;
    ci.cbSize=sizeof(CURSORINFO);
    GetCursorInfo(&ci);
    p=ci.ptScreenPos;
  }

  ScreenToClient(_window, &p);
  _mousedata.relcoord.x = p.x;
  _mousedata.relcoord.y = p.y;
}

psVeciu psGUIManager::_resizewindow(psVeciu dim, char mode)
{
  PROFILE_FUNC();
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  RECT rect;
  GetClientRect(_window, &rect);
  rect.right = rect.left + ((mode == PSINIT::MODE_BORDERLESS) ? screen.x : dim.x);
  rect.bottom = rect.top + ((mode == PSINIT::MODE_BORDERLESS) ? screen.y : dim.y);

  unsigned long style = WS_POPUP;
  if(mode == PSINIT::MODE_WINDOWED)
    style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX;
  if(mode >= PSINIT::MODE_BORDERLESS)
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;

  AdjustWindowRect(&rect, style, FALSE);
  int rwidth = rect.right - rect.left;
  int rheight = rect.bottom - rect.top;

  _guiflags[PSGUIMANAGER_AUTOMINIMIZE] = mode == PSINIT::MODE_BORDERLESS_TOPMOST || PSINIT::MODE_FULLSCREEN;
  _guiflags[PSGUIMANAGER_LOCKCURSOR] = mode == PSINIT::MODE_BORDERLESS || mode == PSINIT::MODE_BORDERLESS_TOPMOST || mode == PSINIT::MODE_FULLSCREEN;
  SetWindowLong(_window, GWL_STYLE, style); //if we don't have the right style, we'll either get a borderless window or a fullscreen app with screwed up coordinates
  bool clickthrough = mode == PSINIT::MODE_COMPOSITE_CLICKTHROUGH;
  bool nomove = mode == PSINIT::MODE_COMPOSITE_NOMOVE;

  HWND top = HWND_TOP;
  switch(mode)
  {
  case PSINIT::MODE_BORDERLESS: top = HWND_NOTOPMOST; break;
  case PSINIT::MODE_COMPOSITE_CLICKTHROUGH:
  case PSINIT::MODE_COMPOSITE_NOMOVE:
  case PSINIT::MODE_BORDERLESS_TOPMOST: top = HWND_TOPMOST; break;
  case PSINIT::MODE_FULLSCREEN: top = HWND_TOP; break;
  }

  if(mode == PSINIT::MODE_BORDERLESS || mode == PSINIT::MODE_BORDERLESS_TOPMOST || mode == PSINIT::MODE_FULLSCREEN) //if its windowed, the screen resolution has now been reset so we move the window back
    SetWindowPos(_window, top, INT(0), INT(0), INT(rwidth), INT(rheight), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
  else
  {
    rect.left = (GetSystemMetrics(SM_CXSCREEN) - rwidth) / 2;
    rect.top = (GetSystemMetrics(SM_CYSCREEN) - rheight) / 2;

    SetWindowPos(_window, top, INT(rect.left), INT(rect.top), INT(rwidth), INT(rheight), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
  }

  _lockcursor(_window, (_guiflags&PSGUIMANAGER_LOCKCURSOR) != 0);
  return dim;
}

void psGUIManager::_dolockcursor(HWND__* hWnd)
{
  _lockcursor(hWnd, GetActiveWindow() == hWnd);
}
