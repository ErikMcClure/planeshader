// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "ps_feather.h"
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

#undef DrawText

#ifdef BSS_COMPILER_MSC
#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64) 
#pragma comment(lib, "feathergui_d.lib") 
#elif defined(BSS_CPU_x86_64) 
#pragma comment(lib, "feathergui.lib") 
#elif defined(BSS_DEBUG) 
#pragma comment(lib, "feathergui32_d.lib") 
#else 
#pragma comment(lib, "feathergui32.lib") 
#endif 
#endif

using namespace planeshader;

const int UNICODE_TERMINATOR = 0;

void* fgCreateFontPS(fgFlag flags, const char* family, short weight, char italic, unsigned int size, const AbsVec* dpi)
{
  return psFont::Create(family, weight, italic, size, (flags&FGTEXT_SUBPIXEL) ? psFont::FAA_LCD : psFont::FAA_ANTIALIAS, psVeciu(dpi->x, dpi->y));
}
void* fgCloneFontPS(void* font, const struct _FG_FONT_DESC* desc)
{
  psFont* f = (psFont*)font;
  if(!desc)
  {
    f->Grab();
    return f;
  }
  return psFont::Create(f->GetPath(), desc->pt, f->GetAntialias(), psVeciu(desc->dpi.x, desc->dpi.y));
}
void fgDestroyFontPS(void* font) { ((psFont*)font)->Drop(); }
void fgDrawFontPS(void* font, const void* text, size_t len, float lineheight, float letterspacing, unsigned int color, const AbsRect* dpiarea, FABS rotation, const AbsVec* dpicenter, fgFlag flags, const fgDrawAuxData* data, void* layout)
{
  psFont* f = (psFont*)font;
  AbsRect area;
  AbsVec center;
  fgResolveDrawRect(dpiarea, &area, dpicenter, &center, flags, data);
  psRectRotateZ rect = { area.left, area.top, area.right, area.bottom, rotation,{ center.x - area.left, center.y - area.top }, 0 };
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  if(f->GetAntialias() == psFont::FAA_LCD)
    f->DrawText(psDriverHold::GetDriver()->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, !len ? &UNICODE_TERMINATOR : (const int*)text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
  else
    f->DrawText(psDriverHold::GetDriver()->library.IMAGE, 0, !len ? &UNICODE_TERMINATOR : (const int*)text, lineheight, letterspacing, rect, color, psRoot::GetDrawFlags(flags));
}
void* fgFontLayoutPS(void* font, const void* text, size_t len, float lineheight, float letterspacing, AbsRect* area, fgFlag flags, const AbsVec* dpi, void* prevlayout)
{
  psFont* f = (psFont*)font;
  fgScaleRectDPI(area, dpi->x, dpi->y);
  psVec dim = { area->right - area->left, area->bottom - area->top };
  if(lineheight == 0.0f) lineheight = f->GetLineHeight();
  f->CalcTextDim(!text ? &UNICODE_TERMINATOR : (const int*)text, dim, lineheight, letterspacing, psRoot::GetDrawFlags(flags));
  area->right = area->left + dim.x;
  area->bottom = area->top + dim.y;
  fgInvScaleRectDPI(area, dpi->x, dpi->y);
  return 0;
}
void fgFontGetPS(void* font, fgFontDesc* desc)
{
  psFont* f = (psFont*)font;
  if(desc)
  {
    desc->dpi.x = f->GetDPI().x;
    desc->dpi.y = f->GetDPI().y;
    desc->pt = f->GetPointSize();
    desc->lineheight = f->GetLineHeight();
  }
}
size_t fgFontIndexPS(void* font, const void* text, size_t len, float lineheight, float letterspacing, const AbsRect* dpiarea, fgFlag flags, AbsVec pos, AbsVec* cursor, const AbsVec* dpi, void* layout)
{
  psFont* f = (psFont*)font;
  AbsRect area = *dpiarea;
  fgScaleRectDPI(&area, dpi->x, dpi->y);
  fgScaleVecDPI(&pos, dpi->x, dpi->y);
  auto r = f->GetIndex(!text ? &UNICODE_TERMINATOR : (const int*)text, area.right - area.left, psRoot::GetDrawFlags(flags), lineheight, letterspacing, psVec(pos.x, pos.y));
  cursor->x = r.second.x;
  cursor->y = r.second.y;
  fgInvScaleVecDPI(cursor, dpi->x, dpi->y);
  return r.first;
}
AbsVec fgFontPosPS(void* font, const void* text, size_t len, float lineheight, float letterspacing, const AbsRect* dpiarea, fgFlag flags, size_t index, const AbsVec* dpi, void* layout)
{
  psFont* f = (psFont*)font;
  AbsRect area = *dpiarea;
  fgScaleRectDPI(&area, dpi->x, dpi->y);
  auto r = f->GetPos(!text ? &UNICODE_TERMINATOR : (const int*)text, area.right - area.left, psRoot::GetDrawFlags(flags), lineheight, letterspacing, index);
  AbsVec c = { r.second.x, r.second.y };
  fgInvScaleVecDPI(&c, dpi->x, dpi->y);
  return c;
}

void*  fgCreateResourcePS(fgFlag flags, const char* data, size_t length, const AbsVec* dpi) { return psTex::Create(data, length, USAGE_SHADER_RESOURCE, FILTER_ALPHABOX); }
void*  fgCloneResourcePS(void* res, fgElement* src) { ((psTex*)res)->Grab(); return res; }
void  fgDestroyResourcePS(void* res) { ((psTex*)res)->Drop(); }
void  fgDrawResourcePS(void* res, const CRect* uv, unsigned int color, unsigned int edge, FABS outline, const AbsRect* dpiarea, FABS rotation, const AbsVec* dpicenter, fgFlag flags, const fgDrawAuxData* data)
{
  psTex* tex = (psTex*)res;
  psRect uvresolve;
  AbsRect area;
  AbsVec center;
  fgResolveDrawRect(dpiarea, &area, dpicenter, &center, flags, data);
  if(tex)
  {
    uvresolve = psRect{ uv->left.rel + (uv->left.abs / tex->GetDim().x),
      uv->top.rel + (uv->top.abs / tex->GetDim().y),
      uv->right.rel + (uv->right.abs / tex->GetDim().x),
      uv->bottom.rel + (uv->bottom.abs / tex->GetDim().y) };
  }
  else
    uvresolve = psRect{ uv->left.abs, uv->top.abs, uv->right.abs, uv->bottom.abs };

  psDriver* driver = psDriverHold::GetDriver();
  if(tex)
    driver->SetTextures(&tex, 1);

  psRectRotate rect(area.left, area.top, area.right, area.bottom, rotation, psVec(center.x - area.left, center.y - area.top));

  if((flags&FGRESOURCE_SHAPEMASK) == FGRESOURCE_RECT)
    psRoundRect::DrawRoundRect(driver->library.ROUNDRECT, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else if((flags&FGRESOURCE_SHAPEMASK) == FGRESOURCE_CIRCLE)
    psRenderCircle::DrawCircle(driver->library.CIRCLE, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else if((flags&FGRESOURCE_SHAPEMASK) == FGRESOURCE_TRIANGLE)
    psRoundTri::DrawRoundTri(driver->library.ROUNDTRI, STATEBLOCK_LIBRARY::PREMULTIPLIED, rect, uvresolve, 0, psColor32(color), psColor32(edge), outline);
  else
    driver->DrawRect(driver->library.IMAGE, 0, rect, &uvresolve, 1, color, 0);
}

void  fgResourceSizePS(void* res, const CRect* uv, AbsVec* dim, fgFlag flags, const AbsVec* dpi)
{
  psTex* tex = (psTex*)res;
  psRect uvresolve = { (uv->left.rel*tex->GetDim().x) + uv->left.abs,
    (uv->top.rel*tex->GetDim().y) + uv->top.abs,
    (uv->right.rel*tex->GetDim().x) + uv->right.abs,
    (uv->bottom.rel*tex->GetDim().y) + uv->bottom.abs };
  dim->x = uvresolve.right - uvresolve.left;
  dim->y = uvresolve.bottom - uvresolve.top;
}

void  fgDrawLinesPS(const AbsVec* p, size_t n, unsigned int color, const AbsVec* translate, const AbsVec* scale, FABS rotation, const AbsVec* center, const fgDrawAuxData* data)
{
  //AbsVec t = *translate;
  //fgScaleVecDPI(&t, data->dpi.x, data->dpi.y);
  psDriver* driver = psDriverHold::GetDriver();
  unsigned long vertexcolor;
  psColor32(color).WriteFormat(FMT_R8G8B8A8, &vertexcolor);
  float m[4][4];
  bss::Matrix<float, 4, 4>::AffineTransform_T(translate->x, translate->y, 0, rotation, center->x, center->y, m);
  float ms[4][4];
  bss::Matrix<float, 4, 4>::AffineScaling(scale->x * (data->dpi.x / 96.0f), scale->y * (data->dpi.y / 96.0f), 1.0f, ms);
  bss::MatrixMultiply<float, 4, 4, 4>(m, ms, m);
  driver->PushTransform(m);

  if(n == 2)
  {
    psBatchObj* o = driver->DrawLinesStart(driver->library.LINE, 0, 0);
    driver->DrawLines(o, psLine{ p[0].x, p[0].y, p[1].x, p[1].y }, 0, 0, vertexcolor);
  }
  else
  {
    DYNARRAY(psVertex, verts, n);
    for(uint32_t i = 0; i < n; ++i)
      verts[i] = { p[i].x, p[i].y, 0, 1, vertexcolor };

    psBatchObj* o = driver->DrawCurveStart(driver->library.LINE, 0, 0);
    driver->DrawCurve(o, verts, n);
  }
  driver->PopTransform();
}

#include "bss-util/win32_includes.h"
#include "bss-util/os.h"

fgRoot*  fgInitialize()
{
  return fgSingleton();
}

void  fgPushClipRectPS(const AbsRect* clip, const fgDrawAuxData* data)
{
  psRect rect = { clip->left, clip->top, clip->right, clip->bottom };
  psDriverHold::GetDriver()->MergeClipRect(rect);
}

AbsRect  fgPeekClipRectPS(const fgDrawAuxData* data)
{
  psRect c = psDriverHold::GetDriver()->PeekClipRect();
  return AbsRect{ c.left, c.top, c.right, c.bottom };
}

void  fgPopClipRectPS(const fgDrawAuxData* data)
{
  psDriverHold::GetDriver()->PopClipRect();
}

void  fgDirtyElementPS(fgElement* e)
{

}

void  fgSetCursorPS(uint32_t type, void* custom)
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

void  fgClipboardCopyPS(uint32_t type, const void* data, size_t length)
{
  OpenClipboard(psEngine::Instance()->GetMonitor()->GetWindow());
  if(data != 0 && length > 0 && EmptyClipboard())
  {
    if(type == FGCLIPBOARD_TEXT)
    {
      length /= sizeof(int);
      size_t len = UTF32toUTF8((const int*)data, length, 0, 0);
      size_t unilen = UTF32toUTF16((const int*)data, length, 0, 0);
      HGLOBAL unimem = GlobalAlloc(GMEM_MOVEABLE, unilen * sizeof(wchar_t));
      if(unimem)
      {
        wchar_t* uni = (wchar_t*)GlobalLock(unimem);
        size_t sz = UTF32toUTF16((const int*)data, length, uni, unilen);
        if(sz < unilen) // ensure we have a null terminator
          uni[sz] = 0;
        GlobalUnlock(unimem);
        SetClipboardData(CF_UNICODETEXT, unimem);
      }
      HGLOBAL gmem = GlobalAlloc(GMEM_MOVEABLE, len);
      if(gmem)
      {
        char* mem = (char*)GlobalLock(gmem);
        size_t sz = UTF32toUTF8((const int*)data, length, mem, len);
        if(sz < len)
          mem[sz] = 0;
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

char  fgClipboardExistsPS(uint32_t type)
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

const void*  fgClipboardPastePS(uint32_t type, size_t* length)
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
      int* ret = bss::bssMalloc<int>(len);
      *length = UTF16toUTF32(str, size, ret, len);
      GlobalUnlock(gdata);
      CloseClipboard();
      return ret;
    }
    {
      HANDLE gdata = GetClipboardData(CF_TEXT);
      const char* str = (const char*)GlobalLock(gdata);
      SIZE_T size = GlobalSize(gdata);
      SIZE_T len = UTF8toUTF32(str, size, 0, 0);
      int* ret = bss::bssMalloc<int>(len);
      *length = UTF8toUTF32(str, size, ret, len);
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
  *length = GlobalSize(gdata);
  void* ret = malloc(*length);
  MEMCPY(ret, *length, data, *length);
  GlobalUnlock(gdata);
  CloseClipboard();
  return ret;
}

void fgClipboardFreePS(const void* mem)
{
  free(const_cast<void*>(mem));
}

void fgDragStartPS(char type, void* data, fgElement* draw)
{
  fgRoot* root = fgSingleton();
  root->dragtype = type;
  root->dragdata = data;
  root->dragdraw = draw;
}
int fgLogHookPS(char level, const char* format, va_list args)
{
  return psEngine::Instance()->GetLog().PrintLogV("feather", __FILE__, __LINE__, level, format, args);
}

char fgProcessMessagesPS()
{
  fgProcessMessagesDefault();
  psEngine::Instance()->FlushMessages();
  return !psEngine::Instance()->GetQuit();
}

psRoot::psRoot() : _psInject(0, 0)
{
  AbsRect area = { 0, 0, 1, 1 };

  static fgBackend BACKEND = {
    FGTEXTFMT_UTF32,
    &fgCreateFontPS,
    &fgCloneFontPS,
    &fgDestroyFontPS,
    &fgDrawFontPS,
    &fgFontLayoutPS,
    &fgFontGetPS,
    &fgFontIndexPS,
    &fgFontPosPS,
    &fgCreateAssetFileDefault,
    &fgCreateResourcePS,
    &fgCloneResourcePS,
    &fgDestroyResourcePS,
    &fgDrawResourcePS,
    &fgResourceSizePS,
    &fgDrawLinesPS,
    &fgCreateDefault,
    &fgFlagMapDefault,
    &fgMessageMapDefault,
    &fgUserDataMapCallbacks,
    &fgPushClipRectPS,
    &fgPeekClipRectPS,
    &fgPopClipRectPS,
    &fgDragStartPS,
    &fgSetCursorPS,
    &fgClipboardCopyPS,
    &fgClipboardExistsPS,
    &fgClipboardPastePS,
    &fgClipboardFreePS,
    &fgDirtyElementPS,
    &fgProcessMessagesPS,
    &fgLoadExtensionDefault,
    &fgLogHookPS,
    &fgTerminateDefault,
  };

  AbsVec dpi = { psGUIManager::BASE_DPI, psGUIManager::BASE_DPI };
  fgRoot_Init(this, &area, &dpi, &BACKEND);
  DWORD blinkrate = 0;
  int64_t sz = bss::GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", 0, 0);
  if(sz > 0)
  {
    DYNARRAY(wchar_t, buf, sz / 2);
    sz = bss::GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", (unsigned char*)buf, sz);
    if(sz > 0)
      cursorblink = atoi(bss::Str(buf, sz / 2)) / 1000.0;
  }
}
psRoot::~psRoot()
{}

void psRoot::_render(const psParent& parent)
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