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
#include "feathergui/fgTopWindow.h"
#include "feathergui/fgSkin.h"
#include "bss-util/bss_win32_includes.h"
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
psEngine* engine=0;
psCamera globalcam;
bool dirkeys[9] = { false }; // left right up down in out counterclockwise clockwise shift
float dirspeeds[4] = { 3.0f, 3.0f, 2.0f, 1.0f };
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
void _ITERALL(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { bool __val=true; for(uint32_t i = 0; i < SIZE; ++i) __val=__val&&(f(i)); TEST(__val); }

bool comparevec(psVec a, psVec b, int diff=1)
{
  return b.x==0.0f?fsmall(a.x, FLT_EPS*4):fcompare(a.x, b.x, diff) && b.y==0.0f?fsmall(a.y, FLT_EPS*4):fcompare(a.y, b.y, diff);
}
bool comparevec(psColor& a, psColor& b, int diff=1)
{
  return b.r==0.0f?fsmall(a.r):fcompare(a.r, b.r, diff) &&
    b.g==0.0f?fsmall(a.g):fcompare(a.g, b.g, diff) &&
    b.b==0.0f?fsmall(a.b):fcompare(a.b, b.b, diff) &&
    b.a==0.0f?fsmall(a.a):fcompare(a.a, b.a, diff);
}
cStr ReadFile(const char* path)
{
  FILE* f;
  FOPEN(f, path, "rb");
  if(!f) return "";
  cStr buf;
  fseek(f, 0, SEEK_END);
  long ln = ftell(f);
  buf.resize(ln+1);
  fseek(f, 0, SEEK_SET);
  fread(buf.UnsafeString(), 1, ln, f);
  return std::move(buf);
}
void processGUI()
{
  float scale = dirkeys[8] ? 0.01f : 1.0f;
  if(dirkeys[0])
    globalcam.SetPositionX(globalcam.GetPosition().x + dirspeeds[0] * engine->secdelta * scale);
  if(dirkeys[1])
    globalcam.SetPositionX(globalcam.GetPosition().x - dirspeeds[0] * engine->secdelta * scale);
  if(dirkeys[2])
    globalcam.SetPositionY(globalcam.GetPosition().y + dirspeeds[1] * engine->secdelta * scale);
  if(dirkeys[3])
    globalcam.SetPositionY(globalcam.GetPosition().y - dirspeeds[1] * engine->secdelta * scale);
  if(dirkeys[4])
    globalcam.SetPositionZ(globalcam.GetPosition().z + dirspeeds[2] * engine->secdelta * scale);
  if(dirkeys[5])
    globalcam.SetPositionZ(globalcam.GetPosition().z - dirspeeds[2] * engine->secdelta * scale);
  if(dirkeys[6])
    globalcam.SetRotation(globalcam.GetRotation() + dirspeeds[3] * engine->secdelta * scale);
  if(dirkeys[7])
    globalcam.SetRotation(globalcam.GetRotation() - dirspeeds[3] * engine->secdelta * scale);
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
  TEST(ci==0xFF7F3F1F);
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
  if(psEngine::CloseProfiler(timer)>1000000000)
  {
    timer = psEngine::OpenProfiler();
    char text[10] = { 0,0,0,0,0,0,0,0,0,0 };
    _itoa_r(fps, text, 10);
    engine->SetWindowTitle(text);
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
  auto timer = psEngine::OpenProfiler();
  int fps=0;
  //psTex* pslogo = psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_BOX, 0, FILTER_NONE, psVeciu(192));
  psTex* pslogo = psTex::Create("../media/pslogo.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_NONE, false);
  const int NUMBATCH = 30;
  psDriver* driver = engine->GetDriver();
  globalcam.SetPosition(500, 500);

  psVec3D imgpos[NUMBATCH];
  for(int i = 0; i < NUMBATCH; ++i) imgpos[i] = psVec3D(RANDINTGEN(0, driver->rawscreendim.x), RANDINTGEN(0, driver->rawscreendim.y), RANDFLOATGEN(0, PI_DOUBLEf));

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), psVec(300,300), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->rawscreendim), psCamera::default_extent);
    driver->SetTextures(&pslogo, 1);
    driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(600, 500, 600+pslogo->GetDim().x, 500+pslogo->GetDim().y, 0.0f, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 1, 0xFFFFFFFF, 0);
    driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(500, 500, 500 + pslogo->GetDim().x, 500 + pslogo->GetDim().y, 0.0f, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 1, 0xFFFFFFFF, 0);
    {
      psBatchObj& obj = driver->DrawRectBatchBegin(driver->library.IMAGE, 0, 1, 0);
      for(int i = 0; i < NUMBATCH; ++i) {
        driver->DrawRectBatch(obj, psRectRotateZ(imgpos[i].x, imgpos[i].y, imgpos[i].x + pslogo->GetDim().x, imgpos[i].y + pslogo->GetDim().y, imgpos[i].z, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 0xFFFFFFFF);
      }
    }

    psBatchObj& obj = driver->DrawLinesStart(driver->library.LINE, 0, PSFLAG_FIXED);
    driver->DrawLines(obj, psLine(0, 0, 100, 200), 0, 0, 0xFFFFFFFF);
    driver->DrawLines(obj, psLine(50, 100, 1000, -2000), 0, 0, 0xFFFFFFFF);

    psVec polygon[5] ={ { 200, 0 }, { 200, 100 }, { 100, 150 }, { 60, 60 }, { 90, 30 } };
    driver->DrawPolygon(driver->library.POLYGON, 0, polygon, 5, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
    engine->End();

    updatefpscount(timer, fps);
  }

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
  auto timer = psEngine::OpenProfiler();
  int fps = 0;

  psTex* particle = psTex::Create("../media/particle.png");
  psVertex verts[5000];
  psVec velocities[5000];
  memset(velocities, 0, sizeof(psVec)*5000);
  psDriver* driver = engine->GetDriver();

  for(int i = 0; i < 5000; ++i)
  {
    verts[i].x = bssrandreal(0, driver->screendim.x);
    verts[i].y = bssrandreal(0, driver->screendim.y);
    verts[i].z = 0;
    verts[i].w = bssrandreal(8, 32);
    verts[i].color = 0x88FFFFFF;
  }

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), globalcam.GetPivot(), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->rawscreendim), psCamera::default_extent);

    for(int i = 0; i < 5000; ++i)
    {
      verts[i].x += velocities[i].x;
      verts[i].y += velocities[i].y;
      double s = dist(verts[i].x, verts[i].y, driver->screendim.x / 2, driver->screendim.y / 2) / 2000.0; // This gets the distance from the center, scaling the outer boundary at 2000 to 1.0
      double d = bssrandreal(-1.0, 1.0);
      d = std::pow(abs(d), 1.0 + s*s) * ((d < 0.0) ? -1.0 : 1.0); // This causes the distribution of d to tend towards zero at a higher rate the farther away a particle is from the center. This allows particles to move freely near the center, but get dragged towards it farther away.
      velocities[i] += psVec::FromPolar(0.004, bssfmod<double>(d*PI - atan2(verts[i].y - driver->screendim.y / 2, -verts[i].x + driver->screendim.x / 2), PI_DOUBLE)); // At d = 0, the velocity will always point towards the center.
    }

    driver->SetTextures(&particle, 1);
    driver->DrawPoints(driver->library.PARTICLE, STATEBLOCK_LIBRARY::GLOW, verts, 5000, 0);
    engine->End();
    updatefpscount(timer, fps);
  }

  ENDTEST;
}

