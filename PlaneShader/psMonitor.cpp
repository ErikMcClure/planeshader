// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psGUIManager.h"
#include "bss-util/bss_win32_includes.h"
#include "bss-util/cStr.h"
#include <Mmsystem.h>
#include <dwmapi.h>

typedef HRESULT(STDAPICALLTYPE *DWMCOMPENABLE)(BOOL*);
typedef HRESULT(STDAPICALLTYPE *DWMEXTENDFRAME)(HWND, const MARGINS*);
typedef HRESULT(STDAPICALLTYPE *DWMBLURBEHIND)(HWND, const DWM_BLURBEHIND*);
DWMEXTENDFRAME dwmextend = 0;
DWMBLURBEHIND dwmblurbehind = 0;

#define MAKELPPOINTS(l)       ((POINTS FAR *)&(l))

using namespace planeshader;

psMonitor::psMonitor() : _manager(0), _window(0) {}
psMonitor::psMonitor(psGUIManager* manager, psVeciu& dim, MODE mode, HWND__* window) : _manager(manager), _window(window), _mode(mode), _backbuffer(0)
{
  // Check for desktop composition
  HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
  if(dwm)
  {
    DWMCOMPENABLE dwmcomp = (DWMCOMPENABLE)GetProcAddress(dwm, "DwmIsCompositionEnabled");
    if(!dwmcomp) { FreeLibrary(dwm); dwm = 0; }
    else
    {
      BOOL res;
      (*dwmcomp)(&res);
      if(res == FALSE) { FreeLibrary(dwm); dwm = 0; } //fail
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

  if(!dwm && mode >= MODE_COMPOSITE) mode = MODE_WINDOWED; //can't do composite if its not supported
  _guiflags[PSMONITOR_AUTOMINIMIZE] = mode == MODE_BORDERLESS_TOPMOST || mode == MODE_FULLSCREEN;
  _guiflags[PSMONITOR_LOCKCURSOR] = mode == MODE_BORDERLESS || mode == MODE_BORDERLESS_TOPMOST || mode == MODE_FULLSCREEN;
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  if(!_window) _window = WndCreate(0, mode == MODE_BORDERLESS ? screen : dim, mode, 0, 0);

  RECT rect;
  GetClientRect(_window, &rect);
  AbsRect r = { 0, 0, rect.right - rect.left, rect.bottom - rect.top };
  fgMonitor_Init(this, 0, &manager->GetGUI(), 0, &r, psGUIManager::GetMonitorDPI(0).y);
  _manager->_updaterootarea();
  this->element.message = (FN_MESSAGE)&Message;
}
psMonitor::~psMonitor()
{
  if(_window)
    DestroyWindow(_window);

  UnregisterClassW(L"PlaneShaderWindow", GetModuleHandle(0));
}
size_t FG_FASTCALL psMonitor::Message(fgMonitor* s, const FG_Msg* m)
{
  psMonitor* self = static_cast<psMonitor*>(s);

  switch(m->type)
  {
  case FG_MOVE:
    self->_manager->_updaterootarea();
    break;
  case FG_SETTEXT:
    self->SetWindowTitle((const char*)m->other);
    return 1;
  }

  return fgMonitor_Message(s, m);
}

// Locks the cursor
void psMonitor::LockCursor(bool lock)
{
  _guiflags[PSMONITOR_LOCKCURSOR] = lock;
  _lockcursor(_window, lock);
}

// Set window title
void psMonitor::SetWindowTitle(const char* caption)
{
  SetWindowTextW(_window, cStrW(caption).c_str());
}

psVeciu psMonitor::_resizewindow(psVeciu dim, char mode)
{
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  RECT rect;
  GetClientRect(_window, &rect);
  rect.right = rect.left + ((mode == MODE_BORDERLESS) ? screen.x : dim.x);
  rect.bottom = rect.top + ((mode == MODE_BORDERLESS) ? screen.y : dim.y);

  unsigned long style = WS_POPUP;
  if(mode == MODE_WINDOWED)
    style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX;
  if(mode >= MODE_BORDERLESS)
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;

  AdjustWindowRect(&rect, style, FALSE);
  int rwidth = rect.right - rect.left;
  int rheight = rect.bottom - rect.top;

  _guiflags[PSMONITOR_AUTOMINIMIZE] = mode == MODE_BORDERLESS_TOPMOST || MODE_FULLSCREEN;
  _guiflags[PSMONITOR_LOCKCURSOR] = mode == MODE_BORDERLESS || mode == MODE_BORDERLESS_TOPMOST || mode == MODE_FULLSCREEN;
  SetWindowLong(_window, GWL_STYLE, style); //if we don't have the right style, we'll either get a borderless window or a fullscreen app with screwed up coordinates
  bool clickthrough = mode == MODE_COMPOSITE_CLICKTHROUGH;
  bool nomove = mode == MODE_COMPOSITE_NOMOVE;

  HWND top = HWND_TOP;
  switch(mode)
  {
  case MODE_BORDERLESS: top = HWND_NOTOPMOST; break;
  case MODE_COMPOSITE_CLICKTHROUGH:
  case MODE_COMPOSITE_NOMOVE:
  case MODE_BORDERLESS_TOPMOST: top = HWND_TOPMOST; break;
  case MODE_FULLSCREEN: top = HWND_TOP; break;
  }

  if(mode == MODE_BORDERLESS || mode == MODE_BORDERLESS_TOPMOST || mode == MODE_FULLSCREEN) //if its windowed, the screen resolution has now been reset so we move the window back
    SetWindowPos(_window, top, INT(0), INT(0), INT(rwidth), INT(rheight), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
  else
  {
    rect.left = (GetSystemMetrics(SM_CXSCREEN) - rwidth) / 2;
    rect.top = (GetSystemMetrics(SM_CYSCREEN) - rheight) / 2;

    SetWindowPos(_window, top, INT(rect.left), INT(rect.top), INT(rwidth), INT(rheight), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
  }

  GetClientRect(_window, &rect);
  element.SetArea(CRect { 0, 0, 0, 0, FABS(rect.right - rect.left), 0, FABS(rect.bottom - rect.top), 0 });
  _lockcursor(_window, (_guiflags&PSMONITOR_LOCKCURSOR) != 0);
  return dim;
}

HWND psMonitor::WndCreate(HINSTANCE instance, psVeciu dim, char mode, const wchar_t* icon, HICON iconrc)
{
  HINSTANCE hInstance = GetModuleHandleW(0);

  if(instance)
    hInstance = instance;

  //Register class
  WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW),              // cbSize
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

  if(mode == MODE_WINDOWED)
    style = WS_SYSMENU | WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX;
  if(mode >= MODE_BORDERLESS)
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;

  AdjustWindowRect(&rsize, style, FALSE);
  int rwidth = rsize.right - rsize.left;
  int rheight = rsize.bottom - rsize.top;
  int wleft = (GetSystemMetrics(SM_CXSCREEN) - rwidth) / 2;
  int wtop = (GetSystemMetrics(SM_CYSCREEN) - rheight) / 2;
  bool clickthrough = mode == MODE_COMPOSITE_CLICKTHROUGH;
  bool nomove = mode == MODE_COMPOSITE_NOMOVE;
  bool opaqueclick = mode == MODE_COMPOSITE_OPAQUE_CLICK;

  HWND hWnd;
  if(mode >= MODE_COMPOSITE)
    hWnd = CreateWindowExW((clickthrough || opaqueclick) ? WS_EX_TRANSPARENT | WS_EX_COMPOSITED | WS_EX_LAYERED : WS_EX_COMPOSITED,
      L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);
  else
    hWnd = CreateWindowW(L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);

  if(mode >= MODE_COMPOSITE)
  {
    //MARGINS margins = { -1,-1,-1,-1 };
    //(*dwmextend)(hWnd, &margins); //extends glass effect
    HRGN region = CreateRectRgn(-1, -1, 0, 0);
    DWM_BLURBEHIND blurbehind = { DWM_BB_ENABLE | DWM_BB_BLURREGION | DWM_BB_TRANSITIONONMAXIMIZED, TRUE, region, FALSE };
    (*dwmblurbehind)(hWnd, &blurbehind);
  }

  HWND top = HWND_TOP;
  switch(mode)
  {
  case MODE_BORDERLESS: top = HWND_NOTOPMOST; break;
  case MODE_COMPOSITE_CLICKTHROUGH:
  case MODE_COMPOSITE_NOMOVE:
  case MODE_BORDERLESS_TOPMOST: top = HWND_TOPMOST; break;
  case MODE_FULLSCREEN: top = HWND_TOP; break;
  }

  ShowWindow(hWnd, clickthrough ? SW_SHOWNOACTIVATE : SW_SHOW);
  UpdateWindow(hWnd);
  SetWindowPos(hWnd, top, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOACTIVATE);

  if(clickthrough || opaqueclick) SetLayeredWindowAttributes(hWnd, 0, 0xFF, LWA_ALPHA);
  else
  {
    SetActiveWindow(hWnd);
    SetForegroundWindow(hWnd);
  }
  SetCursor(LoadCursor(NULL, IDC_ARROW));
  SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  return hWnd;
}

void psMonitor::_lockcursor(HWND hWnd, bool lock)
{
  if(!lock || GetActiveWindow() != hWnd)
    ClipCursor(0);
  else
  {
    RECT holdrect;
    memset(&holdrect, 0, sizeof(RECT));
    AdjustWindowRect(&holdrect, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    RECT winrect;
    GetWindowRect(hWnd, &winrect);
    winrect.left -= holdrect.left;
    winrect.top -= holdrect.top;
    winrect.right -= holdrect.right;
    winrect.bottom -= holdrect.bottom;
    ClipCursor(&winrect);
  }
}

POINTS* __stdcall psMonitor::_STCpoints(HWND hWnd, POINTS* target)
{
  POINT pp = { target->x, target->y };
  ScreenToClient(hWnd, &pp);
  target->x = (SHORT)pp.x;
  target->y = (SHORT)pp.y;
  return target;
}

LRESULT __stdcall psMonitor::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static tagTRACKMOUSEEVENT _trackingstruct = { sizeof(tagTRACKMOUSEEVENT), TME_LEAVE, 0, 0 };
  WPARAM wpcopy = wParam;
  psMonitor* self = reinterpret_cast<psMonitor*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
  psGUIManager* gui = !self ? 0 : self->_manager;
  if(!self)
    return DefWindowProcW(hWnd, message, wParam, lParam);

  switch(message)
  {
    //case WM_PAINT:
    //cEngine::Instance()->Render();
    //return 0;
    //break;
  case WM_DESTROY:
    gui->Quit();
    PostQuitMessage(0);
    break;
  case WM_MOUSEWHEEL:
  {
    POINTS pointstemp;
    pointstemp.x = GET_WHEEL_DELTA_WPARAM(wpcopy);
    gui->SetMouse(&pointstemp, FG_MOUSESCROLL, 0, wpcopy, GetMessageTime());
  }
  break;
  case WM_NCMOUSEMOVE:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam)); //This is a bit weird but it works because we get passed lParam by value, so this function actually ends up modifying lParam directly.
  case WM_MOUSEMOVE:
    if(!(self->_guiflags&PSMONITOR_ISINSIDE))
    {
      _trackingstruct.hwndTrack = hWnd;
      BOOL result = TrackMouseEvent(&_trackingstruct);
      self->_guiflags += PSMONITOR_ISINSIDE;
    }
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEMOVE, 0, wpcopy, GetMessageTime());
    break;
  case WM_NCLBUTTONDOWN:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_LBUTTONDOWN:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSELBUTTON, wpcopy, GetMessageTime());
    SetCapture(hWnd);
    break;
  case WM_NCLBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_LBUTTONUP:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEUP, FG_MOUSELBUTTON, wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_LBUTTONDBLCLK:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDBLCLICK, FG_MOUSELBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCRBUTTONDOWN:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_RBUTTONDOWN:
    SetCapture(hWnd);
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSERBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCRBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_RBUTTONUP:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEUP, FG_MOUSERBUTTON, wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_RBUTTONDBLCLK:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDBLCLICK, FG_MOUSERBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCMBUTTONDOWN:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_MBUTTONDOWN:
    SetCapture(hWnd);
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSEMBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCMBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_MBUTTONUP:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEUP, FG_MOUSEMBUTTON, wpcopy, GetMessageTime());
    ReleaseCapture();
    break;
  case WM_MBUTTONDBLCLK:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDBLCLICK, FG_MOUSEMBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCXBUTTONDOWN:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONDOWN:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break;
  case WM_NCXBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (uint32_t)-1;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONUP:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEUP, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break;
  case WM_XBUTTONDBLCLK:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDBLCLICK, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break;
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:
    //if(wParam==VK_MENU) //if we don't handle the alt key press, it freezes our program :C
    //{
    //  BYTE allKeys[256]; 
    //GetKeyboardState(allKeys);
    //  (cEngine::Instance()->*winhook_setkey)((uint8_t)wParam,0, 0, message==WM_SYSKEYDOWN, allKeys);
    //  return 0;
    //} //do not break to allow syskeys to drop down
  case WM_KEYUP:
  case WM_KEYDOWN:
  {
    gui->SetKey((uint8_t)wParam, message == WM_KEYDOWN || message == WM_SYSKEYDOWN, (lParam & 0x40000000) != 0, GetMessageTime());
  }
  return 0;
  case WM_SYSCOMMAND:
    if((wParam & 0xFFF0) == SC_SCREENSAVE || (wParam & 0xFFF0) == SC_MONITORPOWER)
      return 0; //No screensavers!
    break;
  case WM_MOUSELEAVE:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSELEAVE, 0, 0, GetMessageTime());
    self->_guiflags -= PSMONITOR_ISINSIDE;
    break;
  case WM_ACTIVATE:
    if(self->_guiflags&PSMONITOR_AUTOMINIMIZE && LOWORD(wParam) == WA_INACTIVE)
      ShowWindow(hWnd, SW_SHOWMINIMIZED);
    if(self->_guiflags&PSMONITOR_LOCKCURSOR)
    {
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
    if(self->_guiflags&PSMONITOR_OVERRIDEHITTEST) return HTCAPTION;
    break;
  case WM_UNICHAR:
    if(wParam == UNICODE_NOCHAR) return TRUE;
  case WM_CHAR:
    gui->SetChar((int)wParam, GetMessageTime());
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
    if(self->_guiflags&PSMONITOR_LOCKCURSOR)
      _lockcursor(hWnd, GetActiveWindow() == hWnd);
    //self->_onresize(LOWORD(lParam), HIWORD(lParam));
    break;
  }

  return DefWindowProcW(hWnd, message, wParam, lParam);
}