// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __CIRCLE_SEGMENT_H__PS__
#define __CIRCLE_SEGMENT_H__PS__

#include "psVec.h"
#include "psLine.h"

namespace planeshader {
  // A circular arc segment defined by a position, radius, min-arc and max-arc.
  template <class T>
  struct BSS_COMPILER_DLLEXPORT psCircleSegmentT
  {
    typedef bss::Vector<T, 2> VEC;
    inline psCircleSegmentT() {} //The following constructors allow for implicit conversion between types
    template<class U>
    inline psCircleSegmentT(const psCircleSegmentT<U>& other) : x((T)other.x), y((T)other.y), inner((T)other.inner), outer((T)other.outer), min((T)other.min), range((T)other.range) {}
    inline psCircleSegmentT(T X, T Y, T Inner, T Outer, T Min, T Range) : x(X), y(Y), inner(Inner), outer(Outer), min(bss::bssFMod<T>(Min, (T)bss::PI_DOUBLE)), range(Range) {}
    inline psCircleSegmentT(const VEC& Pos, T Inner, T Outer, T Min, T Range) : x(Pos.x), y(Pos.y), inner(Inner), outer(Outer), min(bss::bssFMod<T>(Min, (T)bss::PI_DOUBLE)), range(Range) {}
    inline T Area() const { return (T)(r*r*range*0.5); } // (r²*angle)/2
    inline T ArcLength() const { return range*r }
    inline T Perimeter() const { return (range + 2)*r; }

    inline bool IntersectPolygon(const VEC* vertices, size_t num)
    {
      if(num < 1)
        return false;
      if(ContainsPoint(vertices[0].x, vertices[0].y))
        return true;
      if(num < 2)
        return false;
      if(num < 3)
        return _intersectLine(vertices[0].x - x, vertices[0].y - y, vertices[1].x - x, vertices[1].y - y);

      for(size_t i = 1; i < num; ++i)
      {
        if(_intersectLine(vertices[i - 1].x - x, vertices[i - 1].y - y, vertices[i].x - x, vertices[i].y - y))
          return true;
      }
      if(_intersectLine(vertices[i].x - x, vertices[i].y - y, vertices[0].x - x, vertices[0].y - y))
        return true;
      return false;
    }

    inline bool IntersectLine(T X1, T Y1, T X2, T Y2)
    {
      X1 -= x;
      Y1 -= y;
      X2 -= x;
      Y2 -= y;
      if(_pointInside(X1, Y1, inner, outer, min, range))
        return true;
      return _intersectLine(X1, Y1, X2, Y2);
    }

    // Constructs a line segment for the given circle segment angle and checks if it collides with the given line segment
    BSS_FORCEINLINE static bool _lineRayIntersect(T X1, T Y1, T X2, T Y2, T inner, T outer, T angle)
    {
      return psLineT<T>::LineLineIntersect(cos(angle)*inner, -sin(angle)*inner, cos(angle)*outer, -sin(angle)*outer, X1, Y1, X2, Y2);
    }
    BSS_FORCEINLINE static bool _rayRayIntersect(T X, T Y, T Inner, T Outer, T Angle, T inner, T outer, T angle)
    {
      return _lineRayIntersect(X + cos(Angle)*Inner, Y - sin(Angle)*Inner, X + cos(Angle)*Outer, Y - sin(Angle)*Outer, inner, outer, angle);
    }

