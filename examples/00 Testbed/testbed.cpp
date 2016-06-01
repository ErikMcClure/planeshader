// Example 00 - Testbed
// -------------------------
// This example runs a series of verification tests to ensure Planeshader is working properly.
//
// Copyright ©2016 Black Sphere Studios

#define _CRT_SECURE_NO_WARNINGS

#include "psEngine.h"
#include "psShader.h"
#include "psColor.h"
#include "psTex.h"
#include "psFont.h"
#include "psImage.h"
#include "psTileset.h"
#include "psPass.h"
#include "psRenderGeometry.h"
#include "psVector.h"
#include "ps_feather.h"
#include "feathergui/fgButton.h"
#include "feathergui/fgResource.h"
#include "feathergui/fgWindow.h"
#include "feathergui/fgRadioButton.h"
#include "feathergui/fgProgressbar.h"
#include "feathergui/fgSlider.h"
#include "feathergui/fgList.h"
#include "feathergui/fgSkin.h"
#include "feathergui/fgRoot.h"
#include "bss-util/bss_win32_includes.h"
#include "bss-util/cHighPrecisionTimer.h"
#include "bss-util/lockless.h"
#include "bss-util/cStr.h"
#include "bss-util/profiler.h"
#include "bss-util/bss_algo.h"
#include <time.h>
#include <iostream>
#include <functional>

#undef DrawText

using namespace planeshader;
using namespace bss_util;

cLog _failedtests("../bin/failedtests.txt"); //This is spawned too early for us to save it with SetWorkDirToCur();
psEngine* engine = 0;
psCamera globalcam;
bool dirkeys[9] = { false }; // left right up down in out counterclockwise clockwise shift
float dirspeeds[4] = { 300.0f, 300.0f, 2.0f, 1.0f }; // X Y Z R
bool gotonext = false;

#pragma warning(disable:4244)

#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader_d.lib")
#pragma comment(lib, "../../lib/bss-util_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader.lib")
#pragma comment(lib, "../../lib/bss-util.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../../bin32/PlaneShader32_d.lib")
#pragma comment(lib, "../../lib/bss-util32_d.lib")
#else
#pragma comment(lib, "../../bin32/PlaneShader32.lib")
#pragma comment(lib, "../../lib/bss-util32.lib")
#endif

struct TESTDEF
{
  typedef std::pair<size_t, size_t> RETPAIR;
  const char* NAME;
  RETPAIR(*FUNC)();
};

#define BEGINTEST TESTDEF::RETPAIR __testret(0,0)
#define ENDTEST return __testret
#define FAILEDTEST(t) BSSLOG(_failedtests,1) << "Test #" << __testret.first << " Failed  < " << MAKESTRING(t) << " >" << std::endl
#define TEST(t) { atomic_xadd(&__testret.first); try { if(t) atomic_xadd(&__testret.second); else FAILEDTEST(t); } catch(...) { FAILEDTEST(t); } }
#define TESTERROR(t, e) { atomic_xadd(&__testret.first); try { (t); FAILEDTEST(t); } catch(e) { atomic_xadd(&__testret.second); } }
#define TESTERR(t) TESTERROR(t,...)
#define TESTNOERROR(t) { atomic_xadd(&__testret.first); try { (t); atomic_xadd(&__testret.second); } catch(...) { FAILEDTEST(t); } }
#define TESTARRAY(t,f) _ITERFUNC(__testret,t,[&](uint32_t i) -> bool { f });
#define TESTALL(t,f) _ITERALL(__testret,t,[&](uint32_t i) -> bool { f });
#define TESTCOUNT(c,t) { for(uint32_t i = 0; i < c; ++i) TEST(t) }
#define TESTCOUNTALL(c,t) { bool __val=true; for(uint32_t i = 0; i < c; ++i) __val=__val&&(t); TEST(__val); }
#define TESTFOUR(s,a,b,c,d) TEST(((s)[0]==(a)) && ((s)[1]==(b)) && ((s)[2]==(c)) && ((s)[3]==(d)))
#define TESTALLFOUR(s,a) TEST(((s)[0]==(a)) && ((s)[1]==(a)) && ((s)[2]==(a)) && ((s)[3]==(a)))
#define TESTRELFOUR(s,a,b,c,d) TEST(fcompare((s)[0],(a)) && fcompare((s)[1],(b)) && fcompare((s)[2],(c)) && fcompare((s)[3],(d)))
#define TESTVEC(v,cx,cy) TEST((v).x==(cx)) TEST((v).y==(cy))
#define TESTVEC3(v,cx,cy,cz) TEST((v).x==(cx)) TEST((v).y==(cy)) TEST((v).z==(cz))

