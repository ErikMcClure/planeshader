// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psSolid.h"
#include "psPass.h"

using namespace planeshader;
using namespace bss;

psSolid::psSolid(const psSolid& copy) : psLocatable(copy), psRenderable(copy), _scale(copy._scale), _dim(copy._dim), _realdim(copy._realdim),  _boundingrect(copy._boundingrect) { }
psSolid::psSolid(psSolid&& mov) : psLocatable(std::move(mov)), psRenderable(std::move(mov)), _scale(mov._scale), _dim(mov._dim), _realdim(mov._realdim), _boundingrect(mov._boundingrect) { }

psSolid::psSolid(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, const psVec& scale) :
  psLocatable(position, rotation, pivot), psRenderable(flags, zorder, stateblock, shader, pass), _dim(VEC_ONE), _realdim(VEC_ONE), _scale(VEC_ONE)
{
  psSolid::SetScale(scale);
}

psSolid::~psSolid() { }
void psSolid::Render(const psParent* parent)
{
  psPass* pass = !_pass ? psPass::CurPass : _pass;
  if(pass != 0)
    if(!pass->GetCamera()->Cull(this, parent))
      psRenderable::Render(parent);
}

void psSolid::SetDim(const psVec& dim) { _realdim = dim; _setdim(_scale*_realdim); }
void psSolid::SetScale(const psVec& scale) { _scale = scale; _setdim(_scale*_realdim); }
void psSolid::SetScaleDim(const psVec& dim) { SetScale(dim/_realdim); }

void psSolid::_setdim(const psVec& dim)
{
  if(_dim.x!=0 && _dim.y!=0 && dim.x!=0 && dim.y!=0) // If the dimensions are valid
    pivot*=(dim/_dim); //Adjust center position
  _dim=dim;
}
void psSolid::GetTransform(psMatrix& m, const psParent* parent)
{
  psLocatable::GetTransform(m, parent);
  sseVec(m[0])*sseVec(_scale.x) >> m[0];
  sseVec(m[1])*sseVec(_scale.y) >> m[1];
}

psSolid& psSolid::operator =(const psSolid& right)
{
  psLocatable::operator =(right);
  psRenderable::operator =(right);
  _dim=right._dim;
  _scale=right._scale;
  _realdim=right._realdim;
  return *this; 
}
psSolid& psSolid::operator =(psSolid&& right)
{
  psLocatable::operator =(std::move(right));
  psRenderable::operator =(std::move(right));
  _dim=right._dim;
  _scale=right._scale;
  _realdim=right._realdim;
  return *this;
}