TESTDEF::RETPAIR test_psPass()
{
  BEGINTEST;

  int fps = 0;
  auto timer = psEngine::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  cStr shfile = ReadFile("../media/motionblur.hlsl");
  auto shader = psShader::MergeShaders(2, driver->library.IMAGE, psShader::CreateShader(0, 0, 1, &SHADER_INFO(shfile.c_str(), "mainPS", PIXEL_SHADER_4_0)));

  psImage image(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_PREMULTIPLY_SRGB, false));
  psImage image2(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_ALPHABOX, 0, FILTER_NONE, false));
  psImage image3(psTex::Create("../media/blendtest.png", USAGE_SHADER_RESOURCE, FILTERS(FILTER_LINEAR|FILTER_SRGB_MIPMAP), 0, FILTER_NONE, false));
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
    updatefpscount(timer, fps);
  }
  ENDTEST;
}

TESTDEF::RETPAIR test_psEffect()
{
  BEGINTEST;
  int fps = 0;
  auto timer = psEngine::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psTex* gamma = psTex::Create(driver->screendim, FMT_R8G8B8A8_SRGB, USAGE_RENDERTARGET | USAGE_SHADER_RESOURCE, 1);
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
    updatefpscount(timer, fps);
  }

  ENDTEST;
}

TESTDEF::RETPAIR test_feather()
{
  BEGINTEST;
  int fps = 0;
  auto timer = psEngine::OpenProfiler();

  fgSkin skin;
  fgSkin_Init(&skin);
  fgSkin* fgButtonSkin = fgSkin_AddSkin(&skin, "fgButton");
  fgSkin* fgResourceSkin = fgSkin_AddSkin(fgButtonSkin, "fgResource");

  FG_UINT nuetral = fgSkin_AddStyle(fgResourceSkin, "nuetral");
  FG_UINT active = fgSkin_AddStyle(fgResourceSkin, "active");
  FG_UINT hover = fgSkin_AddStyle(fgResourceSkin, "hover");
  FG_Msg msg = { 0 };
  msg.type = FG_SETCOLOR;
  msg.otherint = 0xFFFF00FF;
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgResourceSkin, nuetral), &msg, 0, 0, 0, 0);
  msg.otherint = 0xFF00FFFF;
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgResourceSkin, hover), &msg, 0, 0, 0, 0);
  msg.otherint = 0xFFFFFF00;
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgResourceSkin, active), &msg, 0, 0, 0, 0);

  FG_UINT bnuetral = fgSkin_AddStyle(fgButtonSkin, "nuetral");
  FG_UINT bactive = fgSkin_AddStyle(fgButtonSkin, "active");
  FG_UINT bhover = fgSkin_AddStyle(fgButtonSkin, "hover");
  msg.type = FG_SETTEXT;
  msg.other = "Nuetral";
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgButtonSkin, bnuetral), &msg, 0, 0, 0, 0);
  msg.other = "Active";
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgButtonSkin, bactive), &msg, 0, 0, 0, 0);
  msg.other = "Hover";
  fgStyle_AddStyleMsg(fgSkin_GetStyle(fgButtonSkin, bhover), &msg, 0, 0, 0, 0);

  void* font = fgCreateFont(0, "arial.ttf", 14, 16, 0);
  msg.type = FG_SETCOLOR;
  msg.otherint = 0xFFFFFFFF;
  fgStyle_AddStyleMsg(&fgButtonSkin->style, &msg, 0, 0, 0, 0);
  msg.type = FG_SETFONT;
  msg.other = font;
  fgStyle_AddStyleMsg(&fgButtonSkin->style, &msg, 0, 0, 0, 0);

  fgChild_VoidMessage((fgChild*)fgSingleton(), FG_SETSKIN, &skin);

  fgChild* res = fgResource_Create(fgCreateResourceFile(0, "../media/circle.png"), 0, 0xFFFFFFFF, FGCHILD_EXPAND | FGCHILD_IGNORE, 0, 0, 0);
  fgChild* button = fgButton_Create(0, FGCHILD_EXPAND, (fgChild*)fgSingleton(), 0, 0);
  fgChild_VoidMessage(button, FG_ADDITEM, res);

  const fgElement fgElement_topwindow = { { 0, 0.2, 0, 0.2, 0, 0.8, 0, 0.8 }, 0, { 0, 0, 0, 0 } };
  fgChild* topwindow = fgTopWindow_Create("test window", 0, &fgElement_topwindow);
  AbsRect r = { 10,22,10,10 };
  fgChild_IntMessage(topwindow, FG_SETCOLOR, 0xFFFFFFFF, 0);
  fgChild_VoidMessage(topwindow, FG_SETFONT, font);
  fgChild_VoidMessage(topwindow, FG_SETPADDING, &r);

  while(!gotonext && engine->Begin())
  {
    psRoot::Instance()->Render();
    engine->GetDriver()->Clear(0xFF000000);
    engine->End();
    updatefpscount(timer, fps);
  }


  fgSkin_Destroy(&skin);

  ENDTEST;
}

