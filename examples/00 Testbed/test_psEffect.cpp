// Copyright ©2017 Black Sphere Studios

#include "testbed.h"
#include "psTex.h"
#include "psPass.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psEffect()
{
  BEGINTEST;
  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
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