template<class T, size_t SIZE, class F>
void _ITERFUNC(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { for(uint32_t i = 0; i < SIZE; ++i) TEST(f(i)) }
template<class T, size_t SIZE, class F>
void _ITERALL(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { bool __val = true; for(uint32_t i = 0; i < SIZE; ++i) __val = __val && (f(i)); TEST(__val); }

bool comparevec(psVec a, psVec b, int diff = 1)
{
  return b.x == 0.0f ? fsmall(a.x, FLT_EPS * 4) : fcompare(a.x, b.x, diff) && b.y == 0.0f ? fsmall(a.y, FLT_EPS * 4) : fcompare(a.y, b.y, diff);
}
bool comparevec(psColor& a, psColor& b, int diff = 1)
{
  return b.r == 0.0f ? fsmall(a.r) : fcompare(a.r, b.r, diff) &&
    b.g == 0.0f ? fsmall(a.g) : fcompare(a.g, b.g, diff) &&
    b.b == 0.0f ? fsmall(a.b) : fcompare(a.b, b.b, diff) &&
    b.a == 0.0f ? fsmall(a.a) : fcompare(a.a, b.a, diff);
}
cStr ReadFile(const char* path)
{
  FILE* f;
  FOPEN(f, path, "rb");
  if(!f) return "";
  cStr buf;
  fseek(f, 0, SEEK_END);
  long ln = ftell(f);
  buf.resize(ln + 1);
  fseek(f, 0, SEEK_SET);
  fread(buf.UnsafeString(), 1, ln, f);
  return std::move(buf);
}
void processGUI()
{
  static cHighPrecisionTimer delta;
  delta.Update();
  double secdelta = delta.GetDeltaNS() / 1000000000.0;
  float scale = dirkeys[8] ? 0.01f : 1.0f;
  if(dirkeys[0])
    globalcam.SetPositionX(globalcam.GetPosition().x + dirspeeds[0] * secdelta * scale);
  if(dirkeys[1])
    globalcam.SetPositionX(globalcam.GetPosition().x - dirspeeds[0] * secdelta * scale);
  if(dirkeys[2])
    globalcam.SetPositionY(globalcam.GetPosition().y + dirspeeds[1] * secdelta * scale);
  if(dirkeys[3])
    globalcam.SetPositionY(globalcam.GetPosition().y - dirspeeds[1] * secdelta * scale);
  if(dirkeys[4])
    globalcam.SetPositionZ(globalcam.GetPosition().z + dirspeeds[2] * secdelta * scale);
  if(dirkeys[5])
    globalcam.SetPositionZ(globalcam.GetPosition().z - dirspeeds[2] * secdelta * scale);
  if(dirkeys[6])
    globalcam.SetRotation(globalcam.GetRotation() + dirspeeds[3] * secdelta * scale);
  if(dirkeys[7])
    globalcam.SetRotation(globalcam.GetRotation() - dirspeeds[3] * secdelta * scale);
}

TESTDEF::RETPAIR test_psCircle()
{
  BEGINTEST;
  TEST(psCirclei(1, 2, 1).IntersectPoint(2, 2));
  TEST(!psCirclei(1, 1, 1).IntersectPoint(2, 2));
  TEST(psCircle(1.5f, 1.5f, 2).IntersectPoint(2, 2));
  TEST(!psCircle(1.5f, 1.5f, 0.1f).IntersectPoint(2, 2));
  ENDTEST;
}

TESTDEF::RETPAIR test_psRect()
{
  BEGINTEST;
  psRect rct(-1.0f, 0.0f, 1.0f, 2.0f);
  TEST(!rct.IntersectPoint(2, 2));
  TEST(!rct.IntersectPoint(1, 2));
  TEST(rct.IntersectPoint(-1, 0));
  TEST(rct.IntersectPoint(0, 1));
  ENDTEST;
}

TESTDEF::RETPAIR test_psColor()
{
  BEGINTEST;
  psColor c(0xFF7F3F1F);
  TEST(comparevec(c, psColor(0.498039f, 0.2470588f, 0.121568f, 1.0f), 100));
  unsigned int ci = c;
  TEST(ci == 0xFF7F3F1F);
  TEST(psColor::Interpolate(0xFF7F3F1F, 0x103070F0, 0) == 0xFF7F3F1F);
  TEST(psColor::Interpolate(0xFF7F3F1F, 0x103070F0, 1.0f) == 0x103070F0);
  TEST(psColor::Interpolate(0xFF7F3F1F, 0x103070F0, 0.5f) == 0x87575787);
  TEST(psColor::Multiply(0xFF7F3F1F, 0) == 0);
  TEST(psColor::Multiply(0xFF7F3F1F, 1.0f) == 0xFF7F3F1F);
  TEST(psColor::Multiply(0xFF7F3F1F, 0.5f) == 0x7F3F1F0F);
  ENDTEST;
}

void updatefpscount(uint64_t& timer, int& fps)
{
  if(cHighPrecisionTimer::CloseProfiler(timer)>1000000000)
  {
    timer = cHighPrecisionTimer::OpenProfiler();
    char text[10] = { 0,0,0,0,0,0,0,0,0,0 };
    _itoa_r(fps, text, 10);
    engine->GetMonitor()->SetWindowTitle(text);
    fps = 0;
  }
  ++fps;
}

TESTDEF::RETPAIR test_psDirectX11()
{
  BEGINTEST;
  cStr shfile = ReadFile("../media/testbed.hlsl");
  auto shader = psShader::CreateShader(0, 0, 2, &SHADER_INFO(shfile.c_str(), (const char*)"vs_main", VERTEX_SHADER_4_0),
    &SHADER_INFO(shfile.c_str(), "ps_main", PIXEL_SHADER_4_0));
  auto timer = cHighPrecisionTimer::OpenProfiler();
  int fps = 0;
  //psTex* pslogo = psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_BOX, 0, FILTER_NONE, psVeciu(192));
  psTex* pslogo = psTex::Create("../media/pslogo.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_NONE, false);
  const int NUMBATCH = 30;
  psDriver* driver = engine->GetDriver();
  globalcam.SetPosition(500, 500);
  globalcam.SetPivotAbs(psVec(300, 300));

  psVec3D imgpos[NUMBATCH];
  for(int i = 0; i < NUMBATCH; ++i) imgpos[i] = psVec3D(RANDINTGEN(0, driver->GetBackBuffer()->GetRawDim().x), RANDINTGEN(0, driver->GetBackBuffer()->GetRawDim().y), RANDFLOATGEN(0, PI_DOUBLEf));

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    //driver->PushCamera(globalcam.GetPosition(), psVec(300,300), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->GetBackBuffer()->GetRawDim()), psCamera::default_extent);
    globalcam.Apply(*engine->GetPass(0)->GetRenderTarget());
    driver->SetTextures(&pslogo, 1);

    //psVec pt = psVec(engine->GetMouse() - (driver->GetBackBuffer()->GetRawDim() / 2u)) * (-driver->ReversePoint(VEC3D_ZERO).z);
    //psVec3D point = driver->ReversePoint(psVec3D(pt.x, pt.y, 1));
    psVec3D point = driver->FromScreenSpace(engine->GetMouse());
    //psVec3D point = driver->FromScreenSpace(VEC_ZERO);
    auto& mouserect = psRectRotateZ(point.x, point.y, point.x + pslogo->GetDim().x, point.y + pslogo->GetDim().y, 0.0f, pslogo->GetDim()*0.5f);
    //if(!globalcam.Cull(mouserect, 0))
    driver->DrawRect(driver->library.IMAGE, 0, mouserect, &RECT_UNITRECT, 1, 0xFFFFFFFF, 0);
    driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(500, 500, 500 + pslogo->GetDim().x, 500 + pslogo->GetDim().y, 0.0f, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 1, 0xFFFFFFFF, 0);
    {
      psBatchObj* obj = driver->DrawRectBatchBegin(driver->library.IMAGE, 0, 1, 0);
      for(int i = 0; i < NUMBATCH; ++i)
      {
        driver->DrawRectBatch(obj, psRectRotateZ(imgpos[i].x, imgpos[i].y, imgpos[i].x + pslogo->GetDim().x, imgpos[i].y + pslogo->GetDim().y, imgpos[i].z, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 0xFFFFFFFF);
      }
    }

    psBatchObj* obj = driver->DrawLinesStart(driver->library.LINE, 0, PSFLAG_FIXED);
    driver->DrawLines(obj, psLine(0, 0, 100, 200), 0, 0, 0xFFFFFFFF);
    driver->DrawLines(obj, psLine(50, 100, 1000, -2000), 0, 0, 0xFFFFFFFF);

    psVec polygon[5] = { { 200, 0 }, { 200, 100 }, { 100, 150 }, { 60, 60 }, { 90, 30 } };
    auto ptest1 = driver->DrawPolygon(driver->library.POLYGON, 0, polygon, 5, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
    auto ptest2 = driver->DrawPolygon(driver->library.POLYGON, 0, polygon, 5, psVec3D(400, 400, 0), 0xFFFFFFFF, 0);
    engine->End();
    engine->FlushMessages();
    driver->PopCamera();

    updatefpscount(timer, fps);
  }

  globalcam.SetPivotAbs(psVec(0));
  ENDTEST;
}

TESTDEF::RETPAIR test_psOpenGL4()
{
  BEGINTEST;
  ENDTEST;
}

TESTDEF::RETPAIR test_psInheritable()
{
  BEGINTEST;

  // use a worst case inheritance pattern here and then copy to another psInheritable
  // Verify that the copy clones things correctly, renders things correctly, and gets culled properly
  // do this both inside and outside the automatic pass management
  // then verify it gets destroyed properly.

  ENDTEST;
}

TESTDEF::RETPAIR test_psParticles()
{
  BEGINTEST;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  int fps = 0;

  psTex* particle = psTex::Create("../media/particle.png");
  psVertex verts[5000];
  psVec velocities[5000];
  memset(velocities, 0, sizeof(psVec) * 5000);
  psDriver* driver = engine->GetDriver();

  for(int i = 0; i < 5000; ++i)
  {
    verts[i].x = bssrandreal(0, driver->GetBackBuffer()->GetDim().x);
    verts[i].y = bssrandreal(0, driver->GetBackBuffer()->GetDim().y);
    verts[i].z = 0;
    verts[i].w = bssrandreal(8, 32);
    verts[i].color = 0x88FFFFFF;
  }

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), globalcam.GetPivot(), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->GetBackBuffer()->GetRawDim()), psCamera::default_extent);

    for(int i = 0; i < 5000; ++i)
    {
      verts[i].x += velocities[i].x;
      verts[i].y += velocities[i].y;
      double s = dist(verts[i].x, verts[i].y, driver->GetBackBuffer()->GetDim().x / 2, driver->GetBackBuffer()->GetDim().y / 2) / 2000.0; // This gets the distance from the center, scaling the outer boundary at 2000 to 1.0
      double d = bssrandreal(-1.0, 1.0);
      d = std::pow(abs(d), 1.0 + s*s) * ((d < 0.0) ? -1.0 : 1.0); // This causes the distribution of d to tend towards zero at a higher rate the farther away a particle is from the center. This allows particles to move freely near the center, but get dragged towards it farther away.
      velocities[i] += psVec::FromPolar(0.004, bssfmod<double>(d*PI - atan2(verts[i].y - driver->GetBackBuffer()->GetDim().y / 2, -verts[i].x + driver->GetBackBuffer()->GetDim().x / 2), PI_DOUBLE)); // At d = 0, the velocity will always point towards the center.
    }

    driver->SetTextures(&particle, 1);
    driver->DrawPoints(driver->library.PARTICLE, STATEBLOCK_LIBRARY::GLOW, verts, 5000, 0);
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }

  ENDTEST;
}

