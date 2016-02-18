// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEngine.h"
#include "psFont.h"
#include "psTex.h"
#include "feathergui\fgButton.h"
#include "feathergui\fgText.h"
#include "feathergui\fgResource.h"
#include "feathergui\fgTopWindow.h"
#include "ps_feather.h"

#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/feathergui_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/feathergui.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../lib/feathergui32_d.lib")
#else
#pragma comment(lib, "../lib/feathergui32.lib")
#endif

using namespace planeshader;

psRoot* psRoot::instance = 0;

void* FG_FASTCALL fgCreateFont(fgFlag flags, const char* font, unsigned int fontsize, float lineheight, float letterspacing)
{ 
  return psFont::Create(font, fontsize, lineheight, (flags&FGTEXT_SUBPIXEL) ? psFont::FAA_LCD : psFont::FAA_ANTIALIAS);
}
void* FG_FASTCALL fgCloneFont(void* font) { ((psFont*)font)->Grab(); return font; }
void FG_FASTCALL fgDestroyFont(void* font) { ((psFont*)font)->Drop(); }
void FG_FASTCALL fgDrawFont(void* font, const char* text, unsigned int color, const AbsRect* area, FABS rotation, AbsVec* center, fgFlag flags)
{ 
  psFont* f = (psFont*)font;
  psRect rect = { area->left, area->top, area->right, area->bottom };
  f->DrawText(text, rect, psRoot::GetDrawFlags(flags), 0, color, 0);
}
void FG_FASTCALL fgFontSize(void* font, const char* text, AbsRect* area, fgFlag flags)
{
  psFont* f = (psFont*)font;
  psVec dim = { area->right - area->left, area->bottom - area->top };
  if(flags&FGCHILD_EXPANDX) dim.x = -1.0f;
  if(flags&FGCHILD_EXPANDY) dim.y = -1.0f;
  f->CalcTextDim(cStrT<int>(text), dim, psRoot::GetDrawFlags(flags), 0.0f);
  area->right = area->left + dim.x;
  area->bottom = area->top + dim.y;
}

void* FG_FASTCALL fgCreateResource(fgFlag flags, const char* data, size_t length) { return psTex::Create(data, length, USAGE_SHADER_RESOURCE, FILTER_ALPHABOX); }
void* FG_FASTCALL fgCloneResource(void* res) { ((psTex*)res)->Grab(); return res; }
void FG_FASTCALL fgDestroyResource(void* res) { ((psTex*)res)->Drop(); }
void FG_FASTCALL fgDrawResource(void* res, const CRect* uv, unsigned int color, const AbsRect* area, FABS rotation, AbsVec* center, fgFlag flags)
{
  psTex* tex = (psTex*)res;
  psRect uvresolve = { uv->left.rel + (uv->left.abs/tex->GetDim().x),
    uv->top.rel + (uv->top.abs / tex->GetDim().y),
    uv->right.rel + (uv->right.abs / tex->GetDim().x),
    uv->bottom.rel+ (uv->bottom.abs / tex->GetDim().y) };

  psRoot::Instance()->GetDriver()->library.IMAGE->Activate();
  psRoot::Instance()->GetDriver()->SetStateblock(0);
  psRoot::Instance()->GetDriver()->SetTextures(&tex, 1);
  psRoot::Instance()->GetDriver()->DrawRect(psRectRotate(area->left, area->top, area->right, area->bottom, rotation, psVec(center->x, center->y)), &uvresolve, 1, color, 0);
}
void FG_FASTCALL fgResourceSize(void* res, const CRect* uv, AbsVec* dim, fgFlag flags)
{
  psTex* tex = (psTex*)res;
  psRect uvresolve = { (uv->left.rel*tex->GetDim().x) + uv->left.abs,
    (uv->top.rel*tex->GetDim().y) + uv->top.abs,
    (uv->right.rel*tex->GetDim().x) + uv->right.abs,
    (uv->bottom.rel*tex->GetDim().y) + uv->bottom.abs };
  dim->x = uvresolve.right - uvresolve.left;
  dim->y = uvresolve.bottom - uvresolve.top;
}

#define DEFAULT_CREATE(type, init, ...) \
  type* r = (type*)malloc(sizeof(type)); \
  init(r, __VA_ARGS__); \
  ((fgChild*)r)->free = &free;

