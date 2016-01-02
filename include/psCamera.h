// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __CAMERA_H__PS__
#define __CAMERA_H__PS__

#include "psLocatable.h"
#include "psRect.h"

namespace planeshader {
  class PS_DLLEXPORT psCamera : public psLocatable
  {
  public:
    // Constructors
    psCamera(const psCamera& copy);
    explicit psCamera(const psVec3D& position = psVec3D(0, 0, -1.0f), FNUM rotation = 0.0f, const psVec& pivot = VEC_ZERO);
    ~psCamera();
    // Gets the absolute mouse coordinates with respect to this camera.
    psVec GetMouseAbsolute() const;
    // Gets a rect representing the visible area of this camera in absolute coordinates given the provided flags.
    const psRectRotate BSS_FASTCALL GetScreenRect(psFlag flags=0) const;
    // Gets or sets the viewport of the camera
    const psRect& GetViewPort() const { return _viewport; }
    void BSS_FASTCALL SetViewPort(const psRect& vp); // The viewport must be in pixels, but fractional values are technically valid here if rendering at a strange DPI
    void BSS_FASTCALL SetPivotAbs(const psVec& pivot);

    inline psCamera& operator =(const psCamera& copy) { psLocatable::operator =(copy); return *this; }

    static const psVeci INVALID_LASTRELMOUSE;
    static const psCamera default_camera;

  protected:
    psRect _viewport; // This is NOT an actual rectangle, it stores left/top/width/height
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

#endif