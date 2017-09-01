// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __INTERFACE_H__PS__
#define __INTERFACE_H__PS__

#include "ps_dec.h"
#include <stdint.h>

#ifdef  __cplusplus
using namespace planeshader;
extern "C" {
#endif

struct PSINIT;
struct psEngine;
struct psDriver;
struct psCamera;
struct psLayer;
struct psLocatable;
struct psRenderable;
struct psShader;
struct psTex;
struct psStateblock;
struct psInheritable;
struct psSolid;
struct psTextured;
struct psImage;

struct psVec {
  FNUM x;
  FNUM y;
};
struct psVeci {
  int x;
  int y;
};
struct psVeciu {
  uint32_t x;
  uint32_t y;
};
struct psVec3D {
  FNUM x;
  FNUM y;
  FNUM z;
};
struct psCircle {
  FNUM r;
  FNUM x;
  FNUM y;
};
struct psRect {
  FNUM left;
  FNUM top;
  FNUM right;
  FNUM bottom;
};
struct psRectl {
  long left;
  long top;
  long right;
  long bottom;
};



#ifdef  __cplusplus
}
#endif
#endif