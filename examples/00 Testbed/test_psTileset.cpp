// Copyright ©2017 Black Sphere Studios

#include "testbed.h"
#include "psTex.h"
#include "psLayer.h"
#include "psTileset.h"

using namespace bss;
using namespace planeshader;

/*
int figure_out_wang(int p) // brute force wang
{
switch(p)
{
case 0b0001: return 0;
case 0b0011: return 1;
case 0b1011: return 2;
case 0b1001: return 3;
case 0b0101: return 4;
case 0b0111: return 5;
case 0b1111: return 6;
case 0b1101: return 7;
case 0b0100: return 8;
case 0b0110: return 9;
case 0b1110: return 10;
case 0b1100: return 11;
case 0b0000: return 12;
case 0b0010: return 13;
case 0b1010: return 14;
case 0b1000: return 15;
}
assert(false);
return 0;
}*/

int wang_indices(int l, int t, int r, int b) // wang using bitwise operations
{
  int p = l << 3 | t << 2 | r << 1 | b << 0;
  int _8 = (!(p & 1) << 3);
  int _4 = ((p & 4) ^ (((~p) & 1) << 2));
  int _2 = ((p & 8) >> 2);
  int _1 = (((p & 2) >> 1) ^ ((p & 8) >> 3));
  return _8 | _4 | _2 | _1;
}

TESTDEF::RETPAIR test_psTileset()
{
  BEGINTEST;
  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  const int dimx = 500;
  const int dimy = 500;
  char edges[dimy + 1][dimx + 1][2];
  for(char* i = edges[0][0]; i - edges[0][0] < sizeof(edges) / sizeof(char); ++i)
    *i = RANDBOOLGEN();

  std::unique_ptr<psTile[]> map(new psTile[dimx * dimy]);
  memset(map.get(), 0, sizeof(psTile) * dimx * dimy);
  for(int j = 0; j < dimy; ++j)
    for(int i = 0; i < dimx; ++i)
    {
      map[j * dimx + i].color = ~0;
      map[j * dimx + i].index = wang_indices(edges[j][i][0], edges[j][i][1], edges[j][i + 1][0], edges[j + 1][i][1]);
      //psVeciu pos = psTileset::WangTile2D(edges[j][i][0], edges[j][i][1], edges[j][i + 1][0], edges[j + 1][i][1]);
      //map[j][i].index = pos.x + (pos.y * 4);
    }

  psTileset tiles(VEC3D_ZERO, 0, VEC_ZERO, PSFLAG_DONOTCULL, 0, 0, driver->library.IMAGE, engine->GetLayer(0));
  tiles.SetTexture(psTex::Create("../media/wang2.png", 64U, FILTER_TRIANGLE, 0, FILTER_NONE, false, STATEBLOCK_LIBRARY::POINTSAMPLE));
  if(!tiles.AutoGenDefs(psVeciu(8, 8)))
    ENDTEST;
  tiles.SetTiles(map.get(), dimx*dimy, dimx);

  psTileset tiles2(psVec3D(0, 0, 1), 0, VEC_ZERO, PSFLAG_DONOTCULL, 0, 0, driver->library.IMAGE, engine->GetLayer(0));
  tiles2.SetTexture(psTex::Create("../media/wang2.png", 64U, FILTER_TRIANGLE, 0, FILTER_NONE, false, STATEBLOCK_LIBRARY::POINTSAMPLE));
  if(!tiles2.AutoGenDefs(psVeciu(8, 8)))
    ENDTEST;
  tiles2.SetTiles(map.get(), dimx*dimy, dimx);

  engine->GetLayer(0)->SetClearColor(0);
  engine->GetLayer(0)->SetCamera(&globalcam);

  while(!gotonext && engine->Begin())
  {
    updatefpscount(timer, fps);
    processGUI();
    tiles.Render(0);
    tiles2.Render(0);
    engine->End();
    engine->FlushMessages();
  }

  ENDTEST;
}