    inline static int _lineSegmentRadiusIntersect(T X1, T Y1, T X2, T Y2, T r, T(&out)[2][2])
    {
      T Dx = X2 - X1;
      T Dy = Y2 - Y1;
      T Dr2 = Dx*Dx + Dy*Dy;
      T b = 2 * (X1*Dx + Y1*Dy);
      T c = (X1*X1 + Y1*Y1) - r*r;

      T d = b*b - 4 * Dr2*c;
      if(d < 0) // No intersection
        return 0;

      d = bss::FastSqrt<T>(d);
      T t1 = (-b - d) / (2 * Dr2);
      T t2 = (-b + d) / (2 * Dr2);
       
      int count = 0;
      if(t1 >= 0 && t1 <= 1) // check if t1 hit inside line segment
      {
        out[count][0] = X1 + Dx*t1;
        out[count][1] = Y1 + Dy*t1;
        ++count;
      }

      if(t2 >= 0 && t2 <= 1) // check if t2 hit inside line segment
      {
        out[count][0] = X1 + Dx*t2;
        out[count][1] = Y1 + Dy*t2;
        ++count;
      }

      return count;
    }
    inline static bool _lineSegmentRadiusIntersect(T X1, T Y1, T X2, T Y2, T r)
    {
      T Dx = X2 - X1;
      T Dy = Y2 - Y1;
      T Dr2 = Dx*Dx + Dy*Dy;
      T b = 2 * (X1*Dx + Y1*Dy);
      T c = (X1*X1 + Y1*Y1) - r*r;

      T d = b*b - 4 * Dr2*c;
      if(d < 0) // No intersection
        return false;

      d = bss::FastSqrt<T>(d);
      T t1 = (-b - d) / (2 * Dr2);
      T t2 = (-b + d) / (2 * Dr2);

      return (t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1);
    }
    inline bool IntersectCircle(T X, T Y, T R)
    {
      X -= x;
      Y -= y;
      T D2 = X*X + Y*Y;
      T tr = R + outer;
      if(D2 > tr*tr) // If the two circles don't intersect, bail out immediately
        return false;

      if(inner > 0) // Check if contained inside inner circle
      {
        T tr = inner - R;
        if(tr > 0 && D2 < (tr*tr))
          return false; // completely inside inner exclusion zone
      }

      if(range >= (T)bss::PI_DOUBLE) // At this point, if this is just a circle, it must intersect
        return true;

      if(_pointInAngle(X, Y, min, range))
        return true; // If the center of the circle is inside the segment, automatically return

      // Otherwise, generate line segments for each angle boundary and see if they intersect the circle
      if(_intersectAngleCircle(X, Y, R, min))
        return true;
      if(_intersectAngleCircle(X, Y, R, min + range))
        return true;
      
      return false; // If we have no intersections, there is no way we could be inside the circle.
    }

