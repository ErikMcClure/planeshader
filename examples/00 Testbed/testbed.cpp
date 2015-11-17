// Example 00 - Testbed
// -------------------------
// This example runs a series of verification tests to ensure Planeshader is working properly.
//
// Copyright ©2015 Black Sphere Studios

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
float dirspeeds[4] = { 300.0f, 300.0f, 2.0f, 1.0f };
bool gotonext = false;

#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader64_d.lib")
#pragma comment(lib, "../../lib/bss-util64_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader64.lib")
#pragma comment(lib, "../../lib/bss-util64.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../../bin/PlaneShader_d.lib")
#pragma comment(lib, "../../lib/bss-util_d.lib")
#else
#pragma comment(lib, "../../bin/PlaneShader.lib")
#pragma comment(lib, "../../lib/bss-util.lib")
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
#define TESTARRAY(t,f) _ITERFUNC(__testret,t,[&](uint i) -> bool { f });
#define TESTALL(t,f) _ITERALL(__testret,t,[&](uint i) -> bool { f });
#define TESTCOUNT(c,t) { for(uint i = 0; i < c; ++i) TEST(t) }
#define TESTCOUNTALL(c,t) { bool __val=true; for(uint i = 0; i < c; ++i) __val=__val&&(t); TEST(__val); }
#define TESTFOUR(s,a,b,c,d) TEST(((s)[0]==(a)) && ((s)[1]==(b)) && ((s)[2]==(c)) && ((s)[3]==(d)))
#define TESTALLFOUR(s,a) TEST(((s)[0]==(a)) && ((s)[1]==(a)) && ((s)[2]==(a)) && ((s)[3]==(a)))
#define TESTRELFOUR(s,a,b,c,d) TEST(fcompare((s)[0],(a)) && fcompare((s)[1],(b)) && fcompare((s)[2],(c)) && fcompare((s)[3],(d)))
#define TESTVEC(v,cx,cy) TEST((v).x==(cx)) TEST((v).y==(cy))
#define TESTVEC3(v,cx,cy,cz) TEST((v).x==(cx)) TEST((v).y==(cy)) TEST((v).z==(cz))

