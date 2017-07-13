// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __LOCATABLE_H__PS__
#define __LOCATABLE_H__PS__

#include "psVec.h"
#include "psDriver.h"
#include "psParent.h"

namespace planeshader {
  // This holds position information. 
  class PS_DLLEXPORT psLocatable : protected psParent
  {
  public:
    psLocatable(const psLocatable& copy);
    explicit psLocatable(const psVec3D& position, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO);
    virtual ~psLocatable();
    // Gets the rotation 
    inline FNUM GetRotation() const { return rotation; }
    // Sets the rotation of this object 
    inline void SetRotation(FNUM r) { rotation = r; }
    // Gets the pivot
    inline const psVec& GetPivot() const { return pivot; }
    // Sets the pivot (this is where the image will rotate around) 
    inline void SetPivot(const psVec& p) { pivot = p; }
    // Gets the position relative to the object parent 
    inline const psVec3D& GetPosition() const { return position; }
    // Sets the position of this object 
    inline void SetPositionX(FNUM X) { position.x = X; }
    inline void SetPositionY(FNUM Y) { position.y = Y; }
    inline void SetPositionZ(FNUM Z) { position.z = Z; }
    inline void SetPosition(const psVec& pos) { position.xy = pos; }
    inline void SetPosition(const psVec3D& pos) { position = pos; }
    inline void SetPosition(FNUM X, FNUM Y) { position.x = X; position.y = Y; }
    inline void SetPosition(FNUM X, FNUM Y, FNUM Z) { position.x = X; position.y = Y; position.z = Z; }
    inline void GetTransform(psMatrix& matrix, const psParent* parent) { psParent::GetTransform(matrix, parent); }
    inline const psParent& ToParent() const { return *this; }

    inline psLocatable& operator=(const psLocatable& right) { position=right.position; rotation=right.rotation; pivot=right.pivot; return *this; }
  };
}

#endif