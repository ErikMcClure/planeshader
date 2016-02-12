// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psChart.h"

using namespace planeshader;

psChartContainer::psChartContainer(psTexFont* font) : _view(0,0,0,0), _font(font)
{
}

size_t psChartContainer::AddChart(const psChart* chart) { _captions.AddConstruct(chart); }
psChart* psChartContainer::GetChart(size_t index) { return _captions[index]; }
bool psChartContainer::RemoveChart(size_t index) { _captions.Remove(index); }

void BSS_FASTCALL psChartContainer::_render(psBatchObj* obj)
{
  assert(!obj);
  psFlag flags = GetAllFlags();
  psBatchObj batch;
  bss_util::Matrix<float, 4, 4> m;
  GetTransform(m);
  _driver->DrawLinesStart(batch, flags, m);
  _driver->DrawLines(batch, psLine(0, 0, _dim.x, 0), 0, 0, 0xFF999999);
  _driver->DrawLines(batch, psLine(0, 0, 0, _dim.y), 0, 0, 0xFF999999);
  _driver->DrawLines(batch, psLine(0, _dim.y, _dim.x, _dim.y), 0, 0, 0xAA999999);
  //_driver->DrawLines(batch, psLine(_dim.x, 0, _dim.x, _dim.y), 0, 0, 0xAA999999);

  // Title

  // axis labels

  // always draw max and min labels

  // Figure out how many intermediate labels we need
  float lineheight = 0;
  int num = _dim.y / lineheight*2.5;

  for(int i = 0; i < num; ++i)
  {
    _driver->DrawLines(batch, psLine(0, lineheight*2.5*i, _dim.x, lineheight*2.5*i), 0, 0, 0xFF999999);

  }
}
void psChartContainer::SetDim(const psVec& dim)
{
  //buffer dim with the size of the axis and title, etc.
  psSolid::SetDim(dim);
}
