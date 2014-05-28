// Example 00 - Testbed
// -------------------------
// This example runs a series of verification tests to ensure Planeshader is working properly.
//
// Copyright ©2014 Black Sphere Studios

#define _CRT_SECURE_NO_WARNINGS

#include "psEngine.h"
#include "bss-util/bss_win32_includes.h"
#include "bss-util/lockless.h"
#include <time.h>

using namespace planeshader;
using namespace bss_util;

cLog _failedtests("../bin/failedtests.txt"); //This is spawned too early for us to save it with SetWorkDirToCur();
psEngine* engine=0;

#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader64_d.lib")
#pragma comment(lib, "../../lib/bss-util64_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../../bin/PlaneShader64.lib")
#pragma comment(lib, "../../lib/bss-util64.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../../bin/PlaneShader_d.lib")
#pragma comment(lib, "../../lib/bss-util_d.lib")
#else
#pragma comment(lib, "../../bin/PlaneShader.lib")
#pragma comment(lib, "../../lib/bss-util.lib")
#endif

struct TESTDEF
{
  typedef std::pair<size_t, size_t> RETPAIR;
  const char* NAME;
  RETPAIR(*FUNC)();
};

#define BEGINTEST TESTDEF::RETPAIR __testret(0,0)
#define ENDTEST return __testret
#define FAILEDTEST(t) BSSLOG(_failedtests,1) << "Test #" << __testret.first << " Failed  < " << MAKESTRING(t) << " >" << std::endl
#define TEST(t) { atomic_xadd(&__testret.first); try { if(t) atomic_xadd(&__testret.second); else FAILEDTEST(t); } catch(...) { FAILEDTEST(t); } }
#define TESTERROR(t, e) { atomic_xadd(&__testret.first); try { (t); FAILEDTEST(t); } catch(e) { atomic_xadd(&__testret.second); } }
#define TESTERR(t) TESTERROR(t,...)
#define TESTNOERROR(t) { atomic_xadd(&__testret.first); try { (t); atomic_xadd(&__testret.second); } catch(...) { FAILEDTEST(t); } }
#define TESTARRAY(t,f) _ITERFUNC(__testret,t,[&](uint i) -> bool { f });
#define TESTALL(t,f) _ITERALL(__testret,t,[&](uint i) -> bool { f });
#define TESTCOUNT(c,t) { for(uint i = 0; i < c; ++i) TEST(t) }
#define TESTCOUNTALL(c,t) { bool __val=true; for(uint i = 0; i < c; ++i) __val=__val&&(t); TEST(__val); }
#define TESTFOUR(s,a,b,c,d) TEST(((s)[0]==(a)) && ((s)[1]==(b)) && ((s)[2]==(c)) && ((s)[3]==(d)))
#define TESTALLFOUR(s,a) TEST(((s)[0]==(a)) && ((s)[1]==(a)) && ((s)[2]==(a)) && ((s)[3]==(a)))
#define TESTRELFOUR(s,a,b,c,d) TEST(fcompare((s)[0],(a)) && fcompare((s)[1],(b)) && fcompare((s)[2],(c)) && fcompare((s)[3],(d)))
#define TESTVEC(v,cx,cy) TEST((v).x==(cx)) TEST((v).y==(cy))
#define TESTVEC3(v,cx,cy,cz) TEST((v).x==(cx)) TEST((v).y==(cy)) TEST((v).z==(cz))

