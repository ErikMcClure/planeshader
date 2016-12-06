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
void* FG_FASTCALL fgDrawFont(void* font, const int* text, float lineheight, float letterspacing, unsigned int color, const AbsRect* area, FABS rotation, const AbsVec* center, fgFlag flags, void* cache)
{
  psFont* f = (psFont*)font;
  psRectRotateZ rect = { area->left, area->top, area->right, area->bottom, rotation, {center->x - area->left, center->y - area->top}, 0 };
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  if(f->GetAntialias() == psFont::FAA_LCD)
    f->DrawText(psDriverHold::GetDriver()->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
  else
    f->DrawText(psDriverHold::GetDriver()->library.IMAGE, 0, text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
  return 0;
}
void FG_FASTCALL fgFontSize(void* font, const int* text, float lineheight, float letterspacing, AbsRect* area, fgFlag flags)
{
  psFont* f = (psFont*)font;
  psVec dim = { area->right - area->left, area->bottom - area->top };
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  f->CalcTextDim(text, dim, lineheight, letterspacing, psRoot::GetDrawFlags(flags));
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
size_t FG_FASTCALL fgFontIndex(void* font, const int* text, float lineheight, float letterspacing, const AbsRect* area, fgFlag flags, AbsVec pos, AbsVec* cursor, void* cache)
{
  psFont* f = (psFont*)font;
  auto r = f->GetIndex(text, area->right - area->left, psRoot::GetDrawFlags(flags), lineheight, letterspacing, psVec(pos.x, pos.y));
  cursor->x = r.second.x;
  cursor->y = r.second.y;
  return r.first;
}
AbsVec FG_FASTCALL fgFontPos(void* font, const int* text, float lineheight, float letterspacing, const AbsRect* area, fgFlag flags, size_t index, void* cache)
{
  psFont* f = (psFont*)font;
  auto r = f->GetPos(text, area->right - area->left, psRoot::GetDrawFlags(flags), lineheight, letterspacing, index);
  return AbsVec { r.second.x, r.second.y };
}

void* FG_FASTCALL fgCreateResource(fgFlag flags, const char* data, size_t length) { return psTex::Create(data, length, USAGE_SHADER_RESOURCE, FILTER_ALPHABOX); }
void* FG_FASTCALL fgCloneResource(void* res) { ((psTex*)res)->Grab(); return res; }
void FG_FASTCALL fgDestroyResource(void* res) { ((psTex*)res)->Drop(); }
void FG_FASTCALL fgDrawResource(void* res, const CRect* uv, unsigned int color, unsigned int edge, FABS outline, const AbsRect* area, FABS rotation, const AbsVec* center, fgFlag flags)
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
  psRectRotate rect(area->left, area->top, area->right, area->bottom, rotation, psVec(center->x - area->left, center->y - area->top));
  if(rotation != 0)
    rotation = rotation;

  if((flags&FGRESOURCE_SHAPEMASK) == FGRESOURCE_ROUNDRECT)
    psRoundedRect::DrawRoundedRect(driver->library.ROUNDRECT, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else if((flags&FGRESOURCE_SHAPEMASK) == FGRESOURCE_CIRCLE)
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

void FG_FASTCALL fgDrawLines(const AbsVec* p, size_t n, unsigned int color, const AbsVec* translate, const AbsVec* scale, FABS rotation, const AbsVec* center)
{
  psDriver* driver = psDriverHold::GetDriver();
  unsigned long vertexcolor;
  psColor32(color).WriteFormat(FMT_R8G8B8A8, &vertexcolor);
  float (&m)[4][4] = *driver->PushMatrix();
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(translate->x, translate->y, 0, rotation, center->x, center->y, m);

  if(n == 2)
  {
    psBatchObj* o = driver->DrawLinesStart(driver->library.LINE, 0, 0, m);
    driver->DrawLines(o, psLine { p[0].x, p[0].y, p[1].x, p[1].y }, 0, 0, vertexcolor);
  }
  else
  {
    DYNARRAY(psVertex, verts, n);
    for(uint32_t i = 0; i < n; ++i)
      verts[i] = { p[i].x, p[i].y, 0, 1, vertexcolor };

    psBatchObj* o = driver->DrawCurveStart(driver->library.LINE, 0, 0, m);
    driver->DrawCurve(o, verts, n);
  }
}

#include "bss-util/bss_win32_includes.h"
#include "bss-util/os.h"

fgRoot* FG_FASTCALL fgInitialize()
{
  return fgSingleton();
}

char FG_FASTCALL fgLoadExtension(void* fg, const char* extname) { return -1; }

void fgPushClipRect(const AbsRect* clip)
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

void fgDirtyElement(fgElement* e)
{

}

void fgSetCursor(uint32_t type, void* custom)
{
  static HCURSOR hArrow = LoadCursor(NULL, IDC_ARROW);
  static HCURSOR hIBeam = LoadCursor(NULL, IDC_IBEAM);
  static HCURSOR hCross = LoadCursor(NULL, IDC_CROSS);
  static HCURSOR hWait = LoadCursor(NULL, IDC_WAIT);
  static HCURSOR hHand = LoadCursor(NULL, IDC_HAND);
  static HCURSOR hSizeNS = LoadCursor(NULL, IDC_SIZENS);
  static HCURSOR hSizeWE = LoadCursor(NULL, IDC_SIZEWE);
  static HCURSOR hSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
  static HCURSOR hSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
  static HCURSOR hSizeAll = LoadCursor(NULL, IDC_SIZEALL);
  static HCURSOR hNo = LoadCursor(NULL, IDC_NO);
  static HCURSOR hHelp = LoadCursor(NULL, IDC_HELP);
  static HCURSOR hDrag = hSizeAll;

  switch(type)
  {
  case FGCURSOR_ARROW: SetCursor(hArrow); break;
  case FGCURSOR_IBEAM: SetCursor(hIBeam); break;
  case FGCURSOR_CROSS: SetCursor(hCross); break;
  case FGCURSOR_WAIT: SetCursor(hWait); break;
  case FGCURSOR_HAND: SetCursor(hHand); break;
  case FGCURSOR_RESIZENS: SetCursor(hSizeNS); break;
  case FGCURSOR_RESIZEWE: SetCursor(hSizeWE); break;
  case FGCURSOR_RESIZENWSE: SetCursor(hSizeNWSE); break;
  case FGCURSOR_RESIZENESW: SetCursor(hSizeNESW); break;
  case FGCURSOR_RESIZEALL: SetCursor(hSizeAll); break;
  case FGCURSOR_NO: SetCursor(hNo); break;
  case FGCURSOR_HELP: SetCursor(hHelp); break;
  case FGCURSOR_DRAG: SetCursor(hDrag); break;
  }
}

void fgClipboardCopy(uint32_t type, const void* data, size_t length)
{
  OpenClipboard(psEngine::Instance()->GetMonitor()->GetWindow());
  if(EmptyClipboard() && data != 0 && length > 0)
  {
    if(type == FGCLIPBOARD_TEXT)
    {
      size_t len = UTF32toUTF8((const int*)data, length / sizeof(int), 0, 0);
      size_t unilen = UTF32toUTF16((const int*)data, length / sizeof(int), 0, 0);
      HGLOBAL unimem = GlobalAlloc(GMEM_MOVEABLE, unilen * sizeof(wchar_t));
      if(unimem)
      {
        wchar_t* uni = (wchar_t*)GlobalLock(unimem);
        UTF32toUTF16((const int*)data, length / sizeof(int), uni, unilen);
        GlobalUnlock(unimem);
        SetClipboardData(CF_UNICODETEXT, unimem);
      }
      HGLOBAL gmem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(char));
      if(gmem)
      {
        char* mem = (char*)GlobalLock(gmem);
        UTF32toUTF8((const int*)data, length / sizeof(int), mem, len);
        GlobalUnlock(gmem);
        SetClipboardData(CF_TEXT, gmem);
      }
    }
    else
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
        case FGCLIPBOARD_WAVE: format = CF_WAVE; break;
        case FGCLIPBOARD_BITMAP: format = CF_BITMAP; break;
        }
        SetClipboardData(format, gmem);
      }
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
  OpenClipboard(psEngine::Instance()->GetMonitor()->GetWindow());
  UINT format = CF_PRIVATEFIRST;
  switch(type)
  {
  case FGCLIPBOARD_TEXT:
    if(IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
      HANDLE gdata = GetClipboardData(CF_UNICODETEXT);
      const wchar_t* str = (const wchar_t*)GlobalLock(gdata);
      SIZE_T size = GlobalSize(gdata) / 2;
      SIZE_T len = UTF16toUTF32(str, size, 0, 0);
      int* ret = bss_util::bssmalloc<int>(len);
      UTF16toUTF32(str, size, ret, len);
      GlobalUnlock(gdata);
      CloseClipboard();
      return ret;
    }
    {
      HANDLE gdata = GetClipboardData(CF_TEXT);
      const char* str = (const char*)GlobalLock(gdata);
      SIZE_T size = GlobalSize(gdata);
      SIZE_T len = UTF8toUTF32(str, size, 0, 0);
      int* ret = bss_util::bssmalloc<int>(len);
      UTF8toUTF32(str, size, ret, len);
      GlobalUnlock(gdata);
      CloseClipboard();
      return ret;
    }
    return 0;
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

void fgDragStart(char type, void* data, fgElement* draw)
{
  fgRoot* root = fgSingleton();
  root->dragtype = type;
  root->dragdata = data;
  root->dragdraw = draw;
}

psRoot::psRoot()
{
  AbsRect area = { 0, 0, 1, 1 };

  static fgBackend BACKEND = {
    &fgBehaviorHookListener,
    &fgCreateFont,
    &fgCopyFont,
    &fgCloneFont,
    &fgDestroyFont,
    &fgDrawFont,
    &fgFontSize,
    &fgFontGet,
    &fgCreateResource,
    &fgCloneResource,
    &fgDestroyResource,
    &fgDrawResource,
    &fgResourceSize,
    &fgFontIndex,
    &fgFontPos,
    &fgDrawLines,
    &fgCreateDefault,
    &fgMessageMapDefault,
    &fgUserDataMapCallbacks,
    &fgPushClipRect,
    &fgPeekClipRect,
    &fgPopClipRect,
    &fgDragStart,
    &fgSetCursor,
    &fgClipboardCopy,
    &fgClipboardExists,
    &fgClipboardPaste,
    &fgClipboardFree,
    &fgDirtyElement,
    0,
    0,
  };
  fgRoot_Init(this, &area, psGUIManager::BASE_DPI, &BACKEND);
  DWORD blinkrate = 0;
  int64_t sz = bss_util::GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", 0, 0);
  if(sz > 0)
  {
    DYNARRAY(wchar_t, buf, sz / 2);
    sz = bss_util::GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", (unsigned char*)buf, sz);
    if(sz > 0)
      cursorblink = atoi(cStr(buf, sz / 2)) / 1000.0;
  }
}
psRoot::~psRoot()
{
  fgRoot_Destroy(this);
}

void BSS_FASTCALL psRoot::_render()
{
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