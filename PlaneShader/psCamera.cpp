// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCamera.h"
#include "psEngine.h"
#include "bss-util/profiler.h"
#include "psSolid.h"

using namespace planeshader;
using namespace bss_util;

const psCamera psCamera::default_camera(psVec3D(0, 0, -1.0f), 0.0f, VEC_ZERO, psVec(1.0f, 50000.0f)); // we must manually set the extent because the default_extent constructor is not gauranteed to have been called.
psVec psCamera::default_extent(1.0f, 50000.0f);

BSS_FORCEINLINE void r_adjust(sseVec& window, sseVec& winhold, sseVec& center, float& last, float adjust)
{
  if(last != adjust)
  {
    last = adjust;
    window = winhold;
    window *= adjust;
    window += center;
  }
}

float BSS_FASTCALL r_gettotalz(psInheritable* p)
{
  if(p->GetParent())
    return p->GetPosition().z + r_gettotalz(p->GetParent());
  return p->GetPosition().z;
}

psCamera::psCamera(const psCamera& copy) : psLocatable(copy), _viewport(copy._viewport), _extent(copy._extent), _cache(copy._cache)
{
}
psCamera::psCamera(const psVec3D& position, FNUM rotation, const psVec& pivot, const psVec& extent) : psLocatable(position, rotation, VEC_ZERO), _viewport(RECT_UNITRECT), _extent(extent)
{
}
psCamera::~psCamera() {}

// Gets the absolute mouse coordinates with respect to this camera.
psVec psCamera::GetMouseAbsolute() const
{
  // We have to adjust the mouse coordinates for the vanishing point in the center of the screen - this adjustment has nothing to do with the camera pivot or the pivot shift.
  psVec dim = _viewport.bottomright*_driver->rawscreendim;
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
  SetPivot((pivot / _driver->screendim) * _viewport.bottomright);
}

// Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
const psRectRotate psCamera::GetScreenRect(psFlag flags) const
{
  return psRectRotate(0, 0, 0, 0, 0);
}
void psCamera::SetViewPort(const psRect& vp)
{
  PROFILE_FUNC();
  const psVec& dim = _driver->screendim;
  _viewport.topleft = vp.topleft/dim;
  _viewport.bottomright = (vp.bottomright-vp.topleft)/dim;
}
inline const psRect& psCamera::Apply() const
{
  auto& vp = GetViewPort();
  psRectiu realvp = { (unsigned int)bss_util::fFastRound(vp.left*_driver->rawscreendim.x), (unsigned int)bss_util::fFastRound(vp.top*_driver->rawscreendim.y), (unsigned int)bss_util::fFastRound(vp.right*_driver->rawscreendim.x), (unsigned int)bss_util::fFastRound(vp.bottom*_driver->rawscreendim.y) };
  _driver->PushCamera(_relpos, GetPivot()*psVec(_driver->rawscreendim), GetRotation(), realvp, GetExtent());
  _cache.window = psRectRotate(realvp.left + _relpos.x, realvp.top + _relpos.y, realvp.right + _relpos.x, realvp.bottom + _relpos.y, GetRotation(), GetPivot()).BuildAABB();
  _cache.winfixed = realvp;
  _cache.SetSSE();
  return _cache.window;
}
inline bool psCamera::Cull(psSolid* solid) const
{
  return _cache.Cull(solid, _relpos.z);
}

inline void psCamera::CamCache::SetSSE()
{
  SSEwindow = sseVec(window._ltrbarray);
  SSEwindow_center = sseVec((SSEwindow + sseVec::Shuffle<0x4E>(SSEwindow))*sseVec(0.5f));
  SSEwindow_hold = sseVec(SSEwindow - SSEwindow_center);

  SSEfixed = sseVec(winfixed._ltrbarray);
  SSEfixed_center = sseVec((SSEfixed + sseVec::Shuffle<0x4E>(SSEfixed))*sseVec(0.5f));
  SSEfixed_hold = sseVec(SSEfixed - SSEfixed_center);
}
inline bool BSS_FASTCALL psCamera::CamCache::Cull(psSolid* solid, float z)
{
  psFlag flags = solid->GetAllFlags();
  if((flags&PSFLAG_DONOTCULL) != 0) return false; // Don't cull if it has a DONOTCULL flag

  const psRect& rect = solid->GetBoundingRect(); // Recalculates the bounding rect
  float adjust = r_gettotalz(solid);

  if(flags&PSFLAG_FIXED) // This is the fixed case
  {
    adjust += 1.0f;
    r_adjust(SSEfixed, SSEfixed_hold, SSEfixed_center, lastfixed, adjust);
    BSS_ALIGN(16) psRect rfixed(SSEfixed);
    if(rect.IntersectRect(rfixed)) return false;
  }
  else { // This is the default case
    adjust -= z;
    r_adjust(SSEwindow, SSEwindow_hold, SSEwindow_center, last, adjust);
    BSS_ALIGN(16) psRect rfixed(SSEwindow);
    if(rect.IntersectRect(rfixed)) return false;
  }
  return true;
}

psCamera::CamCache::CamCache(const CamCache& copy) : SSEwindow(copy.SSEwindow), SSEwindow_center(copy.SSEwindow_center), SSEwindow_hold(copy.SSEwindow_hold),
  SSEfixed(copy.SSEfixed), SSEfixed_center(copy.SSEfixed_center), SSEfixed_hold(copy.SSEfixed_hold), last(copy.last), lastfixed(copy.lastfixed),
  window(copy.window), winfixed(copy.winfixed)
{}
psCamera::CamCache::CamCache() : SSEwindow(0), SSEwindow_center(0), SSEwindow_hold(0), SSEfixed(0), SSEfixed_center(0), SSEfixed_hold(0), last(0), lastfixed(0)
{}