TESTDEF::RETPAIR test_psPass()
{
  BEGINTEST;

  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  cStr shfile = ReadFile("../media/motionblur.hlsl");
  auto shader = psShader::MergeShaders(2, driver->library.IMAGE, psShader::CreateShader(0, 0, 1, &SHADER_INFO(shfile.c_str(), "mainPS", PIXEL_SHADER_4_0)));

  psImage image(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_PREMULTIPLY_SRGB, false));
  psImage image2(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_ALPHABOX, 0, FILTER_NONE, false));
  psImage image3(psTex::Create("../media/blendtest.png", USAGE_SHADER_RESOURCE, FILTERS(FILTER_LINEAR | FILTER_SRGB_MIPMAP), 0, FILTER_NONE, false));
  psRenderLine line(psLine3D(0, 0, 0, 100, 100, 4));
  //engine->GetPass(0)->Insert(&image2);
  engine->GetPass(0)->Insert(&image);
  //engine->GetPass(0)->Insert(&image3);
  //engine->GetPass(0)->Insert(&line);
  engine->GetPass(0)->SetClearColor(0xFF000000);
  engine->GetPass(0)->SetCamera(&globalcam);
  //globalcam.SetPositionZ(-4.0);
  image3.SetScale(psVec(0.5));

  image.SetStateblock(STATEBLOCK_LIBRARY::PREMULTIPLIED);
  const_cast<psTex*>(image.GetTexture())->SetTexblock(STATEBLOCK_LIBRARY::UVBORDER);
  image.ApplyEdgeBuffer();
  image2.SetShader(shader);

  while(!gotonext && engine->Begin(0))
  {
    for(int i = 0; i < 200; ++i)
    {
      image3.Render();
      image.Render();
    }
    processGUI();
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }
  ENDTEST;
}

TESTDEF::RETPAIR test_psEffect()
{
  BEGINTEST;
  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psTex* gamma = psTex::Create(driver->GetBackBuffer()->GetDim(), FMT_R8G8B8A8_SRGB, USAGE_RENDERTARGET | USAGE_SHADER_RESOURCE, 1);
  engine->GetPass(0)->SetRenderTarget(gamma);

  while(!gotonext && engine->Begin(0))
  {
    // TODO: Draw everything normally on the gamma buffer
    // Calculate each light and it's shadows on one buffer
    // Then add this to the light accumulation buffer
    // All of the above can be done with one psEffect with the lights tied to the accumulation buffers, it will sort everything out properly.

    // Then draw this light accumulation buffer on to the gamma buffer and multiply everything
    // Copy the gamma buffer to the backbuffer
    // Draw sRGB GUI elements blended in sRGB space.

    processGUI();
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }

  ENDTEST;
}

fgElement* fg_progbar;

