// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __VEC_H__PS__
#define __VEC_H__PS__

#include "ps_dec.h"
#include "bss-util/bss_util.h"

namespace planeshader {
  // psVecT (usually used typedef'd to psVecT<f> by the name psVec stores an x and y coordinate and helps interact with that coordinate 
  template <class T>
  struct BSS_COMPILER_DLLEXPORT psVecT
  {
    inline psVecT() {}
    //inline psVecT(const psVecT<T>& copy) : x(copy.x),y(copy.y) {}
    // This allows for conversion between types 
    template<class U>
    inline psVecT(const psVecT<U>& copy) : x((T)copy.x), y((T)copy.y) {}
    explicit inline psVecT(T XY) : x(XY), y(XY) {}
    inline psVecT(T X, T Y) : x(X), y(Y) {}
    explicit inline psVecT(const T(&posarray)[2]) : x(posarray[0]), y(posarray[1]) {}
    inline bool BSS_FASTCALL IntersectCircle(const psVecT<T>& pos, T R) const { return IntersectCircle(pos.x, pos.y, R); }
    inline bool BSS_FASTCALL IntersectCircle(T X, T Y, T R) const { return bss_util::distsqr<T>(X, Y, x, y)<=(R*R); }
    inline bool BSS_FASTCALL IntersectRect(T left, T top, T right, T bottom) const { return (x >= left && y >= top && x < right && y < bottom); } // All rectangles are treated as inclusive-exclusive.
    inline bool BSS_FASTCALL IntersectRect(const T(&rect)[4]) const { return (x >= rect[0] && y >= rect[1] && x < rect[2] && y < rect[3]); } //array must be in left top right bottom format
    inline bool BSS_FASTCALL IntersectEllipse(const psVecT<T>& pos, T A, T B) const { return IntersectEllipse(pos.x, pos.y, A, B, x, y); }
    inline bool BSS_FASTCALL IntersectEllipse(T X, T Y, T A, T B) const { return IntersectEllipse(X, Y, A, B, x, y); }
    inline T BSS_FASTCALL Distance(const psVecT<T>& other) const { return bss_util::dist<T>(other.x, other.y, x, y); }
    inline T BSS_FASTCALL DistanceSquared(const psVecT<T>& other) const { return bss_util::distsqr<T>(other.x, other.y, x, y); }
    inline T BSS_FASTCALL Distance(T X, T Y) const { return bss_util::dist<T>(X, Y, x, y); }
    inline T BSS_FASTCALL DistanceSquared(T X, T Y) const { return bss_util::distsqr<T>(X, Y, x, y); }
    inline T BSS_FASTCALL LineInfDistance(T X1, T Y1, T X2, T Y2) const { return PointLineInfDist(x, y, X1, Y1, X2, Y2); } //treats line as infinite
    inline T BSS_FASTCALL LineInfDistanceSqr(T X1, T Y1, T X2, T Y2) const { return PointLineInfDistSqr(x, y, X1, Y1, X2, Y2); } //see above
    inline T BSS_FASTCALL LineDistance(T X1, T Y1, T X2, T Y2) const { return PointLineDist(x, y, X1, Y1, X2, Y2); }
    inline T BSS_FASTCALL LineDistanceSqr(T X1, T Y1, T X2, T Y2) const { return PointLineDistSqr(x, y, X1, Y1, X2, Y2); }
    inline T Length() const { return bss_util::FastSqrt((x*x)+(y*y)); }
    inline psVecT<T> Normalize() const { assert(Length()!=0.0f); T invlength = 1/Length(); return psVecT<T>(x*invlength, y*invlength); }
    inline const psVecT<T> Abs() const { return psVecT<T>(abs(x), abs(y)); }
    inline psVecT<T> BSS_FASTCALL Rotate(T R, const psVecT<T>& center) const { return Rotate(R, center.x, center.y); }
    inline psVecT<T> BSS_FASTCALL Rotate(T R, T X, T Y) const { float tx=x; float ty=y; RotatePoint(tx, ty, R, X, Y); return psVecT<T>(tx, ty); }
    inline float BSS_FASTCALL CrossProduct(const psVecT<T>& other) const { return CrossProduct(other.x, other.y, x, y); }
    inline float BSS_FASTCALL CrossProduct(T X, T Y) const { return CrossProduct(X, Y, x, y); }
    inline float BSS_FASTCALL DotProduct(const psVecT<T>& other) const { return DotProduct(other.x, other.y, x, y); }
    inline float BSS_FASTCALL DotProduct(T X, T Y) const { return DotProduct(X, Y, x, y); }
    inline const psVecT<T> BSS_FASTCALL MatrixMultiply(const T(&m)[4]) const { return psVecT<T>((x*m[0])+(y*m[1]), (x*m[2])+(y*m[3])); }

