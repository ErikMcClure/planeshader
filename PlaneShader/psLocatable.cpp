// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psLocatable.h"
#include "bss-util/delegate.h"

using namespace planeshader;

psLocatable::psLocatable(const psLocatable& copy) : _relpos(copy._relpos), _rotation(copy._rotation), _pivot(copy._pivot) {}
psLocatable::psLocatable(const psVec3D& pos, FNUM rotation, const psVec& pivot) : _relpos(pos), _rotation(rotation), _pivot(pivot) {}
psLocatable::~psLocatable() {}
void psLocatable::SetRotation(FNUM rotation) { _rotation=rotation; }
void psLocatable::SetPivot(const psVec& pivot) { _pivot=pivot; }
void psLocatable::SetPosition(FNUM X, FNUM Y, FNUM Z) { _relpos.x=X; _relpos.y=Y; _relpos.z=Z; }
void psLocatable::GetTransform(psMatrix& m)
{
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(_relpos.x - _pivot.x, _relpos.y - _pivot.y, _relpos.z, _rotation, _pivot.x, _pivot.y, m);
}
