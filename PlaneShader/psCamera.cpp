// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCamera.h"
#include "psEngine.h"
#include "bss-util/profiler.h"

using namespace planeshader;

const psCamera psCamera::default_camera;

psCamera::psCamera(const psCamera& copy) : psLocatable(copy), _viewport(copy._viewport)
{

}
psCamera::psCamera(const psVec3D& position, FNUM rotation, const psVec& pivot) : psLocatable(position, rotation, pivot), _viewport(RECT_UNITRECT)
{

}
psCamera::~psCamera() {}
// Gets the absolute mouse coordinates with respect to this camera.
const psVec& psCamera::GetMouseAbsolute() const
{
  return VEC_ZERO;
}
psVec psCamera::TransformPoint(const psVec3D& point, FLAG_TYPE flags) const
{
  return point.xy;

}
psVec psCamera::ReversePoint(const psVec3D& point, FLAG_TYPE flags) const
{
  return point.xy;

}
// Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
const psRectRotate psCamera::GetScreenRect(FLAG_TYPE flags) const
{
  return psRectRotate(0, 0, 0, 0, 0, VEC_ZERO);
}
void psCamera::SetViewPort(const psRectiu& vp)
{
  PROFILE_FUNC();
  psVec dim = psEngine::Instance()->GetDriver()->screendim;
  _viewport.topleft = vp.topleft/dim;
  _viewport.bottomright = (vp.bottomright-vp.topleft)/dim;
}