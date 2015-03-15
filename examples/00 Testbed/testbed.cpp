// Example 00 - Testbed
// -------------------------
// This example runs a series of verification tests to ensure Planeshader is working properly.
//
// Copyright ©2014 Black Sphere Studios

#define _CRT_SECURE_NO_WARNINGS

#include "psEngine.h"
#include "psShader.h"
#include "psColor.h"
#include "psTex.h"
#include "bss-util/bss_win32_includes.h"
#include "bss-util/lockless.h"
#include "bss-util/cStr.h"
#include "bss-util/profiler.h"
#include "bss-util/bss_algo.h"
#include <time.h>
#include <iostream>
#include <functional>

using namespace planeshader;
using namespace bss_util;

cLog _failedtests("../bin/failedtests.txt"); //This is spawned too early for us to save it with SetWorkDirToCur();
psEngine* engine=0;

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

TESTDEF::RETPAIR test_psVec()
{
  BEGINTEST;

  psVeci a(3);
  TESTVEC(a, 3, 3);
  psVeci b(2, 3);
  TESTVEC(b, 2, 3);
  psVec c(4.0f);
  TESTVEC(c, 4.0f, 4.0f)
    psVec d(b);
  TESTVEC(d, 2.0f, 3.0f);
  float ee[2] ={ -1.0f, 1.0f };
  psVec e(ee);
  TESTVEC(e, -1.0f, 1.0f);
  TEST(psVeci(2, 2).IntersectCircle(psVeci(1, 2), 1));
  TEST(!psVeci(2, 2).IntersectCircle(psVeci(1, 1), 1));
  TEST(psVec(2, 2).IntersectCircle(psVec(1.5f, 1.5f), 2));
  TEST(!psVec(2, 2).IntersectCircle(psVec(1.5f, 1.5f), 0.1f));
  float rct[4] ={ -1.0f, 0.0f, 1.0f, 2.0f };
  TEST(!psVec(2, 2).IntersectRect(rct));
  TEST(psVec(2, 2).IntersectRect(rct)==psVec(2, 2).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(!psVec(1, 2).IntersectRect(rct)); // Test to make sure our rectangle intersection is inclusive-exclusive.
  TEST(psVec(1, 2).IntersectRect(rct)==psVec(1, 2).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(-1, 0).IntersectRect(rct));
  TEST(psVec(-1, 0).IntersectRect(rct)==psVec(-1, 0).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(0, 1).IntersectRect(rct));
  TEST(psVec(0, 1).IntersectRect(rct)==psVec(0, 1).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(2, 2).IntersectEllipse(psVec(1.5f, 1.5f), 2, 4));
  TEST(!psVec(2, 2).IntersectEllipse(psVec(1.5f, 1.5f), 0.1f, 0.2f));
  TEST(psVec(2, 2).IntersectEllipse(1.5f, 1.5f, 2, 4));
  TEST(!psVec(2, 2).IntersectEllipse(1.5f, 1.5f, 0.1f, 0.2f));
  TEST(psVec(2, 2).DistanceSquared(0, 0)==8.0f);
  TEST(psVec(2, 2).DistanceSquared(psVec(1, 1))==2.0f);
  TEST(psVec(-2, -2).DistanceSquared(1, 1)==18.0f);
  TEST(psVec(-2, -1).DistanceSquared(psVec(1, 0))==10.0f);
  TEST(fcompare(psVec(2, 2).Distance(psVec(0, 0)), 2.82843f ,100));
  TEST(fcompare(psVec(2, 2).Distance(1, 1), 1.4142136f, 100));
  TEST(fcompare(psVec(-2, -2).Distance(psVec(1, 1)), 4.24264f, 100));
  TEST(fcompare(psVec(-2, -1).Distance(1, 0), 3.16228f, 100));
  TEST(psVec(2, 2).DotProduct(3, 3)==12.0f);
  TEST(psVec(2, 2).DotProduct(psVec(3, 3))==12.0f);
  TEST(psVec(2, 2).DotProduct(0, 0)==0.0f);
  TEST(psVec(0, 0).DotProduct(psVec(3, 3))==0.0f);
  TEST(psVec(1, 2).CrossProduct(3, 4)==-2.0f);
  TEST(psVec(3, 4).CrossProduct(psVec(1, 2))==2.0f);
  TEST(psVec(0, 0).Length()==0.0f);
  TEST(fcompare(psVec(1, 1).Length(), 1.41421f, 100));
  TEST(fcompare(psVec(2, 4).Length(), 4.47214f, 100));
  TEST(psVec(1, 1)==psVec(1, 1));
  TEST(!(psVec(1, 1)==psVec(2, 1)));
  TEST(!(psVec(2, 2)==psVec(1, 1)));
  TEST(!(psVec(1, 1)!=psVec(1, 1)));
  TEST(psVec(1, 1)!=psVec(2, 1));
  TEST(psVec(2, 2)!=psVec(1, 1));
  TEST(psVec(1, 1)==1.0f);
  TEST(psVec(1, 1)!=2.0f);
  TEST(psVec(2, 1)!=2.0f);
  TEST(!(psVec(1, 1)==2.0f));
  TEST(!(psVec(2, 1)==1.0f));
  TEST(psVec(2, 2)==2.0f);
  //TESTERROR(psVec(0,0).Normalize(),...);
  psVec na(1, 1);
  psVec nb(2, 2);
  na=na.Normalize();
  nb=nb.Normalize();
  TEST(na==nb);
  TEST(fcompare(na.x, 0.707107f, 100) && fcompare(na.y, 0.707107f, 100));
  psVec nc(1, 4);
  nc=nc.Normalize();
  TEST(fcompare(nc.x, 0.242536f, 100) && fcompare(nc.y, 0.970143f, 100));
  TEST(comparevec(psVec(0, 4).Normalize(), psVec(0,1), 100));
  TEST(psVec(-2, -2).Abs()==psVec(2, 2));
  TEST(psVec(2, -2).Abs()==psVec(2, 2));
  TEST(psVec(2, 2).Abs()==psVec(2, 2));
  TEST(comparevec(psVec(2, 3).Rotate(0, 0, 0),psVec(2, 3),100));
  TEST(comparevec(psVec(2, 3).Rotate(PI_DOUBLEf, psVec(1, 1)), psVec(2, 3), 100));
  TEST(comparevec(psVec(2, 3).Rotate(PIf, 0, 0), psVec(-2, -3), 100));
  TEST(comparevec(psVec(2, 3).Rotate(PIf, psVec(1, 1)), psVec(-1, -1), 100));
  float mm[4] ={ cos(PIf), sin(PIf), -sin(PIf), cos(PIf) };
  TEST(comparevec(psVec(2, 3).MatrixMultiply(mm), psVec(-2, -3), 100));
  TEST(comparevec((psVec(1, 1)+psVec(2, 2)-(psVec(3, 1)*((-psVec(-2, 2))/psVec(-1, 2)))), psVec(9, 4), 100));
  TEST(comparevec(((((psVec(1, 1)+5.0f)+psVec(0, 0)-3.0f+psVec(0, 0))/5.0f)*2.0f), psVec(24.0f/20.0f), 100));
  TEST((psVec(1, 1)+=psVec(1, 1))==psVec(2, 2));
  TEST((psVec(1, 1)-=psVec(-1, 1))==psVec(2, 0));
  TEST((psVec(1, 1)*=psVec(-1, 2))==psVec(-1, 2));
  TEST((psVec(1, 4)/=psVec(-1, 2))==psVec(-1, 2));
  TEST((psVec(1, 1)+=1.0f)==psVec(2, 2));
  TEST((psVec(1, 1)-=1.0f)==psVec(0, 0));
  TEST((psVec(1, 2)*=-2.0f)==psVec(-2, -4));
  TEST((psVec(4, 2)/=2.0f)==psVec(2, 1));
  TEST(psVec(2, 2)<psVec(3, 3));
  TEST(psVec(2, 3)<psVec(3, 3));
  TEST(!(psVec(3, 3)<psVec(3, 3)));
  TEST(psVec(3, 3)>=psVec(3, 3));
  TEST(psVec(3, 4)>=psVec(3, 3));
  TEST(psVec(3, 3)>psVec(2, 3));
  TEST(psVec(3, 3)>psVec(2, 2));
  TEST(!(psVec(3, 3)>psVec(3, 3)));
  TEST(psVec(3, 3)<=psVec(3, 3));
  TEST(psVec(3, 3)<=psVec(3, 4));
  TEST(psVec(3, 3)<=3);
  TEST(!(psVec(4, 3)>3));
  TEST(psVec(4, 4)>3);
  TEST(psVec(2, 2)<3);
  TEST(!(psVec(2, 2)>=3));
  TEST(psVec(2, 2)>=2);
  TEST(psVec(2, 2)>=1);
  psVec f(3, 3);
  f=psVeci(2, 2);
  TEST(f==psVec(2, 2));
  f=psVec(2.5f, 2.5f);
  TEST(f==psVec(2.5f, 2.5f));
  psVeci g(2, 2);
  g=f;
  TEST(g==psVeci(2, 2));
  psVec ln(2, 4);
  ln=ln.Normalize();
  psVec la(1, 3);
  psVec lp(2, -3);
  psVec dd = (la - lp) - (ln*((la-lp).DotProduct(ln)));
  TEST(lp.LineInfDistanceSqr(1, 3, 1+ln.x, 3+ln.y)==dd.DotProduct(dd));
  TEST(fcompare(lp.LineInfDistance(1, 3, 1+ln.x, 3+ln.y),dd.Length(),100));
  TEST(lp.LineDistanceSqr(1, 3, 1+ln.x, 3+ln.y)==bss_util::distsqr(1, 3, 2, -3));
  TEST(fcompare(lp.LineDistance(1, 3, 1+ln.x, 3+ln.y),bss_util::dist<float>(1, 3, 2, -3),100));

  TEST(comparevec(psVec::ToPolar(psVec(3, 4)), psVec(5, 0.9273f), 1000));
  TEST(comparevec(psVec::ToPolar(psVec(0, 4)), psVec(4, PI_HALFf), 1000));
  TEST(comparevec(psVec::ToPolar(psVec(3, 0)), psVec(3, 0), 1000));
  TEST(comparevec(psVec::ToPolar(-3, 0), psVec(3, PIf), 1000));
  TEST(comparevec(psVec::ToPolar(0, -4), psVec(4, -PI_HALFf), 1000));
  TEST(comparevec(psVec::FromPolar(5, 0.9273f), psVec(3, 4), 1000));
  TEST(comparevec(psVec::FromPolar(4, -PI_HALFf), psVec(0, -4), 1000));
  TEST(comparevec(psVec::FromPolar(psVec(3, 0)), psVec(3, 0), 1000));
  TEST(comparevec(psVec::FromPolar(psVec(3, PIf)), psVec(-3, 0), 1000));
  TEST(comparevec(psVec::FromPolar(psVec(4, PIf+PI_HALFf)), psVec(0, -4), 1000));

  /*
  static BSS_FORCEINLINE bool BSS_FASTCALL IntersectEllipse(T X, T Y, T A, T B, T x, T y) { T tx=X-x; T ty=Y-y; return ((tx*tx)/(A*A)) + ((ty*ty)/(B*B)) <= 1; }
  static inline void BSS_FASTCALL EllipseNearestPoint(T A, T B, T cx, T cy, T& outX, T& outY)*/

  ENDTEST;
}
TESTDEF::RETPAIR test_psCircle()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psRect()
{
  BEGINTEST;

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

TESTDEF::RETPAIR test_psCamera()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX10()
{
  BEGINTEST;
  cStr shfile = ReadFile("../media/testbed.hlsl");
  auto shader = psShader::CreateShader(0, 0, 2, &SHADER_INFO(shfile.c_str(), (const char*)"vs_main", VERTEX_SHADER_4_0),
    &SHADER_INFO(shfile.c_str(), "ps_main", PIXEL_SHADER_4_0));
  auto timer = psEngine::OpenProfiler();
  int fps=0;
  psTex* pslogo = psTex::Create("../media/pslogo.png");

  while(engine->Begin())
  {
    shader->Activate();
    engine->GetDriver()->library.PARTICLE->Activate();
    //engine->GetDriver()->DrawFullScreenQuad();
    engine->GetDriver()->DrawRect(psRectRotateZ(100, 100, 200, 200, 0), RECT_UNITRECT, 0xFFFFFFFF, &pslogo, 1, 0);
    engine->End();
    if(psEngine::CloseProfiler(timer)>1000000000)
    {
      timer = psEngine::OpenProfiler();
      char text[10]={ 0,0,0,0,0,0,0,0,0,0 };
      _itoa_r(fps, text, 10);
      engine->SetWindowTitle(text);
      fps=0;
    }
    ++fps;
  }

  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX9()
{
  BEGINTEST;
  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX11()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psRenderable()
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
    { "psVec", &test_psVec },
    { "psCircle", &test_psCircle },
    { "psRect", &test_psRect },
    { "psColor", &test_psColor },
    { "psDirectX10", &test_psDirectX10 },
  };

  const size_t NUMTESTS=sizeof(tests)/sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint)PS_VERSION_MAJOR << '.' << (uint)PS_VERSION_MINOR << '.' <<
    (uint)PS_VERSION_REVISION << ": Unit Tests\nCopyright (c)2014 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] ={ 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");


  std::vector<uint> failures;
  PSINIT init;
  init.driver=RealDriver::DRIVERTYPE_DX10;
  init.width=640;
  init.height=480;
  //init.iconresource=101;
  //init.filter=5;
  {
    psEngine ps(init);
    if(ps.GetQuit()) return 0;
    std::function<bool(const psGUIEvent&)> guicallback =[&](const psGUIEvent& evt) -> bool { if(evt.type == GUI_KEYDOWN && evt.keycode == KEY_ESCAPE) ps.Quit(); return false; };
    ps.SetInputReceiver(guicallback);
    //ps[0].SetClear(true, 0);
    engine=&ps;

    TESTDEF::RETPAIR numpassed;
    for(uint i = 0; i < NUMTESTS; ++i)
    {
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