fgChild* FG_FASTCALL fgResource_Create(void* res, const CRect* uv, unsigned int color, fgFlag flags, fgChild* BSS_RESTRICT parent, fgChild* BSS_RESTRICT prev, const fgElement* element)
{
  DEFAULT_CREATE(fgResource, fgResource_Init, res, uv, color, flags, parent, prev, element);
  return (fgChild*)r;
}
fgChild* FG_FASTCALL fgText_Create(char* text, void* font, unsigned int color, fgFlag flags, fgChild* BSS_RESTRICT parent, fgChild* BSS_RESTRICT prev, const fgElement* element)
{
  DEFAULT_CREATE(fgText, fgText_Init, text, font, color, flags, parent, prev, element);
  return (fgChild*)r;
}
fgChild* FG_FASTCALL fgButton_Create(const char* text, fgFlag flags, fgChild* BSS_RESTRICT parent, fgChild* BSS_RESTRICT prev, const fgElement* element)
{
  DEFAULT_CREATE(fgButton, fgButton_Init, flags, parent, prev, element);
  if(text)
    fgChild_VoidMessage((fgChild*)r, FG_SETTEXT, (void*)text);
  return (fgChild*)r;
}
fgChild* FG_FASTCALL fgTopWindow_Create(const char* caption, fgFlag flags, const fgElement* element)
{
  fgTopWindow* r = (fgTopWindow*)malloc(sizeof(fgTopWindow));
  fgTopWindow_Init(r, flags, element);
  fgChild_VoidMessage((fgChild*)r, FG_SETPARENT, fgSingleton());
  fgChild_VoidMessage((fgChild*)r, FG_SETTEXT, (void*)caption);
  r->window.element.free = &free;
  return (fgChild*)r;
}

fgRoot* FG_FASTCALL fgInitialize()
{
  new psRoot();
  return fgSingleton();
}

char FG_FASTCALL fgLoadExtension(void* fg, const char* extname) { return -1; }

void fgPushClipRect(AbsRect* clip)
{ 
  psRect rect = { clip->left, clip->top, clip->right, clip->bottom };
  psRoot::Instance()->GetDriver()->PushScissorRect(rect);
}

void fgPopClipRect()
{
  psRoot::Instance()->GetDriver()->PopScissorRect();
}

psRoot::psRoot() : _prev(0,0)
{
  fgRoot_Init(&_root);
  _root.gui.element.element.area.right.abs = _driver->screendim.x;
  _root.gui.element.element.area.bottom.abs = _driver->screendim.y;
  if(instance)
    delete instance;
  instance = this;
  _prev = psEngine::Instance()->GetInputReceiver();
  psEngine::Instance()->SetInputReceiver(bss_util::delegate<bool, const psGUIEvent&>::From<psRoot, &psRoot::ProcessGUI>(this));
}
psRoot::~psRoot()
{
  fgRoot_Destroy(&_root);
  psEngine::Instance()->SetInputReceiver(_prev);
  instance = 0;
}
bool psRoot::ProcessGUI(const psGUIEvent& evt)
{
  FG_Msg msg;
  memcpy(&msg, &evt, sizeof(FG_Msg));

  switch(evt.type)
  {
  case GUI_MOUSEDOWN: msg.type = FG_MOUSEDOWN; break;
  case GUI_MOUSEDBLCLICK: msg.type = FG_MOUSEDBLCLICK; break;
  case GUI_MOUSEUP: msg.type = FG_MOUSEUP; break;
  case GUI_MOUSEMOVE: msg.type = FG_MOUSEMOVE; break;
  case GUI_MOUSESCROLL: msg.type = FG_MOUSESCROLL; break;
  case GUI_MOUSELEAVE: msg.type = FG_MOUSELEAVE; break;
  case GUI_KEYUP: msg.type = FG_KEYUP; break;
  case GUI_KEYDOWN: msg.type = FG_KEYDOWN; break;
  case GUI_KEYCHAR: msg.type = FG_KEYCHAR; break;
  case GUI_JOYBUTTONDOWN: msg.type = FG_JOYBUTTONDOWN; break;
  case GUI_JOYBUTTONUP: msg.type = FG_JOYBUTTONUP; break;
  case GUI_JOYAXIS: msg.type = FG_JOYAXIS; break;
  }

  if(fgRoot_Inject(&_root, &msg))
    return !_prev.IsEmpty() ? _prev(evt) : false;
  return true;
}

void BSS_FASTCALL psRoot::_render(psBatchObj*)
{
  CRect area = _root.gui.element.element.area;
  area.right.abs = _driver->screendim.x;
  area.bottom.abs = _driver->screendim.y;
  fgChild_VoidMessage(&_root.gui.element, FG_SETAREA, &area);
  fgChild_VoidMessage(&_root.gui.element, FG_DRAW, 0);
}
psFlag psRoot::GetDrawFlags(fgFlag flags)
{
  psFlag flag = 0;
  if(flags&FGTEXT_CHARWRAP) flag |= TDT_CHARBREAK;
  if(flags&FGTEXT_WORDWRAP) flag |= TDT_WORDBREAK;
  if(flags&FGTEXT_RTL) flag |= TDT_RTL;
  if(flags&FGTEXT_RIGHTALIGN) flag |= TDT_RIGHT;
  if(flags&FGTEXT_CENTER) flag |= TDT_CENTER;
  return flag;
}

psRoot* psRoot::Instance()
{
  return instance;
}
