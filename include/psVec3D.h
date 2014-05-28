// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __VEC3D_H__PS__
#define __VEC3D_H__PS__

#include "psVec.h"

namespace planeshader {
  // This is a 3D position vector to allow for depth. It inherits psVecT and adds 3D versions of the functions. Note that the 2D versions all still function, but they will ignore the z component. 
  template<typename T>
  struct _declspec(dllexport) psVec3DT : public psVecT<T>
  {
  public:
    inline psVec3DT() {}
    inline psVec3DT(const psVec3DT<T>& copy) : psVecT(copy), z(copy.z) {}
    // This allows for conversion between types 
    template<class U>
    inline psVec3DT(const psVec3DT<U>& copy) : psVecT(copy), z((T)copy.z) {}
    explicit inline psVec3DT(T XYZ) : psVecT(XYZ), z(XYZ) {}
    inline psVec3DT(T X, T Y, T Z) : psVecT(X, Y), z(Z) {}
    explicit inline psVec3DT(const T(&posarray)[3]) : psVecT(posarray[0], posarray[1]), z(posarray[2]) {}
    inline bool BSS_FASTCALL Equals(T X, T Y, T Z) const { return X == x && Y == y && Z==z; }
    inline T BSS_FASTCALL GetDistance(const psVec3DT<T>& other) const { T tz = (other.z - z);  T ty = (other.y - y); T tx = (other.x - x); return (T)bss_util::FastSqrt((tx*tx)+(ty*ty)+(tz*tz)); }
    inline T BSS_FASTCALL GetDistanceSquared(const psVec3DT<T>& other) const { T tz = (other.z - z); T ty = (other.y - y); T tx = (other.x - x); return (T)((tx*tx)+(ty*ty)+(tz*tz)); }
    inline T BSS_FASTCALL GetDistance(T X, T Y, T Z) const { T tz = (other.z - z); T ty = (Y - y); T tx = (X - x); return (T)bss_util::FastSqrt((tx*tx)+(ty*ty)+(tz*tz)); }
    inline T BSS_FASTCALL GetDistanceSquared(T X, T Y, T Z) const { T tz = (other.z - z); T ty = (Y - y); T tx = (X - x); return (T)((tx*tx)+(ty*ty)+(tz*tz)); }
    inline T GetLength() const { return bss_util::FastSqrt((x*x)+(y*y)+(z*z)); }
    inline psVec3DT<T> Normalize() const { T length = GetLength(); return psVec3DT<T>(x/length, y/length, z/length); }
    inline psVec3DT<T> Abs() const { return psVec3DT<T>(abs(x), abs(y), abs(z)); }
    inline psVec3DT<T> CrossProduct(const psVec3DT<T>& other) const { return CrossProduct(other.x, other.y, other.z); }
    inline psVec3DT<T> CrossProduct(T X, T Y, T Z) const { return psVec3DT<T>(y*Z - z*Y, z*X - x*Z, x*Y - X*y); }
    inline float DotProduct(const psVec3DT<T>& other) const { return x * other.x + y * other.y + z * other.z; }
    inline psVec3DT<T> BSS_FASTCALL MatrixMultiply(const T(&m)[9]) const { return psVec3DT<T>((x*m[0])+(y*m[1])+(z*m[2]), (x*m[3])+(y*m[4])+(z*m[5]), (x*m[6])+(y*m[7])+(z*m[8])); }
    inline const psVecT<T>& to2D() const { return *this; }
    inline psVecT<T>& to2D() { return *this; }

    inline const psVec3DT<T> BSS_FASTCALL operator +(const psVec3DT<T>& other) const { return psVec3DT<T>(x + other.x, y + other.y, z + other.z); }
    inline const psVec3DT<T> BSS_FASTCALL operator -(const psVec3DT<T>& other) const { return psVec3DT<T>(x - other.x, y - other.y, z - other.z); }
    inline const psVec3DT<T> BSS_FASTCALL operator *(const psVec3DT<T>& other) const { return psVec3DT<T>(x * other.x, y * other.y, z * other.z); }
    inline const psVec3DT<T> BSS_FASTCALL operator /(const psVec3DT<T>& other) const { return psVec3DT<T>(x / other.x, y / other.y, z / other.z); }
    inline const psVec3DT<T> BSS_FASTCALL operator +(const psVecT<T>& other) const { return psVec3DT<T>(x + other.x, y + other.y, z); }
    inline const psVec3DT<T> BSS_FASTCALL operator -(const psVecT<T>& other) const { return psVec3DT<T>(x - other.x, y - other.y, z); }
    inline const psVec3DT<T> BSS_FASTCALL operator *(const psVecT<T>& other) const { return psVec3DT<T>(x * other.x, y * other.y, z); }
    inline const psVec3DT<T> BSS_FASTCALL operator /(const psVecT<T>& other) const { return psVec3DT<T>(x / other.x, y / other.y, z); }
    inline const psVec3DT<T> BSS_FASTCALL operator +(const T other) const { return psVec3DT<T>(x + other, y + other, z + other); }
    inline const psVec3DT<T> BSS_FASTCALL operator -(const T other) const { return psVec3DT<T>(x - other, y - other, z - other); }
    inline const psVec3DT<T> BSS_FASTCALL operator *(const T other) const { return psVec3DT<T>(x * other, y * other, z * other); }
    inline const psVec3DT<T> BSS_FASTCALL operator /(const T other) const { return psVec3DT<T>(x / other, y / other, z / other); }