    inline bool IntersectCircleSegment(T X, T Y, T Inner, T Outer, T Min, T Range)
    {
      X -= x;
      Y -= y;
      T D2 = X*X + Y*Y;
      T tr = Outer + outer;
      if(D2 > tr*tr) // If the two circles don't intersect, bail out immediately
        return false;
      if(_pointInside(X + cos(Min)*Inner, Y - sin(Min)*Inner, inner, outer, min, range))
        return true;
      if(_pointInside(-X + cos(min)*inner, -Y - sin(min)*inner, Inner, Outer, Min, Range))
        return true;

      // It is possible for coincidental circles to reach this point if they have a nonzero inner radius.
      if(bss::fSmall(X) && bss::fSmall(Y)) // If this true, both the angle range and radius range must intersect for the segments to intersect
        return inner <= Outer && Inner <= outer && min <= (Min + Range) && Min <= (min + range); 

      T out[2][2];
      if(_checkBothAngles(X, Y, _intersectCircle(outer, X, Y, Outer, out), out, min, range, Min, Range))
        return true;  // outer <-> Outer intersection

      if(inner > 0 && _checkBothAngles(X, Y, _intersectCircle(inner, X, Y, Outer, out), out, min, range, Min, Range))
        return true; // inner <-> Outer intersection

      if(Inner > 0 && _checkBothAngles(X, Y, _intersectCircle(Inner, X, Y, outer, out), out, min, range, Min, Range))
        return true; // Inner <-> outer intersection

      if(inner > 0 && Inner > 0 && _checkBothAngles(X, Y, _intersectCircle(inner, X, Y, Inner, out), out, min, range, Min, Range))
        return true; // inner <-> Inner intersection

      if(Range < (T)bss::PI_DOUBLE || range < (T)bss::PI_DOUBLE)
      {
        if(_rayRayIntersect(X, Y, Inner, Outer, Min, inner, outer, min))
          return true; // Check minarc line segment intersection with other minarc line
        if(_rayRayIntersect(X, Y, Inner, Outer, Min + Range, inner, outer, min))
          return true; // Check minarc line segment intersection with other maxarc line
        if(_rayRayIntersect(X, Y, Inner, Outer, Min, inner, outer, min + range))
          return true; // Check maxarc line segment intersection with other minarc line
        if(_rayRayIntersect(X, Y, Inner, Outer, Min + Range, inner, outer, min + range))
          return true; // Check maxarc line segment intersection with other maxarc line

        if(_intersectRayCircle(X, Y, Outer, inner, outer, min, Min, Range))
          return true; // Check if our minarc ray intersects the other outer radius inside the arc
        if(_intersectRayCircle(X, Y, Outer, inner, outer, min + range, Min, Range))
          return true; // Check if our maxarc ray intersects the other outer radius inside the arc
        if(_intersectRayCircle(-X, -Y, outer, Inner, Outer, Min, min, range))
          return true; // Check if the other minarc ray intersects our outer radius inside the arc
        if(_intersectRayCircle(-X, -Y, outer, Inner, Outer, Min + Range, min, range))
          return true; // Check if the other maxarc ray intersects our outer radius inside the arc
         
        if(Inner > 0)
        {
          if(_intersectRayCircle(X, Y, Inner, inner, outer, min, Min, Range))
            return true; // Check if our minarc ray intersects the other inner radius inside the arc
          if(_intersectRayCircle(X, Y, Inner, inner, outer, min + range, Min, Range))
            return true; // Check if our maxarc ray intersects the other inner radius inside the arc
        }
        if(inner > 0)
        {
          if(_intersectRayCircle(-X, -Y, inner, Inner, Outer, Min, min, range))
            return true; // Check if the other minarc ray intersects our inner radius inside the arc
          if(_intersectRayCircle(-X, -Y, inner, Inner, Outer, Min + Range, min, range))
            return true; // Check if the other maxarc ray intersects our inner radius inside the arc
        }
      }

      return false; // If we had no valid intersection points, we don't intersect
    }

    // Returns true if point is inside or on the border of the circle segment
    inline bool ContainsPoint(T X, T Y) { return _pointInside(X - x, Y - y, inner, outer, min, range); }

    union {
      struct {
        VEC pos;
      };
      struct {
        T x;
        T y;
      };
    };
    T inner;
    T outer;
    T min; // Must be in the range [0,2PI]
    T range; // Must be positive

    //protected:
      BSS_FORCEINLINE static bool _intersectRayCircle(T X, T Y, T R, T inner, T outer, T angle, T Min, T Range)
      {
        T out[2][2];
        T c = cos(angle);
        T s = sin(angle);
        return _checkAngles(_lineSegmentRadiusIntersect((c * inner) - X, - (s * inner) - Y, (c * outer) - X, - (s * outer) - Y, R, out), out, Min, Range);
      }
      
