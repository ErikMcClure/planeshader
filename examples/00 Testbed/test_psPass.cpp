// Copyright �2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in testbed.cpp

#include "testbed.h"
#include "psImage.h"
#include "psLayer.h"
#include "psRenderGeometry.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psPass()
{
  BEGINTEST;

  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  Str shfile = ReadFile("../media/motionblur.hlsl");
  auto shader = psShader::MergeShaders(2, driver->library.IMAGE, psShader::CreateShader(0, 0, 1, &SHADER_INFO(shfile.c_str(), "mainPS", PIXEL_SHADER_4_0)));

  psImage image(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_PREMULTIPLY_SRGB, false));
  psImage image2(psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_ALPHABOX, 0, FILTER_NONE, false));
  psImage image3(psTex::Create("../media/blendtest.png", USAGE_SHADER_RESOURCE, FILTERS(FILTER_LINEAR | FILTER_SRGB_MIPMAP), 0, FILTER_NONE, false));
  psRenderLine line(psLine3D(0, 0, 0, 100, 100, 4));
  //engine->GetLayer(0)->Insert(&image2);
  engine->GetLayer(0)->Insert(&image);
  //engine->GetLayer(0)->Insert(&image3);
  //engine->GetLayer(0)->Insert(&line);
  engine->GetLayer(0)->SetClearColor(0xFF000000);
  engine->GetLayer(0)->SetCamera(&globalcam);
  //globalcam.SetPositionZ(-4.0);
  image3.SetScale(psVec(0.5));

  image.SetStateblock(STATEBLOCK_LIBRARY::PREMULTIPLIED);
  if(!image.GetTexture())
    ENDTEST;
  image.GetTexture()->SetTexblock(STATEBLOCK_LIBRARY::UVBORDER);
  image.ApplyEdgeBuffer();
  image2.SetShader(shader);

  psVertex rect[4]{
    { 300,300,0,1,0xFF00FF00 },
    { 300,400,0,1,0xFF0000FF },
    { 400,400,0,1,0xFF0000FF },
    { 400,300,0,1,0xFF00FF00 },
  };

  psImage image4(psTex::Create("../media/radial.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_NONE, true), psVec3D(300, 100, 0));
  image4.SetColor(0xFF00FF00);

  psVertex rect2[4]{
    { 300,100,0,1,0xFF0000FF },
    { 300,200,0,1,0xFF0000FF },
    { 400,200,0,1,0xFF0000FF },
    { 400,100,0,1,0xFF0000FF },
  };

  while(!gotonext && engine->Begin())
  {
    for(int i = 0; i < 200; ++i)
    {
      image3.Render(psTransform2D::Zero);
      image.Render(psTransform2D::Zero);
    }
    psRenderPolygon::DrawPolygon(rect2, 4);
    image4.Render(psTransform2D::Zero);
    psRenderPolygon::DrawPolygon(rect, 4);
    processGUI();
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }
  ENDTEST;
}