template<class T, size_t SIZE, class F>
void _ITERFUNC(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { for(uint i = 0; i < SIZE; ++i) TEST(f(i)) }
template<class T, size_t SIZE, class F>
void _ITERALL(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { bool __val=true; for(uint i = 0; i < SIZE; ++i) __val=__val&&(f(i)); TEST(__val); }

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

void updatefpscount(unsigned __int64& timer, int& fps)
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
  psTex* pslogo = psTex::Create("../media/pslogo.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_NONE, psVeciu(psDriver::BASE_DPI), false);
  const int NUMBATCH = 30;
  psDriver* driver = engine->GetDriver();
  globalcam.SetPosition(500, 500);

  psVec imgpos[NUMBATCH];
  for(int i = 0; i < NUMBATCH; ++i) imgpos[i] = psVec(RANDINTGEN(0, driver->rawscreendim.x), RANDINTGEN(0, driver->rawscreendim.y));

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), psVec(300,300), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->rawscreendim));
    driver->library.IMAGE->Activate();
    driver->DrawRect(psRectRotateZ(500, 500, 500+pslogo->GetDim().x, 500+pslogo->GetDim().y, 0.0f, pslogo->GetDim()*0.5f), &RECT_UNITRECT, 1, 0xFFFFFFFF, &pslogo, 1, 0);
    driver->library.CIRCLE->Activate();
    driver->DrawRectBatchBegin(&pslogo, 1, 1, 0);
    for(int i = 0; i < NUMBATCH; ++i) {
      driver->DrawRectBatch(psRectRotateZ(imgpos[i].x, imgpos[i].y, imgpos[i].x+pslogo->GetDim().x, imgpos[i].y+pslogo->GetDim().y, 0), &RECT_UNITRECT, 0xFFFFFFFF);
    }
    driver->DrawRectBatchEnd();

    driver->library.LINE->Activate();
    driver->DrawLinesStart(PSFLAG_FIXED);
    driver->DrawLines(psLine(0, 0, 100, 200), 0, 0, 0xFFFFFFFF);
    driver->DrawLines(psLine(50, 100, 1000, -2000), 0, 0, 0xFFFFFFFF);
    driver->DrawLinesEnd();

    driver->library.POLYGON->Activate();
    psVec polygon[5] ={ { 200, 0 }, { 200, 100 }, { 100, 150 }, { 60, 60 }, { 90, 30 } };
    driver->DrawPolygon(polygon, 5, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
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
  psDriver* driver = engine->GetDriver();

  for(int i = 0; i < 5000; ++i)
  {
    verts[i].x = bssrandreal(0, driver->screendim.x);
    verts[i].y = bssrandreal(0, driver->screendim.y);
    verts[i].z = 0;
    verts[i].w = 0;
    verts[i].color = 0x88FFFFFF;
  }

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), globalcam.GetPivot(), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->rawscreendim));
    driver->library.PARTICLE->Activate();
    driver->SetStateblock(STATEBLOCK_LIBRARY::GLOW->GetSB());
    driver->DrawPointsBegin(&particle, 1, 16, 0);
    driver->DrawPoints(verts, 5000);
    driver->DrawPointsEnd();
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

  psImage image(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_PREMULTIPLY_SRGB, psVeciu(psDriver::BASE_DPI), false));
  psImage image2(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_ALPHABOX, 0, FILTER_NONE, psVeciu(psDriver::BASE_DPI), false));
  psRenderLine line(psLine3D(0, 0, 0, 100, 100, 4));
  //engine->GetPass(0)->Insert(&image2);
  engine->GetPass(0)->Insert(&image);
  //engine->GetPass(0)->Insert(&line);
  engine->GetPass(0)->SetClearColor(0xFF000000);
  engine->GetPass(0)->SetCamera(&globalcam);
  //globalcam.SetPositionZ(-4.0);

  image.SetStateblock(STATEBLOCK_LIBRARY::PREMULTIPLIED);
  //image2.ApplyEdgeBuffer();

  while(!gotonext && engine->Begin(0))
  {
    processGUI();
    engine->End();
    updatefpscount(timer, fps);
  }
  ENDTEST;
}

