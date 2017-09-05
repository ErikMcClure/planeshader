// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __CAMERA_H__PS__
#define __CAMERA_H__PS__

#include "psLocatable.h"
#include "psRect.h"
#include "psDriver.h"
#include "bss-util/RefCounter.h"

#pragma warning(push)
#pragma warning(disable:4251)

namespace planeshader {
  // Represents a camera and viewport, but must be applied relative to some texture's dimensions
  class PS_DLLEXPORT psCamera : public psLocatable, public bss::RefCounter, public psDriverHold // The reference counter is optional (set to 1 on initial construction)
  {
    typedef bss::sseVec sseVec;
  public:
    // Constructors
    psCamera(const psCamera& copy);
    explicit psCamera(const psVec3D& position = psVec3D(0, 0, -1.0f), FNUM rotation = 0.0f, const psVec& pivot = VEC_ZERO, const psVec& extent = default_extent);
    ~psCamera();
    // Gets the absolute mouse coordinates with respect to this camera.
    psVec GetMouseAbsolute(const psVeciu& rawdim) const;
    // Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
    const psRectRotate GetScreenRect(psFlag flags=0) const;
    // Gets or sets the viewport of the camera
    const psRect& GetViewPort() const { return _viewport; }
    inline void SetViewPort(const psRect& vp) { _viewport = vp; }
    void SetViewPortAbs(const psRect& vp, const psVeciu& dim); // The viewport must be in pixels, but fractional values are technically valid here if rendering at a strange DPI
    inline void SetPivotAbs(const psVec& pivot, const psVeciu& dim) { SetPivot((pivot / dim) * _viewport.Dim()); }
    inline const psVec& GetExtent() const { return _extent; }
    inline void SetExtent(float znear, float zfar) { _extent.x = znear; _extent.y = zfar; }
    inline void SetExtent(const psVec& extent) { _extent = extent; }
    inline psCamera& operator =(const psCamera& copy) 
    { 
      psLocatable::operator =(copy); 
      _viewport = copy._viewport; 
      _extent = copy._extent; 
      return *this;
    }

    static const psVeci INVALID_LASTRELMOUSE;
    static const psCamera default_camera;
    static psVec default_extent;

    struct PS_DLLEXPORT Culling {
      BSS_ALIGN(16) psRect window;
      BSS_ALIGN(16) psRect winfixed;
      mutable sseVec SSEwindow;
      sseVec SSEwindow_center;
      sseVec SSEwindow_hold;
      mutable sseVec SSEfixed;
      sseVec SSEfixed_center;
      sseVec SSEfixed_hold;
      mutable float last;
      mutable float lastfixed;
      float z;
      psRectRotateZ full;

      Culling(const Culling&);
      Culling();
      inline void SetSSE();
      BSS_FORCEINLINE bool Cull(const psRectRotateZ& rect, const psTransform2D* parent, psFlag flags) const
      {
        if((flags&PSFLAG_DONOTCULL) != 0)
          return false;
        return Cull(rect.BuildAABB(), rect.z, z, flags);
      }

      inline psRectRotateZ Resolve(const psRectRotateZ& rect) const;
      inline psTransform2D Resolve(const psTransform2D& rect) const;
      bool Cull(const psRect& rect, float rectz, float camz, psFlag flags) const;
    };

    inline void Apply(const psVeciu& rawdim, Culling& culling) const;

  protected:
    psRect _viewport; // This is NOT an actual rectangle, it stores left/top/width/height
    psVec _extent;
  };

  /*class PS_DLLEXPORT psCamera3D : public psCamera
  {
  public:
    // Constructors
    psCamera(const psCamera& copy);
    explicit psCamera(const psVec3D& position=VEC3D_ZERO, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO);
    ~psCamera();

    inline psCamera& operator =(const psCamera& copy) { psLocatable::operator =(copy); return *this; }
    
  protected:
    psRect _viewport;
  };*/
}
#pragma warning(pop)

#endif