TESTDEF::RETPAIR test_feather()
{
  BEGINTEST;
  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  const fgTransform FILL_TRANSFORM = { { 0, 0, 0, 0, 0, 1, 0, 1 }, 0, { 0, 0, 0, 0 } };

  fgSkin skin;
  fgSkin_Init(&skin);
  fgSkin* fgButtonTestSkin = fgSkin_AddSkin(&skin, "buttontest");
  fgSkin* fgWindowSkin = fgSkin_AddSkin(&skin, "fgWindow");
  fgSkin* fgButtonSkin = fgSkin_AddSkin(fgWindowSkin, "fgButton");
  fgSkin* fgResourceSkin = fgSkin_AddSkin(fgButtonTestSkin, "fgResource");
  fgSkin* fgCheckboxSkin = fgSkin_AddSkin(fgWindowSkin, "fgCheckbox");
  fgSkin* fgRadioButtonSkin = fgSkin_AddSkin(fgWindowSkin, "fgRadioButton");
  fgSkin* fgProgressbarSkin = fgSkin_AddSkin(fgWindowSkin, "fgProgressbar");
  fgSkin* fgSliderSkin = fgSkin_AddSkin(fgWindowSkin, "fgSlider");
  fgSkin* fgTextboxSkin = fgSkin_AddSkin(fgWindowSkin, "fgTextbox");
  //fgSkin* fgListSkin = fgSkin_AddSkin(&skin, "fgList");

  auto fnAddRect = [](fgSkin* target, const char* name, const CRect& uv, const fgTransform& transform, unsigned int color, unsigned int edge, float outline, fgFlag flags, int order = 0) -> fgStyleLayout* {
    fgStyleLayout* layout = fgSkin_GetChild(target, fgSkin_AddChild(target, "fgResource", name, FGRESOURCE_ROUNDRECT | flags, &transform, order));
    AddStyleMsgArg<FG_SETUV, CRect>(&layout->style, &uv);
    AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(&layout->style, color);
    AddStyleMsg<FG_SETCOLOR, ptrdiff_t, size_t>(&layout->style, edge, 1);
    AddStyleMsg<FG_SETOUTLINE, float>(&layout->style, outline);
    return layout;
  };
  auto fnAddCircle = [](fgSkin* target, const char* name, const CRect& uv, const fgTransform& transform, unsigned int color, unsigned int edge, float outline, fgFlag flags, int order = 0) -> fgStyleLayout* {
    fgStyleLayout* layout = fgSkin_GetChild(target, fgSkin_AddChild(target, "fgResource", name, FGRESOURCE_CIRCLE | flags, &transform, order));
    AddStyleMsgArg<FG_SETUV, CRect>(&layout->style, &uv);
    AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(&layout->style, color);
    AddStyleMsg<FG_SETCOLOR, ptrdiff_t, size_t>(&layout->style, edge, 1);
    AddStyleMsg<FG_SETOUTLINE, float>(&layout->style, outline);
    return layout;
  };

  // fgResource
  FG_UINT nuetral = fgSkin_AddStyle(fgResourceSkin, "nuetral");
  FG_UINT active = fgSkin_AddStyle(fgResourceSkin, "active");
  FG_UINT hover = fgSkin_AddStyle(fgResourceSkin, "hover");
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgResourceSkin, nuetral), 0xFFFF00FF);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgResourceSkin, hover), 0xFF00FFFF);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgResourceSkin, active), 0xFFFFFF00);

  // fgButton
  FG_UINT bnuetral = fgSkin_AddStyle(fgButtonTestSkin, "nuetral");
  FG_UINT bactive = fgSkin_AddStyle(fgButtonTestSkin, "active");
  FG_UINT bhover = fgSkin_AddStyle(fgButtonTestSkin, "hover");
  AddStyleMsg<FG_SETTEXT, void*>(fgSkin_GetStyle(fgButtonTestSkin, bnuetral), "Nuetral");
  AddStyleMsg<FG_SETTEXT, void*>(fgSkin_GetStyle(fgButtonTestSkin, bactive), "Active");
  AddStyleMsg<FG_SETTEXT, void*>(fgSkin_GetStyle(fgButtonTestSkin, bhover), "Hover");

  void* font = fgCreateFont(FGTEXT_SUBPIXEL, "arial.ttf", 14, 96);
  ((psFont*)font)->SetLineHeight(16);

  FG_Msg msg = { 0 };
  msg.type = FG_SETCOLOR;
  msg.otherint = 0xFFFFFFFF;
  fgStyle_AddStyleMsg(&fgButtonSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgButtonTestSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgCheckboxSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgRadioButtonSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgProgressbarSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgTextboxSkin->style, &msg, 0, 0, 0, 0);
  msg.type = FG_SETFONT;
  msg.other = font;
  fgStyle_AddStyleMsg(&fgButtonSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgButtonTestSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgCheckboxSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgRadioButtonSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgProgressbarSkin->style, &msg, 0, 0, 0, 0);
  fgStyle_AddStyleMsg(&fgTextboxSkin->style, &msg, 0, 0, 0, 0);

  AbsRect buttonpadding = { 5, 5, 5, 5 };
  AddStyleMsgArg<FG_SETPADDING, AbsRect>(&fgButtonSkin->style, &buttonpadding);

  fgSkin* fgbuttonbgskin = fgSkin_AddSkin(fgButtonSkin, ":bg");
  bnuetral = fgSkin_AddStyle(fgbuttonbgskin, "nuetral");
  bactive = fgSkin_AddStyle(fgbuttonbgskin, "active");
  bhover = fgSkin_AddStyle(fgbuttonbgskin, "hover");

  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgbuttonbgskin, bnuetral), 0xFF666666);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgbuttonbgskin, bactive), 0xFF505050);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgbuttonbgskin, bhover), 0xFF7F7F7F);

  fnAddRect(fgButtonSkin, ":bg", CRect { 5, 0, 5, 0, 5, 0, 5, 0 }, FILL_TRANSFORM, 0xFF666666, 0xFFAAAAAA, 1.0f, FGELEMENT_BACKGROUND);

  // fgWindow
  fnAddRect(fgWindowSkin, 0, CRect { 5, 0, 5, 0, 5, 0, 5, 0 }, FILL_TRANSFORM, 0xFF666666, 0xFFAAAAAA, 1.0f, FGELEMENT_BACKGROUND);

  fgSkin* topwindowcaption = fgSkin_AddSkin(fgWindowSkin, "fgWindow:text");
  AbsRect margin = { 4, 4, 0, 0 };
  AddStyleMsgArg<FG_SETMARGIN, AbsRect>(&topwindowcaption->style, &margin);

  // fgCheckbox
  fnAddRect(fgCheckboxSkin, ":checkbg", CRect { 3, 0, 3, 0, 3, 0, 3, 0 }, fgTransform { { 5, 0, 0, 0.5, 20, 0, 15, 0.5 }, 0, { 0, 0, 0, 0.5 } }, 0xFFFFFFFF, 0xFF222222, 1.0f, FGELEMENT_BACKGROUND | FGELEMENT_SNAP);
  //fnAddRect(fgCheckboxSkin, 0, CRect { 0, 0, 0, 0, 0, 0, 0, 0 }, fgTransform { { 0, 0, 0, 0, 0, 1, 0, 1 }, 0, { 0, 0, 0, 0 } }, 0x99000000, 0, 0.0f, FGELEMENT_BACKGROUND);
  AddStyleMsgArg<FG_SETPADDING, AbsRect>(&fgCheckboxSkin->style, &AbsRect { 25,0,5,0 });
  fnAddRect(fgCheckboxSkin, ":check", CRect { 2, 0, 2, 0, 2, 0, 2, 0 }, fgTransform { { 8, 0, 0, 0.5, 17, 0, 9, 0.5 }, 0, { 0, 0, 0, 0.5 } }, 0xFF000000, 0, 0, FGELEMENT_BACKGROUND | FGELEMENT_SNAP, 1);
  fgSkin* fgCheckedSkin = fgSkin_AddSkin(fgCheckboxSkin, ":check");

  fgSkin* fgCheckboxSkinbg = fgSkin_AddSkin(fgCheckboxSkin, ":checkbg");
  bnuetral = fgSkin_AddStyle(fgCheckboxSkinbg, "nuetral");
  bactive = fgSkin_AddStyle(fgCheckboxSkinbg, "active");
  bhover = fgSkin_AddStyle(fgCheckboxSkinbg, "hover");
  FG_UINT bdefault = fgSkin_AddStyle(fgCheckedSkin, "default");
  FG_UINT bchecked = fgSkin_AddStyle(fgCheckedSkin, "checked");
  FG_UINT bindeterminate = fgSkin_AddStyle(fgCheckedSkin, "indeterminate");

  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgCheckboxSkinbg, bnuetral), 0xFFFFFFFF);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgCheckboxSkinbg, bactive), 0xFFBBBBBB);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgCheckboxSkinbg, bhover), 0xFFDDDDDD);
  AddStyleMsg<FG_SETFLAG, ptrdiff_t, size_t>(fgSkin_GetStyle(fgCheckedSkin, bdefault), FGELEMENT_HIDDEN, 1);
  AddStyleMsg<FG_SETFLAG, ptrdiff_t, size_t>(fgSkin_GetStyle(fgCheckedSkin, bchecked), FGELEMENT_HIDDEN, 0);
  AddStyleMsg<FG_SETFLAG, ptrdiff_t, size_t>(fgSkin_GetStyle(fgCheckedSkin, bindeterminate), FGELEMENT_HIDDEN, 1);

  // fgRadioButton
  fnAddCircle(fgRadioButtonSkin, ":bg", CRect { 0, 0, PI_DOUBLEf, 0, 0, 0, PI_DOUBLEf, 0 }, fgTransform { { 5, 0, 0, 0.5, 20, 0, 15, 0.5 }, 0, { 0, 0, 0, 0.5 } }, 0xFFFFFFFF, 0xFF222222, 1.0f, FGELEMENT_BACKGROUND | FGELEMENT_SNAP);
  AddStyleMsgArg<FG_SETPADDING, AbsRect>(&fgRadioButtonSkin->style, &AbsRect { 25,5,5,5 });
  fnAddCircle(fgRadioButtonSkin, ":check", CRect { 0, 0, PI_DOUBLEf, 0, 0, 0, PI_DOUBLEf, 0 }, fgTransform { { 8, 0, 0, 0.5, 17, 0, 9, 0.5 }, 0, { 0, 0, 0, 0.5 } }, 0xFF000000, 0, 0, FGELEMENT_BACKGROUND | FGELEMENT_SNAP, 1);
  fgSkin* fgRadioSelectedSkin = fgSkin_AddSkin(fgRadioButtonSkin, ":check");

  fgSkin* fgRadioButtonSkinbg = fgSkin_AddSkin(fgRadioButtonSkin, ":bg");
  bnuetral = fgSkin_AddStyle(fgRadioButtonSkinbg, "nuetral");
  bactive = fgSkin_AddStyle(fgRadioButtonSkinbg, "active");
  bhover = fgSkin_AddStyle(fgRadioButtonSkinbg, "hover");
  bdefault = fgSkin_AddStyle(fgRadioSelectedSkin, "default");
  bchecked = fgSkin_AddStyle(fgRadioSelectedSkin, "checked");

  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgRadioButtonSkinbg, bnuetral), 0xFFFFFFFF);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgRadioButtonSkinbg, bactive), 0xFFBBBBBB);
  AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(fgSkin_GetStyle(fgRadioButtonSkinbg, bhover), 0xFFDDDDDD);
  AddStyleMsg<FG_SETFLAG, ptrdiff_t, size_t>(fgSkin_GetStyle(fgRadioSelectedSkin, bdefault), FGELEMENT_HIDDEN, 1);
  AddStyleMsg<FG_SETFLAG, ptrdiff_t, size_t>(fgSkin_GetStyle(fgRadioSelectedSkin, bchecked), FGELEMENT_HIDDEN, 0);

  // fgSlider
  AddStyleMsgArg<FG_SETPADDING, AbsRect>(&fgSliderSkin->style, &AbsRect { 5,0,5,0 });
  {
    fgStyleLayout* layout = fgSkin_GetChild(fgSliderSkin, fgSkin_AddChild(fgSliderSkin, "fgResource", 0, FGRESOURCE_LINE, &fgTransform { { 0, 0, 0.5, 0.5, 0, 1.0, 1.5, 0.5 }, 0, { 0, 0, 0, 0 } }));
    AddStyleMsg<FG_SETCOLOR, ptrdiff_t>(&layout->style, 0xFFFFFFFF);
  }
  fgSkin* fgSliderDragSkin = fgSkin_AddSkin(fgSliderSkin, "fgSlider:slider");
  fnAddRect(fgSliderDragSkin, 0, CRect { 5, 0, 5, 0, 5, 0, 5, 0 }, fgTransform { { 0, 0, 0, 0, 10, 0, 20, 0 }, 0, { 0, 0, 0, 0 } }, 0xFFFFFFFF, 0xFF666666, 1.0f, FGELEMENT_NOCLIP);

  // fgProgressbar
  AddStyleMsgArg<FG_SETPADDING, AbsRect>(&fgProgressbarSkin->style, &AbsRect { 0,0,0,0 });
  fnAddRect(fgProgressbarSkin, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0, 0xFFDDDDDD, 3.0f, FGELEMENT_BACKGROUND);
  fgSkin* fgProgressSkin = fgSkin_AddSkin(fgProgressbarSkin, "fgProgressbar:bar");
  fnAddRect(fgProgressSkin, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0xFF333333, 0, 3.0f, 0);

  // fgBox
  fgSkin* fgBoxSkin = fgSkin_AddSkin(fgWindowSkin, "fgBox");
  fgSkin* fgScrollbarSkinBGx = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:horzbg");
  fgSkin* fgScrollbarSkinBGy = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:vertbg");
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinBGx->style, &CRect { 0, 0, -20, 1, 0, 1, 0, 1 });
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinBGy->style, &CRect { -20, 1, 0, 0, 0, 1, 0, 1 });
  fnAddRect(fgScrollbarSkinBGx, 0, CRect { 0, 0, 0, 0, 0, 0, 0, 0 }, FILL_TRANSFORM, 0x66000000, 0, 0.0f, FGELEMENT_BACKGROUND);
  fnAddRect(fgScrollbarSkinBGy, 0, CRect { 0, 0, 0, 0, 0, 0, 0, 0 }, FILL_TRANSFORM, 0x66000000, 0, 0.0f, FGELEMENT_BACKGROUND);
  fgSkin* fgScrollbarSkinBarx = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrollhorz");
  fgSkin* fgScrollbarSkinBary = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrollvert");
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinBarx->style, &CRect { 0, 0, 0, 0, 0, 1, 0, 1 });
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinBary->style, &CRect { 0, 0, 0, 0, 0, 1, 0, 1 });
  AddStyleMsgArg<FG_SETMARGIN, AbsRect>(&fgScrollbarSkinBarx->style, &AbsRect { 10, 0, 10, 0 });
  AddStyleMsgArg<FG_SETMARGIN, AbsRect>(&fgScrollbarSkinBary->style, &AbsRect { 0, 20, 0, 20 });
  fnAddRect(fgScrollbarSkinBarx, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);
  fnAddRect(fgScrollbarSkinBary, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);
  fgSkin* fgScrollbarSkinLeft = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrollleft");
  fgSkin* fgScrollbarSkinTop = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrolltop");
  fgSkin* fgScrollbarSkinRight = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrollright");
  fgSkin* fgScrollbarSkinBottom = fgSkin_AddSkin(fgBoxSkin, "fgScrollbar:scrollbottom");
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinLeft->style, &CRect { 0, 0, 0, 0, 20, 0, 20, 0 });
  AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinTop->style, &CRect { 0, 0, 0, 0, 20, 0, 20, 0 });
  //AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinRight->style, &CRect { 0, 0, 0, 0, 0, 1, 0, 1 });
  //AddStyleMsgArg<FG_SETAREA, CRect>(&fgScrollbarSkinBottom->style, &CRect { 0, 0, 0, 0, 0, 1, 0, 1 });
  //fnAddRect(fgScrollbarSkinLeft, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);
  //fnAddRect(fgScrollbarSkinTop, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);
  //fnAddRect(fgScrollbarSkinRight, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);
  //fnAddRect(fgScrollbarSkinBottom, 0, CRect { 6, 0, 6, 0, 6, 0, 6, 0 }, FILL_TRANSFORM, 0x660000FF, 0xFF0000FF, 1.0f, FGELEMENT_BACKGROUND);

  // fgTextbox
  fnAddRect(fgTextboxSkin, 0, CRect { 3, 0, 3, 0, 3, 0, 3, 0 }, FILL_TRANSFORM, 0x99000000, 0xFFAAAAAA, 1.0f, FGELEMENT_BACKGROUND);

  // Apply skin and set up layout
  fgVoidMessage(*fgSingleton(), FG_SETSKIN, &skin, 0);

  fgElement* res = fgResource_Create(fgCreateResourceFile(0, "../media/circle.png"), 0, 0xFFFFFFFF, 0, 0, 0, FGELEMENT_EXPAND | FGELEMENT_IGNORE, 0);
  fgElement* button = fgCreate("fgButton", *fgSingleton(), 0, 0, FGELEMENT_EXPAND, 0);
  fgVoidMessage(button, FG_ADDITEM, res, 0);
  button->SetName("buttontest");

  fgElement* topwindow = fgCreate("fgWindow", *fgSingleton(), 0, 0, 0, &fgTransform { { 0, 0.2f, 0, 0.2f, 0, 0.8f, 0, 0.8f }, 0, { 0, 0, 0, 0 } });
  topwindow->SetText("test window");
  topwindow->SetColor(0xFFFFFFFF, 0);
  topwindow->SetFont(font);
  topwindow->SetPadding(AbsRect { 10,22,10,10 });

  fgElement* buttontop = fgCreate("fgButton", topwindow, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 10, 0, 40, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } });
  buttontop->SetText("Not Pressed");
  fgListener listener = [](fgElement* self, const FG_Msg*) { self->SetText("Pressed!"); };
  buttontop->AddListener(FG_ACTION, listener);

  fgElement* boxholder = fgCreate("fgBox", topwindow, 0, 0, FGBOX_TILEY | FGELEMENT_EXPANDX, &fgTransform { { 10, 0, 160, 0, 150, 0, 240, 0 }, 0, { 0, 0, 0, 0 } });
  boxholder->SetMaxDim(100, -1);
  //boxholder->SetMaxDim(-1, 100);

  fgCreate("fgCheckbox", boxholder, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Check Test 1");
  fgCreate("fgCheckbox", boxholder, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Check Test 22");
  fgCreate("fgCheckbox", boxholder, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Check Test 3");
  fgCreate("fgCheckbox", boxholder, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Check Test 4");
  fgCreate("fgCheckbox", boxholder, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Check Test 5");

  fgCreate("fgRadiobutton", topwindow, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 190, 0, 160, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Radio Test 1");
  fgCreate("fgRadiobutton", topwindow, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 190, 0, 190, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Radio Test 2");
  fgCreate("fgRadiobutton", topwindow, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 190, 0, 220, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } })->SetText("Radio Test 3");

  fgElement* slider = fgCreate("fgSlider", topwindow, 0, 0, 0, &fgTransform { { 10, 0, 100, 0, 150, 0, 120, 0 }, 0, { 0, 0, 0, 0 } });
  slider->SetState(500, 1);
  slider->AddListener(FG_SETSTATE, [](fgElement* self, const FG_Msg*) { fg_progbar->SetStatef(self->GetState(0) / (float)self->GetState(1), 0); fg_progbar->SetText(cStrF("%i", self->GetState(0))); });
  fg_progbar = fgProgressbar_Create(0.0, topwindow, 0, 0, 0, &fgTransform { { 10, 0, 130, 0, 150, 0, 155, 0 }, 0, { 0, 0, 0, 0 } });

  fgCreate("fgTextbox", topwindow, 0, 0, 0, &fgTransform { { 190, 0, 30, 0, 250, 0, 150, 0 }, 0, { 0, 0, 0, 0 } });

  fgSingleton()->behaviorhook = &fgRoot_BehaviorListener; // make sure the listener hash is enabled

  while(!gotonext && engine->Begin())
  {
    engine->GetDriver()->Clear(0xFF000000);
    engine->GetGUI().Render();
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }

  fgElement_Clear(*fgSingleton());
  fgSkin_Destroy(&skin);

  ENDTEST;
}

TESTDEF::RETPAIR test_psFont()
{
  BEGINTEST;

  psFont* font = psFont::Create("arial.ttf", 14, psFont::FAA_LCD);

  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psStateblock* block = STATEBLOCK_LIBRARY::SUBPIXELBLEND->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 1, 0.0f));
  block = block->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 0, 0.0f));
  block = block->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 2, 0.0f));
  //font->PreloadGlyphs("qwertyuiopasdfghjklzxcvbnm`1234567890-=[];',./~!@#$%^&*()_+{}|:");

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0xFF999999);
    driver->SetTextures(0, 0);
    driver->DrawRect(driver->library.IMAGE0, 0, psRectRotateZ(0, 0, 100, 100, 0), 0, 0, 0xFF999900, PSFLAG_FIXED);
    //font->DrawText("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla maximus sem at ante porttitor vehicula. Nulla a lorem imperdiet, consectetur metus id, congue enim. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin placerat a ipsum ac sodales. Curabitur vel neque scelerisque elit mollis convallis. Proin porta augue metus, sed pulvinar nisl mollis ut. Proin at aliquam erat. Quisque at diam tellus. Aenean facilisis justo ut mauris egestas dignissim. Maecenas scelerisque, ante ac blandit consectetur, magna sem pharetra massa, eu luctus orci ligula luctus augue. Integer at metus eros. Donec sed eros molestie, posuere nunc id, porta sem. \nMauris fermentum mauris ac eleifend ultrices.Fusce nec sollicitudin turpis, a ultricies metus.Nulla suscipit cursus orci, ac fringilla massa volutpat et.Nullam vestibulum dolor at tortor bibendum condimentum.Donec vitae faucibus risus, ut placerat mauris.Curabitur quis purus at urna pharetra lobortis.Pellentesque turpis velit, molestie aliquet elit sed, vestibulum rutrum nibh. \nSuspendisse ultricies leo nec ante accumsan ullamcorper.Suspendisse scelerisque molestie enim sit amet lacinia.Proin at lorem justo.Curabitur lectus ipsum, accumsan at quam eu, iaculis pellentesque felis.Fusce blandit feugiat dui, id placerat justo sollicitudin sed.Cras auctor lorem hendrerit leo facilisis porttitor.Sed vitae pulvinar purus, sed ornare ligula.\nPhasellus blandit, magna quis bibendum mattis, neque quam gravida quam, at tempus sem sapien eu mi.Phasellus ornare laoreet neque at blandit.Suspendisse vulputate fringilla fermentum.Fusce ante eros, laoreet ultricies eros sit amet, lobortis viverra elit.Curabitur consequat erat neque, in fringilla eros elementum eu.Quisque aliquam laoreet metus, volutpat vulputate tortor vehicula ut.Fusce sodales commodo justo, in condimentum ipsum aliquam at.Phasellus eget tellus ac arcu ultrices vehicula.Integer sagittis metus nibh, in varius mi scelerisque quis.Etiam ullamcorper gravida urna, et vestibulum velit posuere id.Aenean fermentum nibh ac dui rhoncus volutpat.Cras quis felis eget tortor vehicula interdum.In efficitur nulla quam, non condimentum ipsum pulvinar non.\nCras ultricies mi sed lacinia consequat.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Suspendisse potenti.Nam consectetur eleifend libero sed pharetra.Suspendisse in dolor dui.Sed imperdiet pellentesque fermentum.Vivamus ac tortor felis.Aliquam id turpis euismod, tincidunt sapien ac, varius sapien.Vivamus id nulla mauris.");
    font->DrawText(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, "the dog jumped \nover the lazy fox", font->GetLineHeight(), 0, psRectRotateZ(0, 0, 100, 0, 0), 0xCCFFAAFF, PSFONT_WORDBREAK);

    const psTex* t = font->GetTex();
    driver->SetTextures(&t, 1);
    driver->DrawRect(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, psRectRotateZ(0, 100, t->GetDim().x, 100 + t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    //driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(0, 100, t->GetDim().x, 100+t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    engine->End();
    engine->FlushMessages();

    updatefpscount(timer, fps);
  }
  ENDTEST;
}