TESTDEF::RETPAIR test_psEffect()
{
  BEGINTEST;

  // Draw everything normally on the gamma buffer
  // Calculate each light and it's shadows on one buffer
  // Then add this to the light accumulation buffer
  // All of the above can be done with one psEffect with the lights tied to the accumulation buffers, it will sort everything out properly.

  // Then draw this light accumulation buffer on to the gamma buffer and multiply everything
  // Copy the gamma buffer to the backbuffer
  // Draw sRGB GUI elements blended in sRGB space.

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
  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0xFF999999);
    driver->library.IMAGE0->Activate();
    driver->DrawRect(psRectRotateZ(0, 0, 100, 100, 0), 0, 0, 0xFF999900, 0, 0, PSFLAG_FIXED);
    //driver->library.IMAGE->Activate();
    //driver->SetStateblock(block->GetSB());
    driver->library.TEXT1->Activate();
    driver->SetStateblock(STATEBLOCK_LIBRARY::SUBPIXELBLEND1->GetSB());
    //font->DrawText("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla maximus sem at ante porttitor vehicula. Nulla a lorem imperdiet, consectetur metus id, congue enim. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin placerat a ipsum ac sodales. Curabitur vel neque scelerisque elit mollis convallis. Proin porta augue metus, sed pulvinar nisl mollis ut. Proin at aliquam erat. Quisque at diam tellus. Aenean facilisis justo ut mauris egestas dignissim. Maecenas scelerisque, ante ac blandit consectetur, magna sem pharetra massa, eu luctus orci ligula luctus augue. Integer at metus eros. Donec sed eros molestie, posuere nunc id, porta sem. \nMauris fermentum mauris ac eleifend ultrices.Fusce nec sollicitudin turpis, a ultricies metus.Nulla suscipit cursus orci, ac fringilla massa volutpat et.Nullam vestibulum dolor at tortor bibendum condimentum.Donec vitae faucibus risus, ut placerat mauris.Curabitur quis purus at urna pharetra lobortis.Pellentesque turpis velit, molestie aliquet elit sed, vestibulum rutrum nibh. \nSuspendisse ultricies leo nec ante accumsan ullamcorper.Suspendisse scelerisque molestie enim sit amet lacinia.Proin at lorem justo.Curabitur lectus ipsum, accumsan at quam eu, iaculis pellentesque felis.Fusce blandit feugiat dui, id placerat justo sollicitudin sed.Cras auctor lorem hendrerit leo facilisis porttitor.Sed vitae pulvinar purus, sed ornare ligula.\nPhasellus blandit, magna quis bibendum mattis, neque quam gravida quam, at tempus sem sapien eu mi.Phasellus ornare laoreet neque at blandit.Suspendisse vulputate fringilla fermentum.Fusce ante eros, laoreet ultricies eros sit amet, lobortis viverra elit.Curabitur consequat erat neque, in fringilla eros elementum eu.Quisque aliquam laoreet metus, volutpat vulputate tortor vehicula ut.Fusce sodales commodo justo, in condimentum ipsum aliquam at.Phasellus eget tellus ac arcu ultrices vehicula.Integer sagittis metus nibh, in varius mi scelerisque quis.Etiam ullamcorper gravida urna, et vestibulum velit posuere id.Aenean fermentum nibh ac dui rhoncus volutpat.Cras quis felis eget tortor vehicula interdum.In efficitur nulla quam, non condimentum ipsum pulvinar non.\nCras ultricies mi sed lacinia consequat.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Suspendisse potenti.Nam consectetur eleifend libero sed pharetra.Suspendisse in dolor dui.Sed imperdiet pellentesque fermentum.Vivamus ac tortor felis.Aliquam id turpis euismod, tincidunt sapien ac, varius sapien.Vivamus id nulla mauris.");
    font->DrawText("the dog jumped \nover the lazy fox", psRect(0, 0, 100, 0), TDT_WORDBREAK, 0, 0xCCFFAAFF);

    const psTex* t = font->GetTex();
    driver->DrawRect(psRectRotateZ(0, 100, t->GetDim().x, 100+t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, &t, 1, PSFLAG_FIXED);
    engine->End();
    driver->SetStateblock(psStateblock::DEFAULT->GetSB());

    updatefpscount(timer, fps);
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

  TESTDEF tests[] ={
    { "psCircle", &test_psCircle },
    { "psRect", &test_psRect },
    { "psColor", &test_psColor },
    { "psDirectX11", &test_psDirectX11 },
    { "psPass", &test_psPass },
    { "psFont", &test_psFont },
    { "psParticles", &test_psParticles },
    { "psEffect", &test_psEffect },
    { "psInheritable", &test_psInheritable },
  };

  const size_t NUMTESTS=sizeof(tests)/sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint)PS_VERSION_MAJOR << '.' << (uint)PS_VERSION_MINOR << '.' <<
    (uint)PS_VERSION_REVISION << ": Unit Tests\nCopyright (c)2015 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] ={ 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");


  std::vector<uint> failures;
  PSINIT init;
  init.driver=RealDriver::DRIVERTYPE_DX11;
  init.width=640;
  init.height=480;
  //init.mode = PSINIT::MODE_BORDERLESS;
  init.extent.x = 0.2;
  init.extent.y = 100;
  init.antialias = 8;
  //init.sRGB = true;
  init.mediapath = "../media";
  //init.iconresource=101;
  //init.filter=5;
  {
    psEngine ps(init);
    if(ps.GetQuit()) return 0;
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
    //ps[0].SetClear(true, 0);
    engine=&ps;

    TESTDEF::RETPAIR numpassed;
    for(uint i = 0; i < NUMTESTS; ++i)
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
    for(uint i = 0; i < failures.size(); i++)
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