TESTDEF::RETPAIR test_psFont()
{
  BEGINTEST;

  psFont* font = psFont::Create("arial.ttf", 14, 0, psFont::FAA_LCD);

  int fps=0;
  auto timer = psEngine::OpenProfiler();
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
    font->DrawText(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, "the dog jumped \nover the lazy fox", psRect(0, 0, 100, 0), TDT_WORDBREAK, 0, 0xCCFFAAFF);

    const psTex* t = font->GetTex();
    driver->SetTextures(&t, 1);
    driver->DrawRect(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, psRectRotateZ(0, 100, t->GetDim().x, 100 + t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    //driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(0, 100, t->GetDim().x, 100+t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    engine->End();

    updatefpscount(timer, fps);
  }
  ENDTEST;
}

TESTDEF::RETPAIR test_psVector()
{
  BEGINTEST;

  int fps = 0;
  auto timer = psEngine::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psQuadraticCurve curve(psVec(100), psVec(200, 200), psVec(300), 4.25f);
  //psQuadraticCurve curve(psVec(186.236328, 105.025391), psVec(202.958221, 106.747375), psVec(220.288574, 109.005638), 4.25f);
  //engine->GetPass(0)->Insert(&curve);

  //psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(793, 213), psVec(300), 4.25f);
  psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(927, 115), psVec(300), 4.25f);
  engine->GetPass(0)->Insert(&curve2);
  
  psRoundedRect rect(psRectRotateZ(400, 300, 550, 400, 0), 0);
  rect.SetCorners(psRect(30, 10, 30, 0));
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
    updatefpscount(timer, fps);
    curve.Set(psVec(100), engine->GetMouse(), psVec(300));
    curve2.Set(psVec(100), psVec(300, 100), engine->GetMouse(), psVec(300));
    circle.SetArcs(psRect(atan2(-engine->GetMouse().y + circle.GetPosition().y, engine->GetMouse().x - circle.GetPosition().x) - 0.5, 1.0, 0, bssfmod(engine->GetTime()*0.001,PI_DOUBLE)));
  }

  ENDTEST;
}

