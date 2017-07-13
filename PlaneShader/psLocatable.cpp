// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psLocatable.h"
#include "bss-util/Delegate.h"

using namespace planeshader;

psLocatable::psLocatable(const psLocatable& copy) : psParent(copy) {}
psLocatable::psLocatable(const psVec3D& pos, FNUM r, const psVec& p) { position = pos; rotation = r; pivot = p; }
psLocatable::~psLocatable() {}