TESTDEF::RETPAIR test_psVector()
{
  BEGINTEST;
  cHighPrecisionTimer time;
  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psQuadraticCurve curve(psVec(100), psVec(200, 200), psVec(300), 4.25f);
  //psQuadraticCurve curve(psVec(186.236328, 105.025391), psVec(202.958221, 106.747375), psVec(220.288574, 109.005638), 4.25f);
  //engine->GetPass(0)->Insert(&curve);

  //psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(793, 213), psVec(300), 4.25f);
  psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(927, 115), psVec(300), 4.25f);
  engine->GetPass(0)->Insert(&curve2);

  psRoundedRect rect(psRectRotateZ(400, 300, 550, 400, 0), 0);
  rect.SetCorners(psRect(30, 10, 30, 0));
  rect.SetColor(0xAAFFFFFF);
  rect.SetOutlineColor(0xFF0000FF);
  rect.SetOutline(0.5);

  psRenderCircle circle(50, psVec3D(200, 300, 0));
  circle.SetOutlineColor(0xFF0000FF);
  circle.SetOutline(5);

  engine->GetPass(0)->Insert(&rect);
  engine->GetPass(0)->Insert(&circle);

  engine->GetPass(0)->SetClearColor(0xFF999999);
  engine->GetPass(0)->SetCamera(&globalcam);

  while(!gotonext && engine->Begin(0))
  {
    processGUI();
    engine->End();
    time.Update();
    engine->FlushMessages();
    updatefpscount(timer, fps);
    curve.Set(psVec(100), engine->GetMouse(), psVec(300));
    curve2.Set(psVec(100), psVec(300, 100), engine->GetMouse(), psVec(300));
    circle.SetArcs(psRect(atan2(-engine->GetMouse().y + circle.GetPosition().y, engine->GetMouse().x - circle.GetPosition().x) - 0.5, 1.0, 0, bssfmod(time.GetTime()*0.001, PI_DOUBLE)));
  }

  ENDTEST;
}
/*
int figure_out_wang(int p) // brute force wang
{
switch(p)
{
case 0b0001: return 0;
case 0b0011: return 1;
case 0b1011: return 2;
case 0b1001: return 3;
case 0b0101: return 4;
case 0b0111: return 5;
case 0b1111: return 6;
case 0b1101: return 7;
case 0b0100: return 8;
case 0b0110: return 9;
case 0b1110: return 10;
case 0b1100: return 11;
case 0b0000: return 12;
case 0b0010: return 13;
case 0b1010: return 14;
case 0b1000: return 15;
}
assert(false);
return 0;
}*/

