// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in testbed.cpp

#include "testbed.h"
#include "psRenderGeometry.h"
#include "psLayer.h"
#include "psVector.h"
#include "psImage.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psGeometry()
{
  BEGINTEST;
  psTex* t = psTex::Create("../media/pslogo192.png", USAGE_STAGING);

  HighPrecisionTimer time;
  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  bss::CircleSector<float> segment(300, 100, 10, 50, PI/2, 0.5);

  psRenderCircle arc(50, psVec3D(300, 100, 0));
  arc.SetOutline(segment.outer - segment.inner);
  arc.SetOutlineColor(0x99FFFFFF);
  arc.SetColor(0);

  psRenderCircle arc2(50, psVec3D(300, 100, 0));
  arc2.SetOutline(segment.outer - segment.inner);
  arc2.SetOutlineColor(0x99FFFFFF);
  arc2.SetColor(0);

  psRenderCircle circle(50, psVec3D(200, 350, 0));
  psRenderCircle circle1(1, psVec3D(0, 0, 0));
  psRenderCircle circle2(1, psVec3D(0, 0, 0));
  //psRenderCircle circle3(10, psVec3D(200, 350, 0));
  //circle3.SetColor(0x99FFFFFF);
  psLine line(0, 0, 0, 0);
  circle1.SetColor(psColor32(255, 255, 0, 0));
  circle2.SetColor(psColor32(255, 255, 0, 0));
  circle.SetOutlineColor(0xFFFFFFFF);

  engine->GetLayer(0)->Insert(&arc);
  engine->GetLayer(0)->Insert(&arc2);
  engine->GetLayer(0)->Insert(&circle);
  engine->GetLayer(0)->Insert(&circle1);
  engine->GetLayer(0)->Insert(&circle2);
  //engine->GetLayer(0)->Insert(&circle3);
  engine->GetLayer(0)->SetClearColor(0xFF999999);
  engine->GetLayer(0)->SetCamera(&globalcam);

  while(!gotonext && engine->Begin())
  {
    processGUI();
    psRenderLine::DrawLine(line, 0xFFFFFFFF);
    psVec out[2];
    int n = bss::LineSegmentRadiusIntersect<float>(line.x1 - circle.GetPosition().x, line.y1 - circle.GetPosition().y, line.x2 - circle.GetPosition().x, line.y2 - circle.GetPosition().y, circle.GetDim().x / 2, out);
    line.p2 = globalcam.GetMouseAbsolute(engine->GetLayer(0)->GetTargets()[0]->GetRawDim());
    //circle3.SetPosition(line.p2);
    arc2.SetPosition(line.p2);
    n = bss::CircleRadiusIntersect<float>(segment.outer, line.p2.x - arc.GetPosition().x, line.p2.y - arc.GetPosition().y, segment.outer, out);
    //float X = arc2.GetPosition().x - segment.x;
    //float Y = arc2.GetPosition().y - segment.y;
    //n = psCircleSegment::_lineSegmentRadiusIntersect((cos(segment.min) * segment.inner) - X, - (sin(segment.min) * segment.inner) - Y, (cos(segment.min) * segment.outer) - X, - (sin(segment.min) * segment.outer) - Y, segment.outer, out);
    if(n > 0)
      circle1.SetPosition(out[0] + arc2.GetPosition().xy);
    if(n > 1)
      circle2.SetPosition(out[1] + arc2.GetPosition().xy);

    engine->End();
    time.Update();
    engine->FlushMessages();
    updatefpscount(timer, fps);
    segment.min = (sin(time.GetTime()*0.0005) + 1) * PI;
    segment.range = (sin(time.GetTime()*0.001) + 1) * PI;
    arc.SetArcs(psRect(0, 0, segment.min, segment.range));
    arc2.SetArcs(psRect(0, 0, segment.min, segment.range));
    //arc.SetOutlineColor(segment.ContainsPoint(line.p2.x, line.p2.y) ? 0xFF0000FF : 0xFFFFFFFF);
    //arc.SetOutlineColor(segment.IntersectLine(line.x1, line.y1, line.x2, line.y2) ? 0xFF00FF00 : 0xFFFFFFFF);
    //arc.SetOutlineColor(segment.IntersectCircle(circle3.GetPosition().x, circle3.GetPosition().y, circle3.GetDim().x * 0.5f) ? 0xFFFF0000 : 0xFFFFFFFF);

    arc.SetOutlineColor(segment.CircleSectorCollide(arc2.GetPosition().x, arc2.GetPosition().y, segment.inner, segment.outer, segment.min, segment.range) ? 0xFFFF0000 : 0xFFFFFFFF);
  }

  ENDTEST;
}