      BSS_FORCEINLINE bool _intersectAngleCircle(T X, T Y, T R, T angle)
      {
        T c = cos(angle);
        T s = sin(angle);
        T x1 = (c * inner) - X;
        T y1 = -(s * inner) - Y;
        if(_lineSegmentRadiusIntersect(x1, y1, (c * outer) - X, -(s * outer) - Y, R))
          return true;
        return x1*x1 + y1*y1 < R*R; // check for containment
      }
      // Assumes an already centered line and does not check for containment - used in polygon testing
      inline bool _intersectLine(T X1, T Y1, T X2, T Y2)
      {
        T out[2][2]; // Get intersection points of line for the circle and see if they lie inside the arc
        if(_checkAngles(_lineSegmentRadiusIntersect(X1, Y1, X2, Y2, outer, out), out, min, range))
          return true;

        if(inner > 0 && _checkAngles(_lineSegmentRadiusIntersect(X1, Y1, X2, Y2, inner, out), out, min, range))
          return true;

        // Check line segment intersections with the angle boundaries
        if(_lineRayIntersect(X1, Y1, X2, Y2, inner, outer, min))
          return true;

        if(_lineRayIntersect(X1, Y1, X2, Y2, inner, outer, min + range))
          return true;

        return false;
      }
      static BSS_FORCEINLINE bool _pointInside(T X, T Y, T inner, T outer, T min, T range)
      {
        return _pointInRadius(X, Y, inner, outer) && _pointInAngle(X, Y, min, range);
      }
      static BSS_FORCEINLINE bool _pointInRadius(T X, T Y, T inner, T outer)
      {
        T dist = X*X + Y*Y;
        return (dist <= outer*outer) && (dist >= inner*inner);
      }

      static BSS_FORCEINLINE bool _pointInAngle(T X, T Y, T min, T range)
      {
        if(range >= (T)bss::PI_DOUBLE)
          return true;

        range *= 0.5;
        T angle = atan2(-Y, X);
        return bss::AngleDist<T>(angle, min + range) <= range;
      }
      static BSS_FORCEINLINE bool _checkBothAngles(T X, T Y, int n, const T(&in)[2][2], T min, T range, T Min, T Range)
      {
        for(int i = 0; i < n; ++i)
          if(_pointInAngle(in[i][0], in[i][1], min, range) && _pointInAngle(in[i][0] - X, in[i][1] - Y, Min, Range))
            return true;
        return false;
      }

      static BSS_FORCEINLINE bool _checkAngles(int n, const T(&in)[2][2], T min, T range)
      {
        for(int i = 0; i < n; ++i)
          if(_pointInAngle(in[i][0], in[i][1], min, range))
            return true;
        return false;
      }

      static BSS_FORCEINLINE bool _checkRadii(int n, const T(&in)[2][2], T inner, T outer)
      {
        for(int i = 0; i < n; ++i)
          if(_pointInRadius(in[i][0], in[i][1], inner, outer))
            return true;
        return false;
      }
      // Gets the intersection points of a circle at the origin and a circle at (X,Y)
      static inline int _intersectCircle(T r, T X, T Y, T R, T(&out)[2][2])
      {
        T dsq = (X*X) + (Y*Y);
        T maxr = r + R;
        T minr = fabs(r - R);

        if(dsq > maxr*maxr)
          return 0; // circles are disjoint
        if(dsq < minr*minr)
          return 0; // One circle is inside the other

        T id = ((T)1) / bss::FastSqrt(dsq);
        T a = (r*r - (R*R) + dsq)*(id/2);
        T h = bss::FastSqrt(r*r - (a*a));

        T x2 = X*a*id;
        T y2 = Y*a*id;

        out[0][0] = x2 + h*Y*id;
        out[0][1] = y2 - h*X*id;
        out[1][0] = x2 - h*Y*id;
        out[1][1] = y2 + h*X*id;
        return 2;
      }
  };

  typedef psCircleSegmentT<float> psCircleSegment; //default typedef
  typedef psCircleSegmentT<int> psCircleSegmenti;
  typedef psCircleSegmentT<double> psCircleSegmentd;
  typedef psCircleSegmentT<short> psCircleSegments;
  typedef psCircleSegmentT<long> psCircleSegmentl;
  typedef psCircleSegmentT<uint32_t> psCircleSegmentiu;
  typedef psCircleSegmentT<uint16_t> psCircleSegmentsu;
  typedef psCircleSegmentT<unsigned long> psCircleSegmentlu;
}

#endif