    inline psVec3DT<T>& BSS_FASTCALL operator +=(const psVec3DT<T> &other) { x += other.x; y += other.y; z += other.z; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator -=(const psVec3DT<T> &other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator *=(const psVec3DT<T> &other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator /=(const psVec3DT<T> &other) { x /= other.x; y /= other.y; z /= other.z; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator +=(const psVecT<T> &other) { x += other.x; y += other.y; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator -=(const psVecT<T> &other) { x -= other.x; y -= other.y; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator *=(const psVecT<T> &other) { x *= other.x; y *= other.y; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator /=(const psVecT<T> &other) { x /= other.x; y /= other.y; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator +=(const T other) { x += other; y += other; z += other; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator -=(const T other) { x -= other; y -= other; z -= other; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator *=(const T other) { x *= other; y *= other; z *= other; return *this; }
    inline psVec3DT<T>& BSS_FASTCALL operator /=(const T other) { x /= other; y /= other; z /= other; return *this; }
    //all const T functions are disabled to prevent mixups.

    inline const psVec3DT<T> operator -(void) const { return psVec3DT<T>(-x, -y, -z); }

    inline bool BSS_FASTCALL operator !=(const psVec3DT<T> &other) const { return (x != other.x) || (y != other.y) || (z != other.z); }
    inline bool BSS_FASTCALL operator ==(const psVec3DT<T> &other) const { return (x == other.x) && (y == other.y) && (z == other.z); }
    inline bool BSS_FASTCALL operator !=(const T other) const { return (x != other) || (y != other) || (z != other); }
    inline bool BSS_FASTCALL operator ==(const T other) const { return (x == other) && (y == other) && (z == other); }
    inline bool BSS_FASTCALL operator >(const psVec3DT<T> &other) const { return (x > other.x) || ((x == other.x) && ((y > other.y) || ((y == other.y) && (z > other.z)))); }
    inline bool BSS_FASTCALL operator <(const psVec3DT<T> &other) const { return (x < other.x) || ((x == other.x) && ((y < other.y) || ((y == other.y) && (z < other.z)))); }
    inline bool BSS_FASTCALL operator >=(const psVec3DT<T> &other) const { return !operator<(other); }
    inline bool BSS_FASTCALL operator <=(const psVec3DT<T> &other) const { return !operator>(other); }
    inline bool BSS_FASTCALL operator >(const T other) const { return (x > other) && (y > other) && (z > other); }
    inline bool BSS_FASTCALL operator <(const T other) const { return (x < other) && (y < other) && (z < other); }
    inline bool BSS_FASTCALL operator >=(const T other) const { return (x >= other) && (y >= other) && (z >= other); }
    inline bool BSS_FASTCALL operator <=(const T other) const { return (x <= other) && (y <= other) && (z <= other); }

    inline psVec3DT<T>& BSS_FASTCALL operator=(const psVec3DT<T>& other) { x=other.x; y=other.y; z=other.z; return *this; }
    template<class U>
    inline psVec3DT<T>& BSS_FASTCALL operator=(const psVec3DT<U>& other) { x=(T)other.x; y=(T)other.y; z=(T)other.z; return *this; }

    T z;
  };

  typedef psVec3DT<float> psVec3D; //default typedef
  typedef psVec3DT<float> psVector3D;
  typedef psVec3DT<int> psVec3Di;
  typedef psVec3DT<double> psVec3Dd;
  typedef psVec3DT<short> psVec3Ds;
  typedef psVec3DT<long> psVec3Dl;
  typedef psVec3DT<unsigned int> psVec3Diu;
  typedef psVec3DT<unsigned short> psVec3Dsu;
  typedef psVec3DT<unsigned long> psVec3Dlu;

  psVec3D const VEC3D_ZERO(0, 0, 0);
  psVec3D const VEC3D_ONE(1, 1, 1);

}

#endif