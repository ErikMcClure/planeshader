// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in testbed.cpp

#include "testbed.h"
#include "psTex.h"
#include "psLayer.h"
#include "psVector.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psVector()
{
  BEGINTEST;
  HighPrecisionTimer time;
  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psQuadraticCurve curve(psVec(100), psVec(200, 200), psVec(300), 4.25f);
  //psQuadraticCurve curve(psVec(186.236328, 105.025391), psVec(202.958221, 106.747375), psVec(220.288574, 109.005638), 4.25f);
  //engine->GetLayer(0)->Insert(&curve);

  //psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(793, 213), psVec(300), 4.25f);
  psCubicCurve curve2(psVec(100), psVec(300, 100), psVec(927, 115), psVec(300), 4.25f);
  engine->GetLayer(0)->Insert(&curve2);

  psRoundRect rect(psRectRotateZ(400, 300, 550, 400, 0), 0);
  rect.SetCorners(psRect(30, 10, 30, 0));
  rect.SetColor(0xAAFFFFFF);
  rect.SetOutlineColor(0xFF0000FF);
  rect.SetOutline(0.5);

  psRoundTri tri(psRectRotateZ(300, 50, 400, 150, 0), 0);
  tri.SetCorners(psRect(30, 10, 30, 0));
  tri.SetColor(0xAAFFFFFF);
  tri.SetOutlineColor(0xFF0000FF);
  tri.SetOutline(5);

  psRenderCircle circle(50, psVec3D(200, 350, 0));
  circle.SetOutlineColor(0xFF0000FF);
  circle.SetOutline(5);

  engine->GetLayer(0)->Insert(&rect);
  engine->GetLayer(0)->Insert(&tri);
  engine->GetLayer(0)->Insert(&circle);

  engine->GetLayer(0)->SetClearColor(0xFF999999);
  engine->GetLayer(0)->SetCamera(&globalcam);

  Matrix<float, 4, 4> m;
  Matrix<float, 4, 4>::AffineScaling(0.8, 0.8, 1.0f, m);

  while(!gotonext && engine->Begin())
  {
    processGUI();
    engine->GetDriver()->PushTransform(m.v);
    engine->End();
    engine->GetDriver()->PopTransform();
    time.Update();
    engine->FlushMessages();
    updatefpscount(timer, fps);
    curve.Set(psVec(100), engine->GetMouse(), psVec(300));
    curve2.Set(psVec(100), psVec(300, 100), engine->GetMouse(), psVec(300));
    tri.SetCorners(psRect(30, 10, 30, (engine->GetMouse().x - tri.GetPosition().x)/tri.GetDim().x));
    circle.SetArcs(psRect(atan2(-engine->GetMouse().y + circle.GetPosition().y, engine->GetMouse().x - circle.GetPosition().x) - 0.5, 1.0, 0, bssFMod(time.GetTime()*0.001, PI_DOUBLE)));
  }

  ENDTEST;
}