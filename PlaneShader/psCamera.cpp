// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCamera.h"
#include "psEngine.h"
#include "bss-util/profiler.h"
#include "psSolid.h"
#include "psTex.h"

using namespace planeshader;
using namespace bss;

const psCamera psCamera::default_camera(psVec3D(0, 0, -1.0f), 0.0f, VEC_ZERO, psVec(1.0f, 50000.0f)); // we must manually set the extent because the default_extent constructor is not gauranteed to have been called.
psVec psCamera::default_extent(1.0f, 50000.0f);

BSS_FORCEINLINE void r_adjust(sseVec& window, const sseVec& winhold, const sseVec& center, float& last, float adjust)
{
  if(last != adjust)
  {
    last = adjust;
    window = winhold;
    window *= adjust;
    window += center;
  }
}

psCamera::psCamera(const psCamera& copy) : psLocatable(copy), _viewport(copy._viewport), _extent(copy._extent), _cache(copy._cache)
{
}
psCamera::psCamera(const psVec3D& position, FNUM rotation, const psVec& pivot, const psVec& extent) : psLocatable(position, rotation, VEC_ZERO), _viewport(RECT_UNITRECT), _extent(extent)
{
}
psCamera::~psCamera() {}

// Gets the absolute mouse coordinates with respect to this camera.
psVec psCamera::GetMouseAbsolute(const psTex* rt) const
{
  if(!rt) rt = _driver->GetBackBuffer();

  // We have to adjust the mouse coordinates for the vanishing point in the center of the screen - this adjustment has nothing to do with the camera pivot or the pivot shift.
  psVec dim = _viewport.bottomright*rt->GetRawDim();
  Vector<float, 4> p(psEngine::Instance()->GetMouse().x - dim.x, psEngine::Instance()->GetMouse().y - dim.y, 0, 1);

  BSS_ALIGN(16) Matrix<float, 4, 4> cam;
  Matrix<float, 4, 4>::AffineTransform_T(position.x - (pivot.x*dim.x), position.y - (pivot.y*dim.y), position.z, rotation, pivot.x, pivot.y, cam.v);
  p = p*cam.Inverse();
  return psVec(p.x*p.z + dim.x, p.y*p.z + dim.y);
}
void psCamera::SetPivotAbs(const psVec& pivot, const psTex* rt)
{
  if(!rt) rt = _driver->GetBackBuffer();
  SetPivot((pivot / rt->GetDim()) * _viewport.GetDimensions());
}

// Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
const psRectRotate psCamera::GetScreenRect(psFlag flags) const
{
  return psRectRotate(0, 0, 0, 0, 0);
}
void psCamera::SetViewPortAbs(const psRect& vp, const psTex* rt)
{
  PROFILE_FUNC();
  if(!rt) rt = _driver->GetBackBuffer();
  const psVec& dim = rt->GetDim();
  _viewport.topleft = vp.topleft/dim;
  _viewport.bottomright = (vp.bottomright-vp.topleft)/dim;
}
inline const psRect& psCamera::Apply(const psTex* rt) const
{
  psVeciu dim = rt->GetRawDim();
  auto& vp = GetViewPort();
  psRectiu realvp = { (uint32_t)bss::fFastRound(vp.left*dim.x), (uint32_t)bss::fFastRound(vp.top*dim.y), (uint32_t)bss::fFastRound(vp.right*dim.x), (uint32_t)bss::fFastRound(vp.bottom*dim.y) };
  psVec pivot = GetPivot()*psVec(dim);
  _driver->PushCamera(position, pivot, GetRotation(), realvp, GetExtent());
  psVec pos = position.xy - pivot;
  _cache.full = psRectRotateZ(realvp.left + pos.x, realvp.top + pos.y, realvp.right + pos.x, realvp.bottom + pos.y, GetRotation(), pivot, position.z);
  _cache.window = _cache.full.BuildAABB();
  _cache.winfixed = realvp;
  _cache.lastfixed = 0;
  _cache.last = 0;
  _cache.SetSSE();
  //_driver->DrawRect(_driver->library.IMAGE0, 0, psRectRotate(_cache.window, 0, VEC_ZERO), 0, 0, 0x88FFFFFF, 0);
  return _cache.window;
}
inline bool psCamera::Cull(psSolid* solid, const psTransform2D* parent) const
{
  if((solid->GetFlags()&PSFLAG_DONOTCULL) != 0) return false; // Don't cull if it has a DONOTCULL flag
  return _cache.Cull(solid->GetBoundingRect((!parent)?(psTransform2D::Zero):(*parent)), solid->GetPosition().z + (!parent ? 0 : parent->position.z), position.z, solid->GetFlags());
}
inline bool psCamera::Cull(const psRectRotateZ& rect, const psTransform2D* parent, psFlag flags) const
{
  if((flags&PSFLAG_DONOTCULL) != 0) return false;
  return _cache.Cull(rect.BuildAABB(), rect.z, position.z, flags);
}
inline psRectRotateZ psCamera::Resolve(const psRectRotateZ& rect) const
{
  return rect.RelativeTo(psVec3D(_cache.full.left, _cache.full.top, _cache.full.z), _cache.full.rotation, _cache.full.pivot);
}
inline psTransform2D psCamera::Resolve(const psTransform2D& rect) const
{
  return psTransform2D{ { _cache.full.left, _cache.full.top, _cache.full.z }, _cache.full.rotation, _cache.full.pivot }.Push(rect.position, rect.rotation, rect.pivot);
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
bool psCamera::CamCache::Cull(const psRect& rect, float rectz, float camz, psFlag flags)
{
  if(flags&PSFLAG_FIXED) // This is the fixed case
  {
    rectz += 1.0f;
    r_adjust(SSEfixed, SSEfixed_hold, SSEfixed_center, lastfixed, rectz);
    BSS_ALIGN(16) psRect rfixed(SSEfixed);
    return !rect.IntersectRect(rfixed);
  }

  rectz -= camz;
  r_adjust(SSEwindow, SSEwindow_hold, SSEwindow_center, last, rectz);
  BSS_ALIGN(16) psRect rfixed(SSEwindow);
  return !rect.IntersectRect(rfixed);
}


psCamera::CamCache::CamCache(const CamCache& copy) : SSEwindow(copy.SSEwindow), SSEwindow_center(copy.SSEwindow_center), SSEwindow_hold(copy.SSEwindow_hold),
  SSEfixed(copy.SSEfixed), SSEfixed_center(copy.SSEfixed_center), SSEfixed_hold(copy.SSEfixed_hold), last(copy.last), lastfixed(copy.lastfixed),
  window(copy.window), winfixed(copy.winfixed)
{}
psCamera::CamCache::CamCache() : SSEwindow(0), SSEwindow_center(0), SSEwindow_hold(0), SSEfixed(0), SSEfixed_center(0), SSEfixed_hold(0), last(0), lastfixed(0)
{}