int wang_indices(int l, int t, int r, int b) // wang using bitwise operations
{
  int p = l << 3 | t << 2 | r << 1 | b << 0;
  int _8 = (!(p & 1) << 3);
  int _4 = ((p & 4) ^ (((~p) & 1) << 2));
  int _2 = ((p & 8) >> 2);
  int _1 = (((p & 2) >> 1) ^ ((p & 8) >> 3));
  return _8 | _4 | _2 | _1;
}

TESTDEF::RETPAIR test_psTileset()
{
  BEGINTEST;
  int fps = 0;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  const int dimx = 500;
  const int dimy = 500;
  char edges[dimy + 1][dimx + 1][2];
  for(char* i = edges[0][0]; i - edges[0][0] < sizeof(edges) / sizeof(char); ++i)
    *i = RANDBOOLGEN();

  psTile map[dimy][dimx];
  memset(&map, 0, sizeof(psTile) * dimx * dimy);
  for(int j = 0; j < dimy; ++j)
    for(int i = 0; i < dimx; ++i)
    {
      map[j][i].color = ~0;
      map[j][i].index = wang_indices(edges[j][i][0], edges[j][i][1], edges[j][i + 1][0], edges[j + 1][i][1]);
      //psVeciu pos = psTileset::WangTile2D(edges[j][i][0], edges[j][i][1], edges[j][i + 1][0], edges[j + 1][i][1]);
      //map[j][i].index = pos.x + (pos.y * 4);
    }

  psTileset tiles(VEC3D_ZERO, 0, VEC_ZERO, PSFLAG_DONOTCULL, 0, 0, driver->library.IMAGE, engine->GetPass(0));
  tiles.SetTexture(psTex::Create("../media/wang2.png", 64U, FILTER_TRIANGLE, 0, FILTER_NONE, false, STATEBLOCK_LIBRARY::POINTSAMPLE));
  tiles.AutoGenDefs(psVeciu(8, 8));
  tiles.SetTiles(map[0], dimx*dimy, dimx);

  psTileset tiles2(psVec3D(0, 0, 1), 0, VEC_ZERO, PSFLAG_DONOTCULL, 0, 0, driver->library.IMAGE, engine->GetPass(0));
  tiles2.SetTexture(psTex::Create("../media/wang2.png", 64U, FILTER_TRIANGLE, 0, FILTER_NONE, false, STATEBLOCK_LIBRARY::POINTSAMPLE));
  tiles2.AutoGenDefs(psVeciu(8, 8));
  tiles2.SetTiles(map[0], dimx*dimy, dimx);

  engine->GetPass(0)->SetCamera(&globalcam);

  while(!gotonext && engine->Begin(0))
  {
    updatefpscount(timer, fps);
    processGUI();
    tiles.Render();
    tiles2.Render();
    engine->End();
    engine->FlushMessages();
  }

  ENDTEST;
}