    inline const psVecT<T> BSS_FASTCALL operator +(const psVecT<T>& other) const { return psVecT<T>(x + other.x, y + other.y); }
    inline const psVecT<T> BSS_FASTCALL operator -(const psVecT<T>& other) const { return psVecT<T>(x - other.x, y - other.y); }
    inline const psVecT<T> BSS_FASTCALL operator *(const psVecT<T>& other) const { return psVecT<T>(x * other.x, y * other.y); }
    inline const psVecT<T> BSS_FASTCALL operator /(const psVecT<T>& other) const { return psVecT<T>(x / other.x, y / other.y); }
    inline const psVecT<T> BSS_FASTCALL operator +(const T other) const { return psVecT<T>(x + other, y + other); }
    inline const psVecT<T> BSS_FASTCALL operator -(const T other) const { return psVecT<T>(x - other, y - other); }
    inline const psVecT<T> BSS_FASTCALL operator *(const T other) const { return psVecT<T>(x * other, y * other); }
    inline const psVecT<T> BSS_FASTCALL operator /(const T other) const { return psVecT<T>(x / other, y / other); }

    inline psVecT<T>& BSS_FASTCALL operator +=(const psVecT<T> &other) { x += other.x; y += other.y; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator -=(const psVecT<T> &other) { x -= other.x; y -= other.y; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator *=(const psVecT<T> &other) { x *= other.x; y *= other.y; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator /=(const psVecT<T> &other) { x /= other.x; y /= other.y; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator +=(const T other) { x += other; y += other; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator -=(const T other) { x -= other; y -= other; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator *=(const T other) { x *= other; y *= other; return *this; }
    inline psVecT<T>& BSS_FASTCALL operator /=(const T other) { x /= other; y /= other; return *this; }

    inline const psVecT<T> operator -(void) const { return psVecT<T>(-x, -y); }

    inline bool BSS_FASTCALL operator !=(const psVecT<T> &other) const { return (x != other.x) || (y != other.y); }
    inline bool BSS_FASTCALL operator ==(const psVecT<T> &other) const { return (x == other.x) && (y == other.y); }
    inline bool BSS_FASTCALL operator !=(const T other) const { return (x != other) || (y != other); }
    inline bool BSS_FASTCALL operator ==(const T other) const { return (x == other) && (y == other); }
    inline bool BSS_FASTCALL operator >(const psVecT<T> &other) const { return (x > other.x) || ((x == other.x) && (y > other.y)); }
    inline bool BSS_FASTCALL operator <(const psVecT<T> &other) const { return (x < other.x) || ((x == other.x) && (y < other.y)); }
    inline bool BSS_FASTCALL operator >=(const psVecT<T> &other) const { return !operator<(other); }
    inline bool BSS_FASTCALL operator <=(const psVecT<T> &other) const { return !operator>(other); }
    inline bool BSS_FASTCALL operator >(const T other) const { return (x > other) && (y > other); }
    inline bool BSS_FASTCALL operator <(const T other) const { return (x < other) && (y < other); }
    inline bool BSS_FASTCALL operator >=(const T other) const { return (x >= other) && (y >= other); }
    inline bool BSS_FASTCALL operator <=(const T other) const { return (x <= other) && (y <= other); }

    inline psVecT<T>& BSS_FASTCALL operator=(const psVecT<T>& other) { x=other.x; y=other.y; return *this; }
    template<class U>
    inline psVecT<T>& BSS_FASTCALL operator=(const psVecT<U>& other) { x=(T)other.x; y=(T)other.y; return *this; }

    static BSS_FORCEINLINE void BSS_FASTCALL RotatePoint(T& x, T& y, T r, T cx, T cy) { T tx = x-cx; T ty = y-cy; T rcos = (T)cos(r); T rsin = (T)sin(r); x = (tx*rcos - ty*rsin)+cx; y = (ty*rcos + tx*rsin)+cy; }
    static BSS_FORCEINLINE void BSS_FASTCALL RotatePoint(T& x, T& y, T r) { T tx=x; T rcos = (T)cos(r); T rsin = (T)sin(r); x = (tx*rcos - y*rsin); y = (y*rcos + tx*rsin); }
    static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL FromPolar(const psVecT<T>& v) { return FromPolar(v.x, v.y); }
    static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL FromPolar(T r, T angle) { return psVecT<T>(r*cos(angle), r*sin(angle)); }
    static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL ToPolar(const psVecT<T>& v) { return ToPolar(v.x, v.y); }
    static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL ToPolar(T x, T y) { return psVecT<T>(bss_util::FastSqrt((x*x) + (y*y)), atan2(y, x)); } //x - r, y - theta
    static BSS_FORCEINLINE bool BSS_FASTCALL IntersectEllipse(T X, T Y, T A, T B, T x, T y) { T tx=X-x; T ty=Y-y; return ((tx*tx)/(A*A)) + ((ty*ty)/(B*B)) <= 1; }
    static BSS_FORCEINLINE T BSS_FASTCALL DotProduct(T X, T Y, T x, T y) { return X*x + Y*y; }
    static BSS_FORCEINLINE T BSS_FASTCALL CrossProduct(T X, T Y, T x, T y) { return x*Y - X*y; }
    static BSS_FORCEINLINE T BSS_FASTCALL PointLineInfDistSqr(T X, T Y, T X1, T Y1, T X2, T Y2) { T tx=X2-X1; T ty=Y2-Y1; T det=(tx*(Y1-Y)) - ((X1-X)*ty); return (det*det)/((tx*tx)+(ty*ty)); }
    static BSS_FORCEINLINE T BSS_FASTCALL PointLineInfDist(T X, T Y, T X1, T Y1, T X2, T Y2) { T tx=X2-X1; T ty=Y2-Y1; return ((tx*(Y1-Y)) - ((X1-X)*ty))/bss_util::FastSqrt((tx*tx)+(ty*ty)); }
    static inline T BSS_FASTCALL PointLineDistSqr(T X, T Y, T X1, T Y1, T X2, T Y2) //line segment
    {
      float vx = X1-X; float vy = Y1-Y; float ux = X2-X1; float uy = Y2-Y1; float length = ux*ux+uy*uy;
      float det = (-vx*ux)+(-vy*uy); //if this is < 0 or > length then its outside the line segment
      if(det<0 || det>length) { ux=X2-X; uy=Y2-Y; vx=vx*vx+vy*vy; ux=ux*ux+uy*uy; return (vx<ux)?vx:ux; }
      det = ux*vy-uy*vx;
      return (det*det)/length;
    }
    static BSS_FORCEINLINE T BSS_FASTCALL PointLineDist(T X, T Y, T X1, T Y1, T X2, T Y2) { return bss_util::FastSqrt(PointLineDistSqr(X, Y, X1, Y1, X2, Y2)); }
    static inline void BSS_FASTCALL NearestPointToLineInf(T X, T Y, T X1, T Y1, T X2, T Y2, T& outX, T& outY) //treated as infinite line
    {
      T tx=X2-X1; T ty=Y2-Y1;
      T u = ((X-X1)*tx + (Y-Y1)*ty)/((tx*tx) + (ty*ty));

      outX=X1 + u*tx;
      outY=Y1 + u*ty;
    }
    static inline void BSS_FASTCALL NearestPointToLine(T X, T Y, T X1, T Y1, T X2, T Y2, T& outX, T& outY) //treated as line segment
    {
      T ux=X2-X1; T uy=Y2-Y1;
      T vx=X1-X; T vy=Y1-Y;
      float det = (-vx*ux)+(-vy*uy); //if this is < 0 or > length then its outside the line segment
      float length = ux*ux+uy*uy;
      if(det<0 || det>length)
      {
        ux=X2-X; uy=Y2-Y;
        vx=vx*vx+vy*vy; ux=ux*ux+uy*uy;
        if(vx<ux) { outX=X1; outY=Y1; } else { outX=X2; outY=Y2; } return;
      }

      T u = ((X-X1)*ux + (Y-Y1)*uy)/((ux*ux) + (uy*uy));

      outX=X1 + u*ux;
      outY=Y1 + u*uy;
    }
    // Method based on: http://www2.imperial.ac.uk/~rn/distance2ellipse.pdf (Note that ellipse must be centered at origin, so
    // (cx,cy) = (X-x,Y-y), where (X,Y) is the ellipse coordinates and (x,y) is the point
    static inline void BSS_FASTCALL EllipseNearestPoint(T A, T B, T cx, T cy, T& outX, T& outY)
    {
      //f(t) = (A*A - B*B)cos(t)sin(t) - x*A*sin(t) + y*B*cos(t)
      //f'(t) = (A*A - B*B)(cos²(t) - sin²(t)) - x*a*cos(t) - y*B*sin(t)
      //x(t) = [A*cos(t),B*sin(t)]
      T A2=A*A;
      T B2=B*B;
      if(((cx*cx)/A2) + ((cy*cy)/B2) <= 1)
      {
        outX=cx;
        outY=cy;
        return;
      }
      T t = atan2(A*cy, B*cx); //Initial guess = arctan(A*y/B*x)
      T AB2 = (A2 - B2); //precalculated values for optimization
      T xA = cx*A;
      T yB = cy*B;
      T f_, f__, tcos, tsin;
      for(int i = 0; i < 4; ++i)
      {
        tcos=cos(t);
        tsin=sin(t);
        f_ = AB2*tcos*tsin - xA*tsin + yB*tcos; //f(t)
        f__ = AB2*(tcos*tcos - tsin*tsin) - xA*tcos - yB*tsin; //f'(t)
        t = t - f_/f__; //Newton's method: t - f(t)/f'(t)
      }

      outX=A*cos(t);
      outY=B*sin(t);
    }

    union
    {
      struct
      {
        T x;
        T y;
      };
      T _xyarray[2];
      //unsigned __int64 _64bitvar;
    };
  };

  typedef psVecT<float> psVec; //default typedef
  typedef psVecT<float> psVector;
  typedef psVecT<int> psVeci;
  typedef psVecT<double> psVecd;
  typedef psVecT<short> psVecs;
  typedef psVecT<long> psVecl;
  typedef psVecT<unsigned int> psVeciu;
  typedef psVecT<unsigned short> psVecsu;
  typedef psVecT<unsigned long> psVeclu;

  psVec const VEC_ZERO(0, 0);
  psVec const VEC_ONE(1, 1);
  psVec const VEC_HALF(0.5f, 0.5f);
  psVec const VEC_NEGHALF(-0.5f, -0.5f);
}

#endif