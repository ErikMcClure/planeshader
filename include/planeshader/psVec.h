// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __VEC_H__PS__
#define __VEC_H__PS__

#include "ps_dec.h"
#include "bss-util/Geometry.h"

namespace planeshader {
  typedef bss::Vector<float, 2> psVec; //default typedef
  typedef bss::Vector<float, 2> psVector;
  typedef bss::Vector<int, 2> psVeci;
  typedef bss::Vector<double, 2> psVecd;
  typedef bss::Vector<uint32_t, 2> psVeciu;

  static psVec const VEC_ZERO(0, 0);
  static psVec const VEC_ONE(1, 1);
  static psVec const VEC_HALF(0.5f, 0.5f);
  static psVec const VEC_NEGHALF(-0.5f, -0.5f);

  typedef bss::Vector<float, 3> psVec3D; //default typedef
  typedef bss::Vector<float, 3> psVector3D;
  typedef bss::Vector<int, 3> psVec3Di;
  typedef bss::Vector<double, 3> psVec3Dd;
  typedef bss::Vector<uint32_t, 3> psVec3Diu;

  static psVec3D const VEC3D_ZERO(0, 0, 0);
  static psVec3D const VEC3D_ONE(1, 1, 1);

  typedef bss::Rect<FNUM> psRect; //Default typedef
  typedef bss::Rect<int> psRecti;
  typedef bss::Rect<double> psRectd;
  typedef bss::Rect<long> psRectl;
  typedef bss::Rect<uint32_t> psRectiu;

  psRect const RECT_ZERO(0, 0, 0, 0);
  psRect const RECT_UNITRECT(0, 0, 1, 1);

  typedef bss::Line<float> psLine; //default typedef
  typedef bss::Line<double> psLined;

  typedef bss::Line3d<float> psLine3D; //default typedef
  typedef bss::Line3d<double> psLine3Dd;

  typedef bss::Circle<float> psCircle; //default typedef
  typedef bss::Circle<double> psCircled;

  typedef bss::Ellipse<float> psEllipse; //default typedef
  typedef bss::Ellipse<double> psEllipsed;

  typedef bss::Polygon<float> psPolygon; //default typedef
  typedef bss::Polygon<double> psPolygond;
}

#endif