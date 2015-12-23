// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCamera.h"
#include "psEngine.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss_util;

const psCamera psCamera::default_camera;

psCamera::psCamera(const psCamera& copy) : psLocatable(copy), _viewport(copy._viewport)
{

}
psCamera::psCamera(const psVec3D& position, FNUM rotation, const psVec& pivot) : psLocatable(position, rotation, VEC_ZERO), _viewport(RECT_UNITRECT)
{

}
psCamera::~psCamera() {}

// Gets the absolute mouse coordinates with respect to this camera.
psVec psCamera::GetMouseAbsolute() const
{
  // We have to adjust the mouse coordinates for the vanishing point in the center of the screen - this adjustment has nothing to do with the camera pivot or the pivot shift.
  psVec dim = _viewport.bottomright*psEngine::Instance()->GetDriver()->rawscreendim;
  Vector<float, 4> p(psEngine::Instance()->GetMouse().x - dim.x, psEngine::Instance()->GetMouse().y - dim.y, 0, 1);

  BSS_ALIGN(16) Matrix<float, 4, 4> cam;
  Matrix<float, 4, 4>::AffineTransform_T(_relpos.x - (_pivot.x*dim.x), _relpos.y - (_pivot.y*dim.y), _relpos.z, _rotation, _pivot.x, _pivot.y, cam.v);
  p = p*cam.Inverse();
  return psVec(p.x*p.z + dim.x, p.y*p.z + dim.y);
  /*
  float z = 1.0f - _relpos.z;
  BSS_ALIGN(16) Matrix<float, 4, 4> m;
  Matrix<float, 4, 4>::Translation_T(dim.x*-0.5f, dim.y*-0.5f, 0, m.v);

  p = p*m*cam;
  return (p.xy*z) + (dim*0.5f) + _relpos.xy;*/
}
void BSS_FASTCALL psCamera::SetPivotAbs(const psVec& pivot)
{
  SetPivot((pivot / psEngine::Instance()->GetDriver()->screendim) * _viewport.bottomright);
}

// Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
const psRectRotate psCamera::GetScreenRect(psFlag flags) const
{
  return psRectRotate(0, 0, 0, 0, 0);
}
void psCamera::SetViewPort(const psRect& vp)
{
  PROFILE_FUNC();
  const psVec& dim = psEngine::Instance()->GetDriver()->screendim;
  _viewport.topleft = vp.topleft/dim;
  _viewport.bottomright = (vp.bottomright-vp.topleft)/dim;
}