template<class T, size_t SIZE, class F>
void _ITERFUNC(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { for(uint i = 0; i < SIZE; ++i) TEST(f(i)) }
template<class T, size_t SIZE, class F>
void _ITERALL(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { bool __val=true; for(uint i = 0; i < SIZE; ++i) __val=__val&&(f(i)); TEST(__val); }

bool comparevec(psVec a, psVec b, int diff=1)
{
  return b.x==0.0f?fsmall(a.x):fcompare(a.x, b.x, diff) && b.y==0.0f?fsmall(a.y):fcompare(a.y, b.y, diff);
}
TESTDEF::RETPAIR test_psVec()
{
  BEGINTEST;

  psVeci a(3);
  TESTVEC(a, 3, 3);
  psVeci b(2, 3);
  TESTVEC(b, 2, 3);
  psVec c(4.0f);
  TESTVEC(c, 4.0f, 4.0f)
    psVec d(b);
  TESTVEC(d, 2.0f, 3.0f);
  float ee[2] ={ -1.0f, 1.0f };
  psVec e(ee);
  TESTVEC(e, -1.0f, 1.0f);
  TEST(psVeci(2, 2).IntersectCircle(psVeci(1, 2), 1));
  TEST(!psVeci(2, 2).IntersectCircle(psVeci(1, 1), 1));
  TEST(psVec(2, 2).IntersectCircle(psVec(1.5f, 1.5f), 2));
  TEST(!psVec(2, 2).IntersectCircle(psVec(1.5f, 1.5f), 0.1f));
  float rct[4] ={ -1.0f, 0.0f, 1.0f, 2.0f };
  TEST(!psVec(2, 2).IntersectRect(rct));
  TEST(psVec(2, 2).IntersectRect(rct)==psVec(2, 2).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(!psVec(1, 2).IntersectRect(rct)); // Test to make sure our rectangle intersection is inclusive-exclusive.
  TEST(psVec(1, 2).IntersectRect(rct)==psVec(1, 2).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(-1, 0).IntersectRect(rct));
  TEST(psVec(-1, 0).IntersectRect(rct)==psVec(-1, 0).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(0, 1).IntersectRect(rct));
  TEST(psVec(0, 1).IntersectRect(rct)==psVec(0, 1).IntersectRect(-1.0f, 0.0f, 1.0f, 2.0f));
  TEST(psVec(2, 2).IntersectEllipse(psVec(1.5f, 1.5f), 2, 4));
  TEST(!psVec(2, 2).IntersectEllipse(psVec(1.5f, 1.5f), 0.1f, 0.2f));
  TEST(psVec(2, 2).IntersectEllipse(1.5f, 1.5f, 2, 4));
  TEST(!psVec(2, 2).IntersectEllipse(1.5f, 1.5f, 0.1f, 0.2f));
  TEST(psVec(2, 2).DistanceSquared(0, 0)==8.0f);
  TEST(psVec(2, 2).DistanceSquared(psVec(1, 1))==2.0f);
  TEST(psVec(-2, -2).DistanceSquared(1, 1)==18.0f);
  TEST(psVec(-2, -1).DistanceSquared(psVec(1, 0))==10.0f);
  TEST(fcompare(psVec(2, 2).Distance(psVec(0, 0)), 2.82843f ,100));
  TEST(fcompare(psVec(2, 2).Distance(1, 1), 1.4142136f, 100));
  TEST(fcompare(psVec(-2, -2).Distance(psVec(1, 1)), 4.24264f, 100));
  TEST(fcompare(psVec(-2, -1).Distance(1, 0), 3.16228f, 100));
  TEST(psVec(2, 2).DotProduct(3, 3)==12.0f);
  TEST(psVec(2, 2).DotProduct(psVec(3, 3))==12.0f);
  TEST(psVec(2, 2).DotProduct(0, 0)==0.0f);
  TEST(psVec(0, 0).DotProduct(psVec(3, 3))==0.0f);
  TEST(psVec(1, 2).CrossProduct(3, 4)==-2.0f);
  TEST(psVec(3, 4).CrossProduct(psVec(1, 2))==2.0f);
  TEST(psVec(0, 0).Length()==0.0f);
  TEST(fcompare(psVec(1, 1).Length(), 1.41421f, 100));
  TEST(fcompare(psVec(2, 4).Length(), 4.47214f, 100));
  TEST(psVec(1, 1)==psVec(1, 1));
  TEST(!(psVec(1, 1)==psVec(2, 1)));
  TEST(!(psVec(2, 2)==psVec(1, 1)));
  TEST(!(psVec(1, 1)!=psVec(1, 1)));
  TEST(psVec(1, 1)!=psVec(2, 1));
  TEST(psVec(2, 2)!=psVec(1, 1));
  TEST(psVec(1, 1)==1.0f);
  TEST(psVec(1, 1)!=2.0f);
  TEST(psVec(2, 1)!=2.0f);
  TEST(!(psVec(1, 1)==2.0f));
  TEST(!(psVec(2, 1)==1.0f));
  TEST(psVec(2, 2)==2.0f);
  //TESTERROR(psVec(0,0).Normalize(),...);
  psVec na(1, 1);
  psVec nb(2, 2);
  na=na.Normalize();
  nb=nb.Normalize();
  TEST(na==nb);
  TEST(fcompare(na.x, 0.707107f, 100) && fcompare(na.y, 0.707107f, 100));
  psVec nc(1, 4);
  nc=nc.Normalize();
  TEST(fcompare(nc.x, 0.242536f, 100) && fcompare(nc.y, 0.970143f, 100));
  TEST(comparevec(psVec(0, 4).Normalize(), psVec(0,1), 100));
  TEST(psVec(-2, -2).Abs()==psVec(2, 2));
  TEST(psVec(2, -2).Abs()==psVec(2, 2));
  TEST(psVec(2, 2).Abs()==psVec(2, 2));
  TEST(comparevec(psVec(2, 3).Rotate(0, 0, 0),psVec(2, 3),100));
  TEST(comparevec(psVec(2, 3).Rotate(PI_DOUBLEf, psVec(1, 1)), psVec(2, 3), 100));
  TEST(comparevec(psVec(2, 3).Rotate(PIf, 0, 0), psVec(-2, -3), 100));
  TEST(comparevec(psVec(2, 3).Rotate(PIf, psVec(1, 1)), psVec(-1, -1), 100));
  float mm[4] ={ cos(PIf), sin(PIf), -sin(PIf), cos(PIf) };
  TEST(comparevec(psVec(2, 3).MatrixMultiply(mm), psVec(-2, -3), 100));
  TEST(comparevec((psVec(1, 1)+psVec(2, 2)-(psVec(3, 1)*((-psVec(-2, 2))/psVec(-1, 2)))), psVec(9, 4), 100));
  TEST(comparevec(((((psVec(1, 1)+5.0f)+psVec(0, 0)-3.0f+psVec(0, 0))/5.0f)*2.0f), psVec(24.0f/20.0f), 100));
  TEST((psVec(1, 1)+=psVec(1, 1))==psVec(2, 2));
  TEST((psVec(1, 1)-=psVec(-1, 1))==psVec(2, 0));
  TEST((psVec(1, 1)*=psVec(-1, 2))==psVec(-1, 2));
  TEST((psVec(1, 4)/=psVec(-1, 2))==psVec(-1, 2));
  TEST((psVec(1, 1)+=1.0f)==psVec(2, 2));
  TEST((psVec(1, 1)-=1.0f)==psVec(0, 0));
  TEST((psVec(1, 2)*=-2.0f)==psVec(-2, -4));
  TEST((psVec(4, 2)/=2.0f)==psVec(2, 1));
  TEST(psVec(2, 2)<psVec(3, 3));
  TEST(psVec(2, 3)<psVec(3, 3));
  TEST(!(psVec(3, 3)<psVec(3, 3)));
  TEST(psVec(3, 3)>=psVec(3, 3));
  TEST(psVec(3, 4)>=psVec(3, 3));
  TEST(psVec(3, 3)>psVec(2, 3));
  TEST(psVec(3, 3)>psVec(2, 2));
  TEST(!(psVec(3, 3)>psVec(3, 3)));
  TEST(psVec(3, 3)<=psVec(3, 3));
  TEST(psVec(3, 3)<=psVec(3, 4));
  TEST(psVec(3, 3)<=3);
  TEST(!(psVec(4, 3)>3));
  TEST(psVec(4, 4)>3);
  TEST(psVec(2, 2)<3);
  TEST(!(psVec(2, 2)>=3));
  TEST(psVec(2, 2)>=2);
  TEST(psVec(2, 2)>=1);
  psVec f(3, 3);
  f=psVeci(2, 2);
  TEST(f==psVec(2, 2));
  f=psVec(2.5f, 2.5f);
  TEST(f==psVec(2.5f, 2.5f));
  psVeci g(2, 2);
  g=f;
  TEST(g==psVeci(2, 2));
  psVec ln(2, 4);
  ln=ln.Normalize();
  psVec la(1, 3);
  psVec lp(2, -3);
  psVec dd = (la - lp) - (ln*((la-lp).DotProduct(ln)));
  TEST(lp.LineInfDistanceSqr(1, 3, 1+ln.x, 3+ln.y)==dd.DotProduct(dd));
  TEST(fcompare(lp.LineInfDistance(1, 3, 1+ln.x, 3+ln.y),dd.Length(),100));
  TEST(lp.LineDistanceSqr(1, 3, 1+ln.x, 3+ln.y)==bss_util::distsqr(1, 3, 2, -3));
  TEST(fcompare(lp.LineDistance(1, 3, 1+ln.x, 3+ln.y),bss_util::dist<float>(1, 3, 2, -3),100));

  /*
  static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL FromPolar(const psVecT<T>& v) { return FromPolar(v.x,v.y); }
  static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL FromPolar(T r, T angle) { return psVecT<T>(r*cos(angle),r*sin(angle)); }
  static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL ToPolar(const psVecT<T>& v) { return ToPolar(v.x,v.y); }
  static BSS_FORCEINLINE const psVecT<T> BSS_FASTCALL ToPolar(T x, T y) { return psVecT<T>(bss_util::FastSqrt((x*x) + (y*y)),atan2(y,x)); } //x - r, y - theta
  static BSS_FORCEINLINE bool BSS_FASTCALL IntersectEllipse(T X, T Y, T A, T B, T x, T y) { T tx=X-x; T ty=Y-y; return ((tx*tx)/(A*A)) + ((ty*ty)/(B*B)) <= 1; }
  static BSS_FORCEINLINE T BSS_FASTCALL PointLineInfDistSqr(T X, T Y, T X1, T Y1, T X2, T Y2) {  T tx=X2-X1; T ty=Y2-Y1; T det=(tx*(Y1-Y)) - ((X1-X)*ty); return (det*det)/((tx*tx)+(ty*ty)); }
  static BSS_FORCEINLINE T BSS_FASTCALL PointLineInfDist(T X, T Y, T X1, T Y1, T X2, T Y2) {  T tx=X2-X1; T ty=Y2-Y1; return ((tx*(Y1-Y)) - ((X1-X)*ty))/bss_util::FastSqrt((tx*tx)+(ty*ty)); }
  static inline void BSS_FASTCALL NearestPointToLineInf(T X, T Y, T X1, T Y1, T X2, T Y2, T& outX, T& outY) //treated as infinite line
  static inline void BSS_FASTCALL NearestPointToLine(T X, T Y, T X1, T Y1, T X2, T Y2, T& outX, T& outY) //treated as line segment
  static inline void BSS_FASTCALL EllipseNearestPoint(T A, T B, T cx, T cy, T& outX, T& outY)*/

  ENDTEST;
}

TESTDEF::RETPAIR test_psVec3D()
{
  BEGINTEST;

  psVec3Di a(3);
  TESTVEC3(a, 3, 3, 3);
  psVec3Di b(1, 2, 3);
  TESTVEC3(b, 1, 2, 3);
  psVec3D c(4.0f);
  TESTVEC3(c, 4.0f, 4.0f, 4.0f)
    psVec3D d(b);
  TESTVEC3(d, 1.0f, 2.0f, 3.0f);
  float ee[3] ={ -1.0f, 1.0f, 2.0f };
  psVec3D e(ee);
  TESTVEC3(e, -1.0f, 1.0f, 2.0f);
  /*
  inline bool BSS_FASTCALL Equals(T X, T Y, T Z) const { return X == x && Y == y && Z==z; }
  inline T BSS_FASTCALL GetDistance(const psVec3DT<T>& other) const { T tz = (other.z - z);  T ty = (other.y - y); T tx = (other.x - x); return (T)bss_util::FastSqrt((tx*tx)+(ty*ty)+(tz*tz)); }
  inline T BSS_FASTCALL GetDistanceSquared(const psVec3DT<T>& other) const { T tz = (other.z - z); T ty = (other.y - y); T tx = (other.x - x); return (T)((tx*tx)+(ty*ty)+(tz*tz)); }
  inline T BSS_FASTCALL GetDistance(T X, T Y, T Z) const { T tz = (other.z - z); T ty = (Y - y); T tx = (X - x); return (T)bss_util::FastSqrt((tx*tx)+(ty*ty)+(tz*tz)); }
  inline T BSS_FASTCALL GetDistanceSquared(T X, T Y, T Z) const { T tz = (other.z - z); T ty = (Y - y); T tx = (X - x); return (T)((tx*tx)+(ty*ty)+(tz*tz)); }
  inline T GetLength3D() const { return bss_util::FastSqrt((x*x)+(y*y)+(z*z)); }
  inline psVec3DT<T>& Normalize3D() { T length = GetLength3D(); x=x/length; y=y/length; z=z/length; return *this; }
  inline psVec3DT<T> Abs3D() const { return psVec3DT<T>(abs(x),abs(y),abs(z)); }
  inline psVec3DT<T> CrossProduct3D(const psVec3DT<T>& other) const { return CrossProduct3D(other.x,other.y,other.z); }
  inline psVec3DT<T> CrossProduct3D(T X, T Y, T Z) const { return psVec3DT<T>(y*Z - z*Y,z*X - x*Z,x*Y - X*y); }
  inline float DotProduct(const psVec3DT<T>& other) const { return x * other.x + y * other.y + z * other.z; }
  inline psVec3DT<T> BSS_FASTCALL MatrixMultiply(const T (&m)[9]) { return psVec3DT<T>((x*m[0])+(y*m[1])+(z*m[2]),(x*m[3])+(y*m[4])+(z*m[5]),(x*m[6])+(y*m[7])+(z*m[8])); }
  inline T BSS_FASTCALL GetZ() const { return z; }
  inline psVecT<T>& Get2D() { return *this; }
  inline const psVecT<T>& Get2D() const { return *this; }

  inline const psVec3DT<T> BSS_FASTCALL operator +(const psVec3DT<T>& other) const { return psVec3DT<T>(x + other.x, y + other.y, z + other.z); }
  inline const psVec3DT<T> BSS_FASTCALL operator -(const psVec3DT<T>& other) const { return psVec3DT<T>(x - other.x, y - other.y, z - other.z); }
  inline const psVec3DT<T> BSS_FASTCALL operator *(const psVec3DT<T>& other) const { return psVec3DT<T>(x * other.x, y * other.y, z * other.z); }
  inline const psVec3DT<T> BSS_FASTCALL operator /(const psVec3DT<T>& other) const { return psVec3DT<T>(x / other.x, y / other.y, z / other.z); }
  inline const psVec3DT<T> BSS_FASTCALL operator +(const psVecT<T>& other) const { return psVec3DT<T>(x + other.x, y + other.y, z); }
  inline const psVec3DT<T> BSS_FASTCALL operator -(const psVecT<T>& other) const { return psVec3DT<T>(x - other.x, y - other.y, z); }
  inline const psVec3DT<T> BSS_FASTCALL operator *(const psVecT<T>& other) const { return psVec3DT<T>(x * other.x, y * other.y, z); }
  inline const psVec3DT<T> BSS_FASTCALL operator /(const psVecT<T>& other) const { return psVec3DT<T>(x / other.x, y / other.y, z); }
  //inline const psVec3DT<T> BSS_FASTCALL operator +(const T other) const { return psVec3DT<T>(x + other, y + other, z + other); }
  //inline const psVec3DT<T> BSS_FASTCALL operator -(const T other) const { return psVec3DT<T>(x - other, y - other, z - other); }
  //inline const psVec3DT<T> BSS_FASTCALL operator *(const T other) const { return psVec3DT<T>(x * other, y * other, z * other); }
  //inline const psVec3DT<T> BSS_FASTCALL operator /(const T other) const { return psVec3DT<T>(x / other, y / other, z / other); }
  //all const T functions are disabled to prevent mixups.

  inline psVec3DT<T>& BSS_FASTCALL operator +=(const psVec3DT<T> &other) { x += other.x; y += other.y; z += other.z; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator -=(const psVec3DT<T> &other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator *=(const psVec3DT<T> &other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator /=(const psVec3DT<T> &other) { x /= other.x; y /= other.y; z /= other.z; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator +=(const psVecT<T> &other) { x += other.x; y += other.y; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator -=(const psVecT<T> &other) { x -= other.x; y -= other.y; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator *=(const psVecT<T> &other) { x *= other.x; y *= other.y; return *this; }
  inline psVec3DT<T>& BSS_FASTCALL operator /=(const psVecT<T> &other) { x /= other.x; y /= other.y; return *this; }
  //inline psVec3DT<T>& BSS_FASTCALL operator +=(const T other) { x += other; y += other; z += other; return *this; }
  //inline psVec3DT<T>& BSS_FASTCALL operator -=(const T other) { x -= other; y -= other; z -= other; return *this; }
  //inline psVec3DT<T>& BSS_FASTCALL operator *=(const T other) { x *= other; y *= other; z *= other; return *this; }
  //inline psVec3DT<T>& BSS_FASTCALL operator /=(const T other) { x /= other; y /= other; z /= other; return *this; }

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

  inline psVec3DT<T>& BSS_FASTCALL operator=(const psVec3DT<T>& other) { x=other.x;y=other.y;z=other.z; return *this; }
  template<class U>
  inline psVec3DT<T>& BSS_FASTCALL operator=(const psVec3DT<U>& other) { x=(T)other.x;y=(T)other.y;z=(T)other.z; return *this; }

  */
  ENDTEST;
}

TESTDEF::RETPAIR test_psCircle()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psRect()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psCamera()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX10()
{
  BEGINTEST;
  engine->Begin();


  engine->End();



  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX9()
{
  BEGINTEST;
  ENDTEST;
}

TESTDEF::RETPAIR test_psDirectX11()
{
  BEGINTEST;

  ENDTEST;
}

TESTDEF::RETPAIR test_psRenderable()
{
  BEGINTEST;

  ENDTEST;
}

// Main program function
int main(int argc, char** argv)
{
  bss_util::ForceWin64Crash();
  SetWorkDirToCur();
  srand(time(NULL));
  AllocConsole();
  freopen("CONOUT$", "wb", stdout);
  freopen("CONIN$", "rb", stdin);

  TESTDEF tests[] ={
    { "psVec", &test_psVec },
    { "psVec3D", &test_psVec3D },
  };

  const size_t NUMTESTS=sizeof(tests)/sizeof(TESTDEF);

  std::cout << "Black Sphere Studios - PlaneShader v" << (uint)PS_VERSION_MAJOR << '.' << (uint)PS_VERSION_MINOR << '.' <<
    (uint)PS_VERSION_REVISION << ": Unit Tests\nCopyright (c)2014 Black Sphere Studios\n" << std::endl;
  const int COLUMNS[3] ={ 24, 11, 8 };
  printf("%-*s %-*s %-*s\n", COLUMNS[0], "Test Name", COLUMNS[1], "Subtests", COLUMNS[2], "Pass/Fail");


  std::vector<uint> failures;
  PSINIT init;
  init.driver=RealDriver::DRIVERTYPE_DX10;
  init.width=320;
  init.height=240;
  //init.iconresource=101;
  //init.filter=5;
  {
    psEngine ps(init);
    if(ps.GetQuit()) return 0;
    //ps[0].SetClear(true, 0);
    engine=&ps;

    TESTDEF::RETPAIR numpassed;
    for(uint i = 0; i < NUMTESTS; ++i)
    {
      numpassed=tests[i].FUNC(); //First is total, second is succeeded
      if(numpassed.first!=numpassed.second) failures.push_back(i);

      printf("%-*s %*s %-*s\n", COLUMNS[0], tests[i].NAME, COLUMNS[1], cStrF("%u/%u", numpassed.second, numpassed.first).c_str(), COLUMNS[2], (numpassed.first==numpassed.second)?"PASS":"FAIL");
    }
  }

  if(failures.empty())
    std::cout << "\nAll tests passed successfully!" << std::endl;
  else
  {
    std::cout << "\nThe following tests failed: " << std::endl;
    for(uint i = 0; i < failures.size(); i++)
      std::cout << "  " << tests[failures[i]].NAME << std::endl;
    std::cout << "\nThese failures indicate either a misconfiguration on your system, or a potential bug. \n\nA detailed list of failed tests was written to failedtests.txt" << std::endl;
  }

  std::cout << "\nPress Enter to exit the program." << std::endl;
  std::cin.get();
  fclose(stdout);
  fclose(stdin);
  FreeConsole();

  //end program
  return 0;
}

struct HINSTANCE__;

// WinMain function, simply a catcher that calls the main function
int __stdcall WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int nShowCmd)
{
  main(0, (char**)hInstance);
}