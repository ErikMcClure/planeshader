// Copyright ©2016 Black Sphere Studios

#include "testbed.h"
#include "psImage.h"
#include "psPass.h"
#include "psRenderGeometry.h"

using namespace bss_util;
using namespace planeshader;

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