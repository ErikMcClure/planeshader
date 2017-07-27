// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __RECT_H__PS__
#define __RECT_H__PS__

#include "psVec.h"
#include "bss-util/sseVec.h"

namespace planeshader {
  // The psRectRotateT class is a rotated rectagle 
  template<class T>
  struct BSS_COMPILER_DLLEXPORT psRectRotateT : public bss::Rect<T>
  {
    inline psRectRotateT() {} //The following constructors allow for implicit conversion between rect types
    template<class U>
    inline psRectRotateT(const psRectRotateT<U>& other) : bss::Rect<T>(other), rotation(other.rotation), pivot(other.pivot) {}
    inline psRectRotateT(T Left, T Top, T Right, T Bottom, T rotation, const V& pivot=VEC_ZERO) : bss::Rect<T>(Left, Top, Right, Bottom), rotation(rotation), pivot(pivot) { }
    inline psRectRotateT(T X, T Y, const V& dim, T rotation, const V& pivot=VEC_ZERO) : rotation(rotation), pivot(pivot) { left = X; right = X+dim.x; top = Y; bottom = Y+dim.y; }
    inline psRectRotateT(const bss::Rect<T>& rect, T rotation=0.0f, const V& pivot=VEC_ZERO) : bss::Rect<T>(rect), rotation(rotation), pivot(pivot) { }
    inline psRectRotateT(const V& pos, const V& dim, T rotation, const V& pivot=VEC_ZERO) : rotation(rotation), pivot(pivot) { left = pos.x; right = pos.x+dim.x; top = pos.y; bottom = pos.y+dim.y; }
    inline bool IntersectPoint(T x, T y) const
    {
      if(!bss::fSmall(rotation))
        V::RotatePoint(x, y, -rotation, pivot.x+left, pivot.y+top);
      return bss::Rect<T>::IntersectPoint(x, y);
    }
    inline bool IntersectPoint(const V& point) const { return IntersectPoint(point.x, point.y); }

    // This builds an Axis-Aligned Bounding Box from the rotated rectangle, rotated around a pivot RELATIVE TO THE TOPLEFT CORNER OF THE RECTANGLE. It does this by representing the box by the distance from an arbitrary rotation axis, and rotating that axis. 
    inline bss::Rect<T> BuildAABB() const
    {
      if(bss::fSmall(rotation))
        return *this;
      float c = (cos(rotation));
      float s = (sin(rotation));
      float ex = ((right-left)*0.5f);
      float ey = ((bottom-top)*0.5f);
      float x_radius = ex * abs(c) + ey * abs(s);
      float y_radius = ex * abs(s) + ey * abs(c);

      //Rotate pivot around origin. ex and ey equal the center of the rectangle minus the topleft corner. That is to say, psVec((r+l)/2 - l, (b+t)/2 - t) -> psVec((r+l)/2 - 2*l/2, (b+t)/2 - 2*t/2) -> psVec((r+l-2l)/2, (b+t-2t)/2) -> psVec((r-l)/2, (b-t)/2) == psVec(ex,ey) since ex and ey were already set to that.
      ex-=pivot.x;
      ey-=pivot.y;
      float tmpex=ex;
      ex = ex * c - ey * s;
      ey = ey * c + tmpex * s;
      ex+=pivot.x+left;
      ey+=pivot.y+top;

      return bss::Rect<T>(ex - x_radius, ey - y_radius, ex + x_radius, ey + y_radius);
    }

    inline psRectRotateT<T>& operator =(const bss::Rect<T>& _right) { bss::Rect<T>::operator =(_right); return *this; }
    inline psRectRotateT<T>& operator =(const psRectRotateT<T>& _right) { if(&_right!=this) { bss::Rect<T>::operator =(_right); rotation=_right.rotation; pivot=_right.pivot; } return *this; }
    BSS_FORCEINLINE psRectRotateT<T> EnforceLTRB() const { return psRectRotateT<T>(bss::Rect<T>::EnforceLTRB(), rotation, pivot); }
    BSS_FORCEINLINE psRectRotateT<T> Inflate(T amount) const { return psRectRotateT<T>(bss::Rect<T>::Inflate(amount), rotation, pivot); }
    inline psRectRotateT RelativeTo(const psVec& pos, float r, const psVec& p) const
    {
      psVec dim = { right - left, bottom - top };
      psRectRotateT ret(*this, r + rotation, pivot);
      if(r != 0.0f)
        psVec::RotatePoint(ret.left, ret.top, r, ret.left - pivot.x, ret.top - pivot.y);
      ret.topleft += pos + p;
      ret.bottomright = ret.topleft + dim;
      return ret;
    }

    T rotation;
    V pivot; //this is what its rotated around
  };

  typedef psRectRotateT<FNUM> psRectRotate; //Default typedef
  typedef psRectRotateT<int> psRectRotatei;
  typedef psRectRotateT<double> psRectRotated;
  typedef psRectRotateT<short> psRectRotates;
  typedef psRectRotateT<long> psRectRotatel;
  typedef psRectRotateT<uint32_t> psRectRotateiu;
  typedef psRectRotateT<uint16_t> psRectRotatesu;
  typedef psRectRotateT<unsigned long> psRectRotatelu;

  // The psRectRotateZT class is a rotated rectangle with a Z coordinate for the sole purpose of storing that Z coordinate in the culling rect
  template<class T>
  struct BSS_COMPILER_DLLEXPORT psRectRotateZT : public psRectRotateT<T>
  {
    inline psRectRotateZT() {}
    template<class U>
    inline psRectRotateZT(const psRectRotateZT<U>& other) : psRectRotateT<T>(other), z((T)other.z) {}
    inline psRectRotateZT(T Left, T Top, T Right, T Bottom, T rotation, const V& pivot=VEC_ZERO, FNUM Z=0.0f) : psRectRotateT<T>(Left, Top, Right, Bottom, rotation, pivot), z(Z) { }
    inline psRectRotateZT(const psRectRotateT<T>& rect, FNUM Z=0.0f) : psRectRotateT<T>(rect), z(Z) { }

    inline psRectRotateZT<T>& operator =(const bss::Rect<T>& _right) { bss::Rect<T>::operator =(_right); return *this; }
    inline psRectRotateZT<T>& operator =(const psRectRotateT<T>& _right) { psRectRotateT<T>::operator =(_right); return *this; }
    inline psRectRotateZT<T>& operator =(const psRectRotateZT<T>& _right) { psRectRotateT<T>::operator =(_right); z=_right.z; return *this; }
    BSS_FORCEINLINE psRectRotateZT<T> EnforceLTRB() const { return psRectRotateZT<T>(psRectRotateT<T>::EnforceLTRB(), z); }
    BSS_FORCEINLINE psRectRotateZT<T> Inflate(T amount) const { return psRectRotateZT<T>(psRectRotateT<T>::Inflate(amount), z); }
    BSS_FORCEINLINE psRectRotateZT RelativeTo(const psVec3D& pos, float r, const psVec& p) const { return psRectRotateZT<T>(psRectRotateT<T>::RelativeTo(pos.xy, r, p), pos.z + z); }

    T z;
  };

  typedef psRectRotateZT<FNUM> psRectRotateZ; // Why would you ever use anything other than the FNUM typedef on this struct?
}

#endif