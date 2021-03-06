// Copyright �2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psCamera.h"
#include "bss-util/profiler.h"
#include "psSolid.h"
#include "psTex.h"

using namespace planeshader;
using namespace bss;

psCamera psCamera::default_camera(psVec3D(0, 0, -1.0f), 0.0f, VEC_ZERO, psVec(1.0f, 50000.0f)); // we must manually set the extent because the default_extent constructor is not gauranteed to have been called.
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

psCamera::psCamera(const psCamera& copy) : psLocatable(copy), _viewport(copy._viewport), _extent(copy._extent)
{
  Grab();
}
psCamera::psCamera(const psVec3D& position, FNUM rotation, const psVec& pivot, const psVec& extent) : psLocatable(position, rotation, VEC_ZERO), _viewport(RECT_UNITRECT), _extent(extent)
{
  Grab();
}
psCamera::~psCamera() {}

// Gets the absolute mouse coordinates with respect to this camera.
psVec psCamera::GetMouseAbsolute(const psVeciu& rawdim) const
{
  // We have to adjust the mouse coordinates for the vanishing point in the center of the screen - this adjustment has nothing to do with the camera pivot or the pivot shift.
  psVec dim = _viewport.bottomright*rawdim;
  Vector<float, 4> p(psEngine::Instance()->GetMouse().x - dim.x, psEngine::Instance()->GetMouse().y - dim.y, 0, 1);

  BSS_ALIGN(16) Matrix<float, 4, 4> cam;
  Matrix<float, 4, 4>::AffineTransform_T(position.x - (pivot.x*dim.x), position.y - (pivot.y*dim.y), position.z, rotation, pivot.x, pivot.y, cam.v);
  p = p*cam.Inverse();
  return psVec(p.x*p.z + dim.x, p.y*p.z + dim.y);
}

// Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
const psRectRotate psCamera::GetScreenRect(psFlag flags) const
{
  return psRectRotate(0, 0, 0, 0, 0);
}
void psCamera::SetViewPortAbs(const psRect& vp, const psVeciu& dim)
{
  PROFILE_FUNC();
  _viewport.topleft = vp.topleft/dim;
  _viewport.bottomright = (vp.bottomright-vp.topleft)/dim;
}

inline void psCamera::Apply(const psVeciu& dim, Culling& cache) const
{
  auto& vp = GetViewPort();
  psRectiu realvp = { (uint32_t)bss::fFastRound(vp.left*dim.x), (uint32_t)bss::fFastRound(vp.top*dim.y), (uint32_t)bss::fFastRound(vp.right*dim.x), (uint32_t)bss::fFastRound(vp.bottom*dim.y) };
  psVec pivot = GetPivot()*psVec(dim);
  _driver->SetCamera(position, pivot, GetRotation(), realvp, GetExtent());
  psVec pos = position.xy - pivot;
  cache.full = psRectRotateZ(realvp.left + pos.x, realvp.top + pos.y, realvp.right + pos.x, realvp.bottom + pos.y, GetRotation(), pivot, position.z);
  cache.window = cache.full.BuildAABB();
  cache.winfixed = realvp;
  cache.lastfixed = 0;
  cache.last = 0;
  cache.z = position.z;
  cache.SetSSE();
  //_driver->DrawRect(_driver->library.IMAGE0, 0, psRectRotate(_cache.window, 0, VEC_ZERO), 0, 0, 0x88FFFFFF, 0);
}
inline psRectRotateZ psCamera::Culling::Resolve(const psRectRotateZ& rect) const
{
  return rect.RelativeTo(psVec3D(full.left, full.top, full.z), full.rotation, full.pivot);
}
inline psTransform2D psCamera::Culling::Resolve(const psTransform2D& rect) const
{
  return psTransform2D{ { full.left, full.top, full.z }, full.rotation, full.pivot }.Push(rect.position, rect.rotation, rect.pivot);
}

inline void psCamera::Culling::SetSSE()
{
  SSEwindow = sseVec(window.ltrb);
  SSEwindow_center = sseVec((SSEwindow + sseVec::Shuffle<0x4E>(SSEwindow))*sseVec(0.5f));
  SSEwindow_hold = sseVec(SSEwindow - SSEwindow_center);

  SSEfixed = sseVec(winfixed.ltrb);
  SSEfixed_center = sseVec((SSEfixed + sseVec::Shuffle<0x4E>(SSEfixed))*sseVec(0.5f));
  SSEfixed_hold = sseVec(SSEfixed - SSEfixed_center);
}
bool psCamera::Culling::Cull(const psRect& rect, float rectz, float camz, psFlag flags) const
{
  if(flags&PSFLAG_FIXED) // This is the fixed case
  {
    rectz += 1.0f;
    r_adjust(SSEfixed, SSEfixed_hold, SSEfixed_center, lastfixed, rectz);
    BSS_ALIGN(16) psRect rfixed(SSEfixed);
    return !rect.RectCollide(rfixed);
  }

  rectz -= camz;
  r_adjust(SSEwindow, SSEwindow_hold, SSEwindow_center, last, rectz);
  BSS_ALIGN(16) psRect rfixed(SSEwindow);
  return !rect.RectCollide(rfixed);
}


psCamera::Culling::Culling(const Culling& copy) : SSEwindow(copy.SSEwindow), SSEwindow_center(copy.SSEwindow_center), SSEwindow_hold(copy.SSEwindow_hold),
  SSEfixed(copy.SSEfixed), SSEfixed_center(copy.SSEfixed_center), SSEfixed_hold(copy.SSEfixed_hold), last(copy.last), lastfixed(copy.lastfixed),
  window(copy.window), winfixed(copy.winfixed), z(copy.z)
{}
psCamera::Culling::Culling() : SSEwindow(0), SSEwindow_center(0), SSEwindow_hold(0), SSEfixed(0), SSEfixed_center(0), SSEfixed_hold(0), last(0), lastfixed(0), z(0.0)
{}
