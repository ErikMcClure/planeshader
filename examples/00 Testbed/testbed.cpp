// Example 00 - Testbed
// -------------------------
// This example runs a series of verification tests to ensure Planeshader is working properly.
//
// Copyright ©2017 Black Sphere Studios


#include "testbed.h"
#include "ps_feather.h"
#include "feathergui/fgDebug.h"
#include "bss-util/Str.h"
#include "bss-util/profiler.h"
#include "bss-util/algo.h"
#include "bss-util/win32_includes.h"
#include <time.h>
#include <iostream>
#include <functional>

#undef DrawText

using namespace planeshader;
using namespace bss;

Logger _failedtests("../bin/failedtests.txt"); //This is spawned too early for us to save it with SetWorkDirToCur();
psEngine* engine = 0;
psCamera globalcam;
bool dirkeys[9] = { false }; // left right up down in out counterclockwise clockwise shift
float dirspeeds[4] = { 300.0f, 300.0f, 2.0f, 1.0f }; // X Y Z R
bool gotonext = false;

#pragma warning(disable:4244)

bool comparevec(psVec a, psVec b, int diff)
{
  return b.x == 0.0f ? fSmall(a.x, FLT_EPS * 4) : fCompare(a.x, b.x, diff) && b.y == 0.0f ? fSmall(a.y, FLT_EPS * 4) : fCompare(a.y, b.y, diff);
}
bool comparevec(psColor& a, psColor& b, int diff)
{
  return b.r == 0.0f ? fSmall(a.r) : fCompare(a.r, b.r, diff) &&
    b.g == 0.0f ? fSmall(a.g) : fCompare(a.g, b.g, diff) &&
    b.b == 0.0f ? fSmall(a.b) : fCompare(a.b, b.b, diff) &&
    b.a == 0.0f ? fSmall(a.a) : fCompare(a.a, b.a, diff);
}
Str ReadFile(const char* path)
{
  FILE* f;
  FOPEN(f, path, "rb");
  if(!f) return "";
  Str buf;
  fseek(f, 0, SEEK_END);
  long ln = ftell(f);
  buf.resize(ln + 1);
  fseek(f, 0, SEEK_SET);
  fread(buf.UnsafeString(), 1, ln, f);
  return std::move(buf);
}

void processGUI()
{
  //Sleep(10); // For some reason, limiting framerate can reduce jitter in windowed mode.
  static HighPrecisionTimer delta;
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

void updatefpscount(uint64_t& timer, int& fps)
{
  if(HighPrecisionTimer::CloseProfiler(timer)>1000000000)
  {
    timer = HighPrecisionTimer::OpenProfiler();
    char text[10] = { 0,0,0,0,0,0,0,0,0,0 };
    _itoa_r(fps, text, 10);
    engine->GetMonitor()->element.SetText(text);
    fps = 0;
  }
  ++fps;
}

TESTDEF::RETPAIR test_psCircle()
{
  BEGINTEST;
  TEST(psCircle(1.5f, 1.5f, 2).ContainsPoint(2, 2));
  TEST(!psCircle(1.5f, 1.5f, 0.1f).ContainsPoint(2, 2));
  ENDTEST;
}

TESTDEF::RETPAIR test_psRect()
{
  BEGINTEST;
  psRect rct(-1.0f, 0.0f, 1.0f, 2.0f);
  TEST(!rct.ContainsPoint(2, 2));
  TEST(!rct.ContainsPoint(1, 2));
  TEST(rct.ContainsPoint(-1, 0));
  TEST(rct.ContainsPoint(0, 1));
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

TESTDEF::RETPAIR test_psOpenGL4()
{
  BEGINTEST;
  ENDTEST;
}

// Main program function
int main(int argc, char** argv)
{
  ForceWin64Crash();
  SetWorkDirToCur();
  bssRandSeed(time(NULL));

  AllocConsole();
  freopen("CONOUT$", "wb", stdout);
  freopen("CONOUT$", "wb", stderr);
  freopen("CONIN$", "rb", stdin);

  TESTDEF tests[] = {
    { "psGeometry", &test_psGeometry },
    //{ "PBR_System", &test_PBR_System },
    { "ps_feather", &test_feather },
    { "psFont", &test_psFont },
    { "psDirectX11", &test_psDirectX11 },
    { "psTileset", &test_psTileset },
    { "psVector", &test_psVector },
    { "psLayer", &test_psPass },
    { "psCircle", &test_psCircle },
    { "psEffect", &test_psEffect },
    { "psRect", &test_psRect },
    { "psColor", &test_psColor },
    { "psParticles", &test_psParticles },
  };

  const size_t NUMTESTS = sizeof(tests) / sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint32_t)psEngine::Version.Major << '.' << (uint32_t)psEngine::Version.Minor << '.' <<
    (uint32_t)psEngine::Version.Revision << ": Unit Tests\nCopyright (c)2017 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] = { 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");

  std::vector<uint32_t> failures;
  PSINIT init;
  init.driver = RealDriver::DRIVERTYPE_DX11;
  //init.driver = RealDriver::DRIVERTYPE_VULKAN;
  init.width = 640;
  init.height = 480;
  //init.vsync = true;
  //init.mode = psMonitor::MODE_FULLSCREEN;
  //init.mode = psMonitor::MODE_BORDERLESS;
  //init.mode = psMonitor::MODE_COMPOSITE;
  //init.antialias = 8;
  init.sRGB = false;
  //init.iconresource=101;
  //init.filter=5;
  {
    psEngine ps(init);
    if(ps.GetQuit()) return 0;
    psCamera::default_extent.x = 0.2f;
    psCamera::default_extent.y = 100;
    globalcam.SetExtent(psCamera::default_extent);
    ps.CaptureAllJoy(ps.GetMonitor()->GetWindow());
    fgSetInjectFunc([](struct _FG_ROOT* self, const FG_Msg* msg) -> size_t
    {
      if(msg->type == FG_KEYDOWN || msg->type == FG_KEYUP)
      {
        bool isdown = msg->type == FG_KEYDOWN;
        dirkeys[8] = msg->IsShiftDown();
        switch(msg->keycode)
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
          if(isdown) psEngine::Instance()->Quit();
          break;
        case FG_KEY_RETURN:
          if(isdown && !msg->IsAltDown()) gotonext = true;
          break;
        case FG_KEY_F11:
          if(isdown)
          {
            if(fgDebug_Get() != 0 && !(fgDebug_Get()->tabs->flags&FGELEMENT_HIDDEN))
              fgDebug_Hide();
            else
              fgDebug_Show(0, false);
          }
        }
      }
      return fgRoot_DefaultInject(self, msg);
    });
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

      printf("%-*s %*s %-*s\n", COLUMNS[0], tests[i].NAME, COLUMNS[1], StrF("%u/%u", numpassed.second, numpassed.first).c_str(), COLUMNS[2], (numpassed.first == numpassed.second) ? "PASS" : "FAIL");
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