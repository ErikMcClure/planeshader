// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEngine.h"
#include "psFont.h"
#include "psTex.h"
#include "psVector.h"
#include "feathergui\fgButton.h"
#include "feathergui\fgText.h"
#include "feathergui\fgResource.h"
#include "feathergui\fgWindow.h"
#include "feathergui\fgRadioButton.h"
#include "feathergui\fgProgressbar.h"
#include "feathergui\fgSlider.h"
#include "feathergui\fgList.h"
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

void* FG_FASTCALL fgCreateFont(fgFlag flags, const char* font, uint32_t fontsize, unsigned int dpi)
{ 
  return psFont::Create(font, fontsize, (flags&FGTEXT_SUBPIXEL) ? psFont::FAA_LCD : psFont::FAA_ANTIALIAS, dpi);
}
void* FG_FASTCALL fgCopyFont(void* font, unsigned int fontsize, unsigned int dpi)
{ 
  psFont* f = (psFont*)font;
  return psFont::Create(f->GetPath(), fontsize, f->GetAntialias(), dpi);
}
void* FG_FASTCALL fgCloneFontDPI(void* font, unsigned int dpi) { ((psFont*)font)->Grab(); return font; }
void* FG_FASTCALL fgCloneFont(void* font) { ((psFont*)font)->Grab(); return font; }
void FG_FASTCALL fgDestroyFont(void* font) { ((psFont*)font)->Drop(); }
void* FG_FASTCALL fgDrawFont(void* font, const char* text, float lineheight, float letterspacing, unsigned int color, const AbsRect* area, FABS rotation, AbsVec* center, fgFlag flags, void* cache)
{ 
  psFont* f = (psFont*)font;
  psRectRotateZ rect = { area->left, area->top, area->right, area->bottom, 0, {0, 0}, 0 };
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  if(f->GetAntialias() == psFont::FAA_LCD)
    f->DrawText(psDriverHold::GetDriver()->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
  else
    f->DrawText(psDriverHold::GetDriver()->library.IMAGE, 0, text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
  return 0;
}
void FG_FASTCALL fgFontSize(void* font, const char* text, float lineheight, float letterspacing, AbsRect* area, fgFlag flags)
{
  psFont* f = (psFont*)font;
  psVec dim = { area->right - area->left, area->bottom - area->top };
  if(flags&FGELEMENT_EXPANDX) dim.x = -1.0f;
  if(flags&FGELEMENT_EXPANDY) dim.y = -1.0f;
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  f->CalcTextDim(cStrT<int>(text), dim, lineheight, letterspacing, psRoot::GetDrawFlags(flags));
  area->right = area->left + dim.x;
  area->bottom = area->top + dim.y;
}
void FG_FASTCALL fgFontGet(void* font, float* lineheight, unsigned int* size, unsigned int* dpi)
{
  psFont* f = (psFont*)font;
  if(lineheight) *lineheight = f->GetLineHeight();
  if(size) *size = f->GetPointSize();
  if(dpi) *dpi = f->GetDPI();
}

void* FG_FASTCALL fgCreateResource(fgFlag flags, const char* data, size_t length) { return psTex::Create(data, length, USAGE_SHADER_RESOURCE, FILTER_ALPHABOX); }
void* FG_FASTCALL fgCloneResource(void* res) { ((psTex*)res)->Grab(); return res; }
void FG_FASTCALL fgDestroyResource(void* res) { ((psTex*)res)->Drop(); }
void FG_FASTCALL fgDrawResource(void* res, const CRect* uv, uint32_t color, uint32_t edge, FABS outline, const AbsRect* area, FABS rotation, AbsVec* center, fgFlag flags)
{
  psTex* tex = (psTex*)res;
  psRect uvresolve;
  if(tex)
  {
    uvresolve = psRect { uv->left.rel + (uv->left.abs / tex->GetDim().x),
      uv->top.rel + (uv->top.abs / tex->GetDim().y),
      uv->right.rel + (uv->right.abs / tex->GetDim().x),
      uv->bottom.rel + (uv->bottom.abs / tex->GetDim().y) };
  }
  else
    uvresolve = psRect { uv->left.abs, uv->top.abs, uv->right.abs, uv->bottom.abs };

  psDriver* driver = psDriverHold::GetDriver();
  if(tex)
    driver->SetTextures(&tex, 1);

  psRect hold = psDriverHold::GetDriver()->PeekClipRect();
  psRectRotate rect(area->left, area->top, area->right, area->bottom, rotation, psVec(center->x, center->y));

  if(flags&FGRESOURCE_ROUNDRECT)
    psRoundedRect::DrawRoundedRect(driver->library.ROUNDRECT, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else if(flags&FGRESOURCE_CIRCLE)
    psRenderCircle::DrawCircle(driver->library.CIRCLE, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else
    driver->DrawRect(driver->library.IMAGE, 0, rect, &uvresolve, 1, color, 0);
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
void FG_FASTCALL fgDrawLine(AbsVec p1, AbsVec p2, uint32_t color)
{
  psDriver* driver = psDriverHold::GetDriver();
  psBatchObj* obj = driver->DrawLinesStart(driver->library.LINE, 0, 0);
  unsigned long vertexcolor;
  psColor32(color).WriteFormat(FMT_R8G8B8A8, &vertexcolor);
  driver->DrawLines(obj, psLine(p1.x, p1.y, p2.x, p2.y), 0, 0, vertexcolor);
}
#define DEFAULT_CREATE(type, init, ...) \
  type* r = (type*)malloc(sizeof(type)); \
  init(r, __VA_ARGS__); \
  ((fgElement*)r)->free = &free;

fgElement* FG_FASTCALL fgResource_Create(void* res, const CRect* uv, uint32_t color, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgResource, fgResource_Init, res, uv, color, parent, prev, name, flags, transform);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgText_Create(char* text, void* font, uint32_t color, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgText, fgText_Init, text, font, color, parent, prev, name, flags, transform);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgButton_Create(const char* text, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgButton, fgButton_Init, parent, prev, name, flags, transform);
  (*r)->SetText(text);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgWindow_Create(const char* caption, fgFlag flags, const fgTransform* transform)
{
  fgWindow* r = (fgWindow*)malloc(sizeof(fgWindow));
  fgWindow_Init(r, flags, transform);
  (*r)->SetParent(*fgSingleton(), 0);
  (*r)->SetText(caption);
  r->control.element.free = &free;
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgCheckbox_Create(const char* text, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgCheckbox, fgCheckbox_Init, parent, prev, name, flags, transform);
  (*r)->SetText(text);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgRadiobutton_Create(const char* text, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgRadiobutton, fgRadiobutton_Init, parent, prev, name, flags, transform);
  (*r)->SetText(text);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgProgressbar_Create(FREL value, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgProgressbar, fgProgressbar_Init, parent, prev, name, flags, transform);
  fgIntMessage((fgElement*)r, FG_SETSTATE, *reinterpret_cast<ptrdiff_t*>(&value), 0);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgSlider_Create(size_t range, fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgSlider, fgSlider_Init, range, parent, prev, name, flags, transform);
  return (fgElement*)r;
}
fgElement* FG_FASTCALL fgBox_Create(fgElement* BSS_RESTRICT parent, fgElement* BSS_RESTRICT prev, const char* name, fgFlag flags, const fgTransform* transform)
{
  DEFAULT_CREATE(fgBox, fgBox_Init, parent, prev, name, flags, transform);
  return (fgElement*)r;
}


fgRoot* FG_FASTCALL fgInitialize()
{
  return fgSingleton();
}

char FG_FASTCALL fgLoadExtension(void* fg, const char* extname) { return -1; }

void fgPushClipRect(AbsRect* clip)
{ 
  psRect rect = { clip->left, clip->top, clip->right, clip->bottom };
  psDriverHold::GetDriver()->MergeClipRect(rect);
}

AbsRect fgPeekClipRect()
{
  psRect c = psDriverHold::GetDriver()->PeekClipRect();
  return AbsRect { c.left, c.top, c.right, c.bottom };
}

void fgPopClipRect()
{
  psDriverHold::GetDriver()->PopClipRect();
}

void fgDirtyElement(fgTransform* e)
{

}

psRoot::psRoot()
{
  fgRoot_Init(this, &AbsRect { 0, 0, 1, 1 }, psGUIManager::BASE_DPI);
}
psRoot::~psRoot()
{
  fgRoot_Destroy(this);
}

void BSS_FASTCALL psRoot::_render()
{
  CRect area = gui.element.transform.area;
  area.right.abs = _driver->screendim.x;
  area.bottom.abs = _driver->screendim.y;
  gui->SetArea(area);
  gui->Draw(0, 0);
}
psFlag psRoot::GetDrawFlags(fgFlag flags)
{
  psFlag flag = 0;
  if(flags&FGTEXT_CHARWRAP) flag |= PSFONT_CHARBREAK;
  if(flags&FGTEXT_WORDWRAP) flag |= PSFONT_WORDBREAK;
  if(flags&FGTEXT_RTL) flag |= PSFONT_RTL;
  if(flags&FGTEXT_RIGHTALIGN) flag |= PSFONT_RIGHT;
  if(flags&FGTEXT_CENTER) flag |= PSFONT_CENTER;
  return flag;
}

#include "bss-util\bss_win32_includes.h"

void fgSetCursor(uint32_t type, void* custom)
{
  static HCURSOR hArrow = LoadCursor(NULL, IDC_ARROW);
  static HCURSOR hIBeam = LoadCursor(NULL, IDC_IBEAM);
  static HCURSOR hCross = LoadCursor(NULL, IDC_CROSS);
  static HCURSOR hWait = LoadCursor(NULL, IDC_WAIT);
  //static HCURSOR hHand = LoadCursor(NULL, IDC_HAND);
  static HCURSOR hSizeNS = LoadCursor(NULL, IDC_SIZENS);
  static HCURSOR hSizeWE = LoadCursor(NULL, IDC_SIZEWE);
  static HCURSOR hSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
  static HCURSOR hSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
  static HCURSOR hSizeAll = LoadCursor(NULL, IDC_SIZEALL);
  static HCURSOR hNo = LoadCursor(NULL, IDC_NO);
  static HCURSOR hHelp = LoadCursor(NULL, IDC_HELP);

  switch(type)
  {
  case FGCURSOR_ARROW: SetCursor(hArrow); break;
  case FGCURSOR_IBEAM: SetCursor(hIBeam); break;
  case FGCURSOR_CROSS: SetCursor(hCross); break;
  case FGCURSOR_WAIT: SetCursor(hWait); break;
  //case FGCURSOR_HAND: SetCursor(hHand); break;
  case FGCURSOR_RESIZENS: SetCursor(hSizeNS); break;
  case FGCURSOR_RESIZEWE: SetCursor(hSizeWE); break;
  case FGCURSOR_RESIZENWSE: SetCursor(hSizeNWSE); break;
  case FGCURSOR_RESIZENESW: SetCursor(hSizeNESW); break;
  case FGCURSOR_RESIZEALL: SetCursor(hSizeAll); break;
  case FGCURSOR_NO: SetCursor(hNo); break;
  case FGCURSOR_HELP: SetCursor(hHelp); break;
  }
}

void fgClipboardCopy(uint32_t type, const void* data, size_t length)
{
  OpenClipboard(psEngine::Instance()->GetWindow());
  if(EmptyClipboard() && data != 0 && length > 0)
  {
    HGLOBAL gmem = GlobalAlloc(GMEM_MOVEABLE, length);
    if(gmem)
    {
      void* mem = GlobalLock(gmem);
      MEMCPY(mem, length, data, length);
      GlobalUnlock(gmem);
      UINT format = CF_PRIVATEFIRST;
      switch(type)
      {
      case FGCLIPBOARD_TEXT: 
      {
        format = CF_TEXT; // While our format is always CF_TEXT, because we use utf8, CF_UNICODE is also set for the benefit of other programs (windows assumes CF_TEXT is ascii, and so will not correctly translate it to unicode).
        cStrW text((const char*)data);
        size_t len = (text.length() + 1)*sizeof(wchar_t); // add one for null terminator
        HGLOBAL unimem = GlobalAlloc(GMEM_MOVEABLE, len);
        if(unimem)
        {
          void* uni = GlobalLock(unimem);
          MEMCPY(uni, len, text.c_str(), len);
          GlobalUnlock(unimem);
          SetClipboardData(CF_UNICODETEXT, unimem);
        }
      }
        break; 
      case FGCLIPBOARD_WAVE: format = CF_WAVE; break;
      case FGCLIPBOARD_BITMAP: format = CF_BITMAP; break;
      }
      SetClipboardData(format, gmem);
    }
  }
  CloseClipboard();
}

char fgClipboardExists(uint32_t type)
{
  switch(type)
  {
  case FGCLIPBOARD_TEXT:
    return IsClipboardFormatAvailable(CF_TEXT) | IsClipboardFormatAvailable(CF_UNICODETEXT);
  case FGCLIPBOARD_WAVE:
    return IsClipboardFormatAvailable(CF_WAVE);
  case FGCLIPBOARD_BITMAP:
    return IsClipboardFormatAvailable(CF_BITMAP);
  case FGCLIPBOARD_CUSTOM:
    return IsClipboardFormatAvailable(CF_PRIVATEFIRST);
  case FGCLIPBOARD_ALL:
    return IsClipboardFormatAvailable(CF_TEXT) | IsClipboardFormatAvailable(CF_UNICODETEXT) | IsClipboardFormatAvailable(CF_WAVE) | IsClipboardFormatAvailable(CF_BITMAP) | IsClipboardFormatAvailable(CF_PRIVATEFIRST);
  }
  return 0;
}

const void* fgClipboardPaste(uint32_t type, size_t* length)
{
  OpenClipboard(psEngine::Instance()->GetWindow());
  UINT format = CF_PRIVATEFIRST;
  switch(type)
  {
  case FGCLIPBOARD_TEXT:
    if(IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
      HANDLE gdata = GetClipboardData(CF_UNICODETEXT);
      const wchar_t* str = (const wchar_t*)GlobalLock(gdata);
      SIZE_T size = GlobalSize(gdata)/2;
      if(!str[size - 1]) // Do not read text that is not null terminated.
      {
        cStr s(str);
        size = s.length() + 1;
        void* ret = malloc(size);
        MEMCPY(ret, size, s.c_str(), size);
        GlobalUnlock(gdata);
        CloseClipboard();
        return ret;
      }
    }
    format = CF_TEXT; // otherwise if unicode isn't available directly paste from the ascii buffer.
    break;
  case FGCLIPBOARD_WAVE: format = CF_WAVE; break;
  case FGCLIPBOARD_BITMAP: format = CF_BITMAP; break;
  }
  HANDLE gdata = GetClipboardData(format);
  void* data = GlobalLock(gdata);
  SIZE_T size = GlobalSize(gdata);
  void* ret = malloc(size);
  MEMCPY(ret, size, data, size);
  GlobalUnlock(gdata);
  CloseClipboard();
  return ret;
}

void fgClipboardFree(const void* mem)
{
  free(const_cast<void*>(mem));
}