TESTDEF::RETPAIR test_psTileset()
{
  BEGINTEST;

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

  TESTDEF tests[] ={
    { "psVector", &test_psVector },
    { "psDirectX11", &test_psDirectX11 },
    { "psFont", &test_psFont },
    { "psPass", &test_psPass },
    { "psCircle", &test_psCircle },
    { "psEffect", &test_psEffect },
    { "psRect", &test_psRect },
    { "psColor", &test_psColor },
    { "psParticles", &test_psParticles },
    { "psInheritable", &test_psInheritable },
    { "ps_feather", &test_feather },
  };

  const size_t NUMTESTS=sizeof(tests)/sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint32_t)PS_VERSION_MAJOR << '.' << (uint32_t)PS_VERSION_MINOR << '.' <<
    (uint32_t)PS_VERSION_REVISION << ": Unit Tests\nCopyright (c)2015 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] ={ 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");


  std::vector<uint32_t> failures;
  PSINIT init;
  init.driver=RealDriver::DRIVERTYPE_DX11;
  init.width=640;
  init.height=480;
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

    std::function<bool(const psGUIEvent&)> guicallback =[&](const psGUIEvent& evt) -> bool
    { 
      if(evt.type == GUI_KEYDOWN || evt.type == GUI_KEYUP)
      {
        bool isdown = evt.type == GUI_KEYDOWN;
        dirkeys[8] = evt.IsShiftDown();
        switch(evt.keycode)
        {
        case KEY_A: dirkeys[0] = isdown; break;
        case KEY_D: dirkeys[1] = isdown; break;
        case KEY_W: dirkeys[2] = isdown; break;
        case KEY_S: dirkeys[3] = isdown; break;
        case KEY_X:
        case KEY_OEM_PLUS: dirkeys[4] = isdown; break;
        case KEY_Z:
        case KEY_OEM_MINUS: dirkeys[5] = isdown; break;
        case KEY_Q: dirkeys[6] = isdown; break;
        case KEY_E: dirkeys[7] = isdown; break;
        case KEY_ESCAPE:
          if(isdown) ps.Quit();
          break;
        case KEY_RETURN:
          if(isdown && !evt.IsAltDown()) gotonext = true;
          break;
        }
      }
      return false;
    };
    ps.SetInputReceiver(guicallback);
    fgInitialize();
    //ps[0].SetClear(true, 0);
    engine=&ps;

    TESTDEF::RETPAIR numpassed;
    for(uint32_t i = 0; i < NUMTESTS; ++i)
    {
      gotonext = false;
      globalcam.SetPosition(psVec3D(0, 0, -1));
      globalcam.SetRotation(0);
      numpassed=tests[i].FUNC(); //First is total, second is succeeded
      if(numpassed.first!=numpassed.second) failures.push_back(i);

      printf("%-*s %*s %-*s\n", COLUMNS[0], tests[i].NAME, COLUMNS[1], cStrF("%u/%u", numpassed.second, numpassed.first).c_str(), COLUMNS[2], (numpassed.first==numpassed.second)?"PASS":"FAIL");
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

  PROFILE_OUTPUT("ps_profiler.txt", 1|4);
  //end program
  return 0;
}

struct HINSTANCE__;

// WinMain function, simply a catcher that calls the main function
int __stdcall WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int nShowCmd)
{
  main(0, (char**)hInstance);
}