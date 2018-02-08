// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "bss-util/Str.h"
#include "feathergui/fgWindow.h"

#include "win32_includes.h"
#include <Mmsystem.h>
#include <dwmapi.h>

using namespace planeshader;

typedef HRESULT(STDAPICALLTYPE *DWMCOMPENABLE)(BOOL*);
typedef HRESULT(STDAPICALLTYPE *DWMEXTENDFRAME)(HWND, const MARGINS*);
typedef HRESULT(STDAPICALLTYPE *DWMBLURBEHIND)(HWND, const DWM_BLURBEHIND*);
HINSTANCE__* psMonitor::dwm;
DWMEXTENDFRAME psMonitor::dwmextend = 0;
DWMBLURBEHIND psMonitor::dwmblurbehind = 0;

#define MAKELPPOINTS(l)       ((POINTS FAR *)&(l))

psMonitor::psMonitor() : _manager(0), _window(0) {}
psMonitor::psMonitor(psGUIManager* manager, psVeciu& dim, MODE mode, HWND__* window) : _manager(manager), _window(window), _mode(mode), _backbuffer(0)
{
  if(!dwm && mode >= MODE_COMPOSITE) mode = MODE_WINDOWED; //can't do composite if its not supported
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  RECT rect;
  if(!_window)
  {
    rect.left = 0;
    rect.top = 0;
    rect.right = mode == MODE_BORDERLESS ? screen.x : dim.x;
    rect.bottom = mode == MODE_BORDERLESS ? screen.y : dim.y;
    _window = WndCreate(0, rect, mode, FGWINDOW_MINIMIZABLE | FGWINDOW_MAXIMIZABLE | ((mode == MODE_WINDOWED) ? FGWINDOW_RESIZABLE : 0), 0);
  }

  GetClientRect(_window, &rect);
  AbsRect r = { 0, 0, rect.right - rect.left, rect.bottom - rect.top };
  psVeciu dpi = psGUIManager::GetMonitorDPI(0);
  AbsVec fgdpi = { dpi.x, dpi.y };
  fgMonitor_Init(this, FGFLAGS_INTERNAL|FGELEMENT_BACKGROUND, &manager->GetGUI(), 0, &r, &fgdpi);
  _manager->_updaterootarea();
  this->element.message = (fgMessage)&Message;
  this->element.destroy = (fgDestroy)&Destroy;
}
psMonitor::~psMonitor()
{
  this->element.destroy = (fgDestroy)&fgMonitor_Destroy;
  if(_window)
    DestroyWindow(_window);
  fgMonitor_Destroy(this);
}
void psMonitor::Destroy(psMonitor* self)
{
  auto& m = psEngine::Instance()->_monitors;
  for(uint8_t i = 0; i < m.Length(); ++i)
    if(&m[i] == self)
    {
      m.Remove(i);
      return;
    }
}

size_t psMonitor::Message(fgMonitor* s, const FG_Msg* m)
{
  psMonitor* self = static_cast<psMonitor*>(s);
  fgFlag otherint = (fgFlag)m->u;
  fgFlag flags = self->element.flags;

  switch(m->type)
  {
  case FG_MOVE:
    self->_manager->_updaterootarea();
    break;
  case FG_SETTEXT:
    SetWindowTextW(self->_window, bss::StrW((const char*)m->p).c_str());
    return 1;
  case FG_SETFLAG: // Do the same thing fgElement does to resolve a SETFLAG into SETFLAGS
    otherint = bss::bssSetBit<fgFlag>(flags, otherint, m->u2 != 0);
  case FG_SETFLAGS:
    if((otherint^flags) & (FGWINDOW_MINIMIZABLE| FGWINDOW_MAXIMIZABLE | FGWINDOW_RESIZABLE | FGWINDOW_NOCAPTION | FGWINDOW_NOBORDER))
    { // handle a layout flag change
      size_t r = fgMonitor_Message(s, m); // we have to actually set the flags first before updating the window
      RECT rect;
      GetClientRect(self->_window, &rect);
      self->WndCreate(0, rect, self->_mode, 0, self->_window);
      return r;
    }
    break;
  }

  return fgMonitor_Message(s, m);
}

// Locks the cursor
void psMonitor::LockCursor(bool lock)
{
  _guiflags[PSMONITOR_LOCKCURSOR] = lock;
  _lockcursor(_window, lock);
}

