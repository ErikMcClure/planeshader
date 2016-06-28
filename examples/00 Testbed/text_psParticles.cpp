// Copyright ©2016 Black Sphere Studios

#include "testbed.h"
#include "psTex.h"
#include "psPass.h"

using namespace bss_util;
using namespace planeshader;

TESTDEF::RETPAIR test_psParticles()
{
  BEGINTEST;
  auto timer = cHighPrecisionTimer::OpenProfiler();
  int fps = 0;

  psTex* particle = psTex::Create("../media/particle.png");
  psVertex verts[5000];
  psVec velocities[5000];
  memset(velocities, 0, sizeof(psVec) * 5000);
  psDriver* driver = engine->GetDriver();

  for(int i = 0; i < 5000; ++i)
  {
    verts[i].x = bssrandreal(0, driver->GetBackBuffer()->GetDim().x);
    verts[i].y = bssrandreal(0, driver->GetBackBuffer()->GetDim().y);
    verts[i].z = 0;
    verts[i].w = bssrandreal(8, 32);
    verts[i].color = 0x88FFFFFF;
  }

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0);
    driver->PushCamera(globalcam.GetPosition(), globalcam.GetPivot(), globalcam.GetRotation(), psRectiu(VEC_ZERO, driver->GetBackBuffer()->GetRawDim()), psCamera::default_extent);

    for(int i = 0; i < 5000; ++i)
    {
      verts[i].x += velocities[i].x;
      verts[i].y += velocities[i].y;
      double s = dist(verts[i].x, verts[i].y, driver->GetBackBuffer()->GetDim().x / 2, driver->GetBackBuffer()->GetDim().y / 2) / 2000.0; // This gets the distance from the center, scaling the outer boundary at 2000 to 1.0
      double d = bssrandreal(-1.0, 1.0);
      d = std::pow(abs(d), 1.0 + s*s) * ((d < 0.0) ? -1.0 : 1.0); // This causes the distribution of d to tend towards zero at a higher rate the farther away a particle is from the center. This allows particles to move freely near the center, but get dragged towards it farther away.
      velocities[i] += psVec::FromPolar(0.004, bssfmod<double>(d*PI - atan2(verts[i].y - driver->GetBackBuffer()->GetDim().y / 2, -verts[i].x + driver->GetBackBuffer()->GetDim().x / 2), PI_DOUBLE)); // At d = 0, the velocity will always point towards the center.
    }

    driver->SetTextures(&particle, 1);
    driver->DrawPoints(driver->library.PARTICLE, STATEBLOCK_LIBRARY::GLOW, verts, 5000, 0);
    engine->End();
    engine->FlushMessages();
    updatefpscount(timer, fps);
  }

  ENDTEST;
}