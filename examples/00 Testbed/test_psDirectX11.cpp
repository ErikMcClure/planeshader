// Copyright ©2017 Black Sphere Studios

#include "testbed.h"
#include "psTex.h"
#include "psPass.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psDirectX11()
{
  BEGINTEST;
  Str shfile = ReadFile("../media/testbed.hlsl");
  auto shader = psShader::CreateShader(0, 0, 2, &SHADER_INFO(shfile.c_str(), (const char*)"vs_main", VERTEX_SHADER_4_0),
    &SHADER_INFO(shfile.c_str(), "ps_main", PIXEL_SHADER_4_0));
  auto timer = HighPrecisionTimer::OpenProfiler();
  int fps = 0;
  //psTex* pslogo = psTex::Create("../media/pslogo192.png", USAGE_SHADER_RESOURCE, FILTER_BOX, 0, FILTER_NONE, psVeciu(192));
  psTex* pslogo = psTex::Create("../media/pslogo.png", USAGE_SHADER_RESOURCE, FILTER_LINEAR, 0, FILTER_NONE, false);
  if(!pslogo)
    ENDTEST;
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
    driver->DrawLines(obj, psLine(0, 400, 0, 200), 0, 0, 0xFFFFFFFF);
    driver->DrawLines(obj, psLine(400, 0, 500, 0), 0, 0, 0xFFFFFFFF);

    psVec polygon[5] = { { 200, 0 }, { 200, 100 }, { 100, 150 }, { 60, 60 }, { 90, 30 } };
    auto ptest1 = driver->DrawPolygon(driver->library.POLYGON, 0, polygon, 5, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
    auto ptest2 = driver->DrawPolygon(driver->library.POLYGON, 0, polygon, 5, psVec3D(400, 400, 0), 0xFFFFFFFF, 0);
    psVec polygon2[4] = { { 400, 2 }, { 500, 2 }, { 500, 3 }, { 400, 3 } };
    auto ptest3 = driver->DrawPolygon(driver->library.LINE, 0, polygon2, 4, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
    psVec polygon3[4] = { { 2, 200 }, { 2, 400 }, { 3, 400 }, { 3, 200 } };
    auto ptest4 = driver->DrawPolygon(driver->library.LINE, 0, polygon3, 4, VEC3D_ZERO, 0xFFFFFFFF, PSFLAG_FIXED);
    engine->End();
    engine->FlushMessages();
    driver->PopCamera();

    updatefpscount(timer, fps);
  }

  globalcam.SetPivotAbs(psVec(0));
  ENDTEST;
}
