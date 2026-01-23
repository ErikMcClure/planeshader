// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __VEC_H__PS__
#define __VEC_H__PS__

#include "ps_dec.h"
#include "buntils/Geometry.h"

namespace planeshader {
  typedef bun::Vector<float, 2> psVec; //default typedef
  typedef bun::Vector<float, 2> psVector;
  typedef bun::Vector<int, 2> psVeci;
  typedef bun::Vector<double, 2> psVecd;
  typedef bun::Vector<uint32_t, 2> psVeciu;

  static const psVec VEC_ZERO(0, 0);
  static const psVec VEC_ONE(1, 1);
  static const psVec VEC_HALF(0.5f, 0.5f);
  static const psVec VEC_NEGHALF(-0.5f, -0.5f);

  typedef bun::Vector<float, 3> psVec3D; //default typedef
  typedef bun::Vector<float, 3> psVector3D;
  typedef bun::Vector<int, 3> psVec3Di;
  typedef bun::Vector<double, 3> psVec3Dd;
  typedef bun::Vector<uint32_t, 3> psVec3Diu;

  static const psVec3D VEC3D_ZERO(0, 0, 0);
  static const psVec3D VEC3D_ONE(1, 1, 1);

  typedef bun::Rect<FNUM> psRect; //Default typedef
  typedef bun::Rect<int> psRecti;
  typedef bun::Rect<double> psRectd;
  typedef bun::Rect<long> psRectl;
  typedef bun::Rect<uint32_t> psRectiu;

  static const psRect RECT_ZERO(0, 0, 0, 0);
  static const psRect RECT_UNITRECT(0, 0, 1, 1);

  typedef bun::Line<float> psLine; //default typedef
  typedef bun::Line<double> psLined;

  typedef bun::Line3d<float> psLine3D; //default typedef
  typedef bun::Line3d<double> psLine3Dd;

  typedef bun::Circle<float> psCircle; //default typedef
  typedef bun::Circle<double> psCircled;

  typedef bun::Ellipse<float> psEllipse; //default typedef
  typedef bun::Ellipse<double> psEllipsed;

  typedef bun::Polygon<float> psPolygon; //default typedef
  typedef bun::Polygon<double> psPolygond;
}

#endif