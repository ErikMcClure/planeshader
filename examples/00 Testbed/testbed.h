// Copyright ©2017 Black Sphere Studios

#ifndef __TESTBED_H__
#define __TESTBED_H__

#define _CRT_SECURE_NO_WARNINGS

#include "psEngine.h"
#include "psColor.h"
#include "psCamera.h"
#include "bss-util/cHighPrecisionTimer.h"
#include "bss-util/lockless.h"
#include "bss-util/cStr.h"

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
#define TESTARRAY(t,f) _ITERFUNC(__testret,t,[&](uint32_t i) -> bool { f });
#define TESTALL(t,f) _ITERALL(__testret,t,[&](uint32_t i) -> bool { f });
#define TESTCOUNT(c,t) { for(uint32_t i = 0; i < c; ++i) TEST(t) }
#define TESTCOUNTALL(c,t) { bool __val=true; for(uint32_t i = 0; i < c; ++i) __val=__val&&(t); TEST(__val); }
#define TESTFOUR(s,a,b,c,d) TEST(((s)[0]==(a)) && ((s)[1]==(b)) && ((s)[2]==(c)) && ((s)[3]==(d)))
#define TESTALLFOUR(s,a) TEST(((s)[0]==(a)) && ((s)[1]==(a)) && ((s)[2]==(a)) && ((s)[3]==(a)))
#define TESTRELFOUR(s,a,b,c,d) TEST(fcompare((s)[0],(a)) && fcompare((s)[1],(b)) && fcompare((s)[2],(c)) && fcompare((s)[3],(d)))
#define TESTVEC(v,cx,cy) TEST((v).x==(cx)) TEST((v).y==(cy))
#define TESTVEC3(v,cx,cy,cz) TEST((v).x==(cx)) TEST((v).y==(cy)) TEST((v).z==(cz))

template<class T, size_t SIZE, class F>
static void _ITERFUNC(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { for(uint32_t i = 0; i < SIZE; ++i) TEST(f(i)) }
template<class T, size_t SIZE, class F>
static void _ITERALL(TESTDEF::RETPAIR& __testret, T(&t)[SIZE], F f) { bool __val = true; for(uint32_t i = 0; i < SIZE; ++i) __val = __val && (f(i)); TEST(__val); }

extern bss_util::cLog _failedtests;
extern planeshader::psCamera globalcam;
extern planeshader::psEngine* engine;
extern bool gotonext;

extern bool comparevec(planeshader::psVec a, planeshader::psVec b, int diff = 1);
extern bool comparevec(planeshader::psColor& a, planeshader::psColor& b, int diff = 1);
extern cStr ReadFile(const char* path);
extern void processGUI();
extern void updatefpscount(uint64_t& timer, int& fps);

extern TESTDEF::RETPAIR test_feather();
extern TESTDEF::RETPAIR test_psDirectX11();
extern TESTDEF::RETPAIR test_psEffect();
extern TESTDEF::RETPAIR test_psFont();
extern TESTDEF::RETPAIR test_psPass();
extern TESTDEF::RETPAIR test_psTileset();
extern TESTDEF::RETPAIR test_psVector();
extern TESTDEF::RETPAIR test_psParticles();
extern TESTDEF::RETPAIR test_PBR_System();

#endif