void psMonitor::Resize(psVeciu dim, MODE mode)
{
  psVeciu screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  dim = psVeciu(!dim.x ? screen.x : dim.x, !dim.y ? screen.y : dim.y);
  RECT rect;
  GetClientRect(_window, &rect);
  rect.right = rect.left + ((mode == MODE_BORDERLESS) ? screen.x : dim.x);
  rect.bottom = rect.top + ((mode == MODE_BORDERLESS) ? screen.y : dim.y);

  WndCreate(0, rect, mode, element.flags, _window);
}

void psMonitor::WndRegister(HINSTANCE instance, const wchar_t* icon, HICON iconrc)
{
  HINSTANCE hInstance = !instance ? GetModuleHandleW(0) : instance;

  //Register class
  WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW),              // cbSize
    CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC,
    (WNDPROC)psMonitor::WndProc,                      // lpfnWndProc
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
}

HWND psMonitor::WndCreate(HINSTANCE instance, RECT& rsize, MODE mode, fgFlag flags, HWND hWnd)
{
  HINSTANCE hInstance = !instance ? GetModuleHandleW(0) : instance;
  unsigned long style = WS_POPUP;
  _mode = mode;

  if(mode == MODE_WINDOWED)
  {
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if(!(flags & FGWINDOW_NOCAPTION)) style |= WS_CAPTION | WS_SYSMENU;
    if(!(flags & FGWINDOW_NOBORDER)) style |= WS_BORDER;
  }
  if(mode >= MODE_BORDERLESS)
    style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;

  if(flags & FGWINDOW_MINIMIZABLE) style |= WS_MINIMIZEBOX;
  if(flags & FGWINDOW_MAXIMIZABLE) style |= WS_MAXIMIZEBOX;
  if(flags & FGWINDOW_RESIZABLE) style |= WS_SIZEBOX;

  AdjustWindowRect(&rsize, style, FALSE);
  int rwidth = rsize.right - rsize.left;
  int rheight = rsize.bottom - rsize.top;
  int wleft = (GetSystemMetrics(SM_CXSCREEN) - rwidth) / 2;
  int wtop = (GetSystemMetrics(SM_CYSCREEN) - rheight) / 2;
  bool clickthrough = mode == MODE_COMPOSITE_CLICKTHROUGH;
  bool nomove = mode == MODE_COMPOSITE_NOMOVE;
  bool opaqueclick = mode == MODE_COMPOSITE_OPAQUE_CLICK;
  bool created = !hWnd;

  if(!hWnd)
  {
    if(mode >= MODE_COMPOSITE)
      hWnd = CreateWindowExW((clickthrough || opaqueclick) ? WS_EX_TRANSPARENT | WS_EX_COMPOSITED | WS_EX_LAYERED : WS_EX_COMPOSITED,
        L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);
    else
      hWnd = CreateWindowW(L"PlaneShaderWindow", L"", style, INT(wleft), INT(wtop), INT(rwidth), INT(rheight), NULL, NULL, hInstance, NULL);
  }
  else
    SetWindowLong(hWnd, GWL_STYLE, style);

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
  _lockcursor(_window, (_guiflags&PSMONITOR_LOCKCURSOR) != 0);
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

WPARAM MapLRKeys(WPARAM vk, LPARAM lParam)
{
  UINT scancode = (lParam & 0x00ff0000) >> 16;
  int extended = (lParam & 0x01000000) != 0;

  switch(vk)
  {
  case VK_SHIFT:
    return MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
  case VK_CONTROL:
    return extended ? VK_RCONTROL : VK_LCONTROL;
  case VK_MENU:
    return extended ? VK_RMENU : VK_LMENU;
  }

  return vk;
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
  case WM_DESTROY:
    gui->Quit();
    PostQuitMessage(0);
    break;
  case WM_MOUSEWHEEL:
  {
    POINTS pointstemp = { 0 };
    pointstemp.y = GET_WHEEL_DELTA_WPARAM(wpcopy);
    gui->SetMouse(&pointstemp, FG_MOUSESCROLL, 0, wpcopy, GetMessageTime());
  }
  break;
  case 0x020E: // WM_MOUSEHWHEEL - this is only sent on vista machines, but our minimum version is XP, so we manually look for the message anyway and process it if it happens to get sent to us.
  {
    POINTS pointstemp = { 0 };
    pointstemp.x = GET_WHEEL_DELTA_WPARAM(wpcopy);
    gui->SetMouse(&pointstemp, FG_MOUSESCROLL, 0, wpcopy, GetMessageTime());
  }
  break;
  case WM_NCMOUSEMOVE:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam)); //This is a bit weird but it works because we get passed lParam by value, so this function actually ends up modifying lParam directly.
  case WM_MOUSEMOVE:
    if(!(self->_guiflags&PSMONITOR_ISINSIDE))
    {
      _trackingstruct.hwndTrack = hWnd;
      BOOL result = TrackMouseEvent(&_trackingstruct);
      self->_guiflags += PSMONITOR_ISINSIDE;
      gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEON, 0, wpcopy, GetMessageTime());
    }
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEMOVE, 0, wpcopy, GetMessageTime());
    break;
  case WM_NCLBUTTONDOWN:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_LBUTTONDOWN:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSELBUTTON, wpcopy, GetMessageTime());
    SetCapture(hWnd);
    break;
  case WM_NCLBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
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
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_RBUTTONDOWN:
    SetCapture(hWnd);
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSERBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCRBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
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
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_MBUTTONDOWN:
    SetCapture(hWnd);
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, FG_MOUSEMBUTTON, wpcopy, GetMessageTime());
    break;
  case WM_NCMBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
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
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONDOWN:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDOWN, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break;
  case WM_NCXBUTTONUP:
    if(!(self->_guiflags&PSMONITOR_HOOKNC)) break;
    wpcopy = (WPARAM)~0;
    _STCpoints(hWnd, MAKELPPOINTS(lParam));
  case WM_XBUTTONUP:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEUP, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break;
  case WM_XBUTTONDBLCLK:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEDBLCLICK, (GET_XBUTTON_WPARAM(wpcopy) == XBUTTON1 ? FG_MOUSEXBUTTON1 : FG_MOUSEXBUTTON2), wpcopy, GetMessageTime());
    break; 
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:
    gui->SetKey((uint8_t)MapLRKeys(wParam, lParam), message == WM_SYSKEYDOWN, (lParam & 0x40000000) != 0, (lParam & 0x00ff0000) >> 16, GetMessageTime());
    break;
  case WM_KEYUP:
  case WM_KEYDOWN: // Windows return codes are the opposite of feathergui's - returning 0 means we accept, anything else rejects, so we invert the return code here.
    return !gui->SetKey((uint8_t)MapLRKeys(wParam, lParam), message == WM_KEYDOWN, (lParam & 0x40000000) != 0, (lParam & 0x00ff0000) >> 16, GetMessageTime());
  case WM_UNICHAR:
    if(wParam == UNICODE_NOCHAR) return TRUE;
  case WM_CHAR:
    gui->SetChar((int)wParam, GetMessageTime());
    return 0;
  case WM_SYSCOMMAND:
    if((wParam & 0xFFF0) == SC_SCREENSAVE || (wParam & 0xFFF0) == SC_MONITORPOWER)
      return 0; //No screensavers!
    break;
  case WM_MOUSELEAVE:
    gui->SetMouse(MAKELPPOINTS(lParam), FG_MOUSEOFF, 0, 0, GetMessageTime());
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
    //case 124:
    //case 125:
    //case 127:
    //case 174:
    //case 12:
    //case 32:
    //  break;
    //default:
    //  OutputDebugString(Str("\n%i",message));
    //  message=message;
    //  break;
  case WM_SIZE:
  {
    if(self->_guiflags&PSMONITOR_LOCKCURSOR)
      _lockcursor(hWnd, GetActiveWindow() == hWnd);
    RECT rect;
    GetClientRect(self->_window, &rect);
    AbsRect absrect = { (FABS)rect.left, (FABS)rect.top, (FABS)rect.right, (FABS)rect.bottom };
    fgSendSubMsg<FG_SETAREA, void*>(&self->element, 1, &absrect);
    self->_manager->_onresize(psVeciu(rect.right - rect.left, rect.bottom - rect.top), self->_mode == MODE_FULLSCREEN);
  }
    break;
  }

  return DefWindowProcW(hWnd, message, wParam, lParam);
}

void psMonitor::CheckDesktopComposition()
{
  // Check for desktop composition
  dwm = LoadLibraryW(L"dwmapi.dll");
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
}