// Main program function
int main(int argc, char** argv)
{
  bss_util::ForceWin64Crash();
  SetWorkDirToCur();
  bssrandseed(time(NULL));

  AllocConsole();
  freopen("CONOUT$", "wb", stdout);
  freopen("CONOUT$", "wb", stderr);
  freopen("CONIN$", "rb", stdin);

  TESTDEF tests[] = {
    { "ps_feather", &test_feather },
    { "psFont", &test_psFont },
    { "psVector", &test_psVector },
    { "psDirectX11", &test_psDirectX11 },
    { "psTileset", &test_psTileset },
    { "psPass", &test_psPass },
    { "psCircle", &test_psCircle },
    { "psEffect", &test_psEffect },
    { "psRect", &test_psRect },
    { "psColor", &test_psColor },
    { "psParticles", &test_psParticles },
    { "psInheritable", &test_psInheritable },
  };

  const size_t NUMTESTS = sizeof(tests) / sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint32_t)PS_VERSION_MAJOR << '.' << (uint32_t)PS_VERSION_MINOR << '.' <<
    (uint32_t)PS_VERSION_REVISION << ": Unit Tests\nCopyright (c)2015 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] = { 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");

  std::vector<uint32_t> failures;
  PSINIT init;
  init.driver = RealDriver::DRIVERTYPE_DX11;
  init.width = 640;
  init.height = 480;
  //init.mode = PSINIT::MODE_BORDERLESS;
  //init.antialias = 8;
  init.sRGB = false;
  init.mediapath = "../media";
  //init.iconresource=101;
  //init.filter=5;
  {
    psEngine ps(init);
    if(ps.GetQuit()) return 0;
    psCamera::default_extent.x = 0.2f;
    psCamera::default_extent.y = 100;
    globalcam.SetExtent(psCamera::default_extent);

    std::function<size_t(const FG_Msg&)> guicallback = [&](const FG_Msg& evt) -> size_t
    {
      if(evt.type == FG_KEYDOWN || evt.type == FG_KEYUP)
      {
        bool isdown = evt.type == FG_KEYDOWN;
        dirkeys[8] = evt.IsShiftDown();
        switch(evt.keycode)
        {
        case FG_KEY_A: dirkeys[0] = isdown; break;
        case FG_KEY_D: dirkeys[1] = isdown; break;
        case FG_KEY_W: dirkeys[2] = isdown; break;
        case FG_KEY_S: dirkeys[3] = isdown; break;
        case FG_KEY_X:
        case FG_KEY_OEM_PLUS: dirkeys[4] = isdown; break;
        case FG_KEY_Z:
        case FG_KEY_OEM_MINUS: dirkeys[5] = isdown; break;
        case FG_KEY_Q: dirkeys[6] = isdown; break;
        case FG_KEY_E: dirkeys[7] = isdown; break;
        case FG_KEY_ESCAPE:
          if(isdown) ps.Quit();
          break;
        case FG_KEY_RETURN:
        //  if(isdown && !evt.IsAltDown()) gotonext = true;
          break;
        }
      }
      return 0;
    };
    ps.SetPreprocess(guicallback);
    //ps[0].SetClear(true, 0);
    engine = &ps;

    TESTDEF::RETPAIR numpassed;
    for(uint32_t i = 0; i < NUMTESTS; ++i)
    {
      gotonext = false;
      globalcam.SetPosition(psVec3D(0, 0, -1));
      globalcam.SetRotation(0);
      numpassed = tests[i].FUNC(); //First is total, second is succeeded
      if(numpassed.first != numpassed.second) failures.push_back(i);

      printf("%-*s %*s %-*s\n", COLUMNS[0], tests[i].NAME, COLUMNS[1], cStrF("%u/%u", numpassed.second, numpassed.first).c_str(), COLUMNS[2], (numpassed.first == numpassed.second) ? "PASS" : "FAIL");
    }
  }

  if(failures.empty())
    std::cout << "\nAll tests passed successfully!" << std::endl;
  else
  {
    std::cout << "\nThe following tests failed: " << std::endl;
    for(uint32_t i = 0; i < failures.size(); i++)
      std::cout << "  " << tests[failures[i]].NAME << std::endl;
    std::cout << "\nThese failures indicate either a misconfiguration on your system, or a potential bug. \n\nA detailed list of failed tests was written to failedtests.txt" << std::endl;
  }

  std::cout << "\nPress Enter to exit the program." << std::endl;
  //std::cin.get();
  fclose(stdout);
  fclose(stdin);
  FreeConsole();

  PROFILE_OUTPUT("ps_profiler.txt", 1 | 4);
  //end program
  return 0;
}

struct HINSTANCE__;

// WinMain function, simply a catcher that calls the main function
int __stdcall WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int nShowCmd)
{
  main(0, (char**)hInstance);
}