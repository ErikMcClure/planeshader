// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psChart.h"

using namespace planeshader;

psChartContainer::psChartContainer(psTexFont* font) : _view(0,0,0,0), _font(font)
{
}

size_t psChartContainer::AddChart(psChart* chart) { return _captions.AddConstruct(chart); }
psChart* psChartContainer::GetChart(size_t index) { return _captions[index].get(); }
bool psChartContainer::RemoveChart(size_t index) { if(index >= _captions.Length()) return false; _captions.Remove(index); return true; }

void psChartContainer::_render(const psParent& parent)
{
  psMatrix m;
  parent.Push(_relpos, _rotation, _pivot).GetTransform(m);
  _driver->PushTransform(m);
  psBatchObj* batch = _driver->DrawLinesStart(_driver->library.LINE, 0, GetFlags());
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

  _driver->PopTransform();
}
void psChartContainer::SetDim(const psVec& dim)
{
  //buffer dim with the size of the axis and title, etc.
  psSolid::SetDim(dim);
}
