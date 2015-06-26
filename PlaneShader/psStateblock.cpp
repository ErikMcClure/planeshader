// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psStateblock.h"
#include "psDriver.h"
#include "bss-util/profiler.h"
#include <stdarg.h>
#include <algorithm>

using namespace planeshader;

STATEINFO::BLOCKHASH STATEINFO::_blocks;
psStateblock* psStateblock::DEFAULT=0;

STATEINFO::STATEINFOS* STATEINFO::Exists(STATEINFOS* infos)
{
  khiter_t iter = _blocks.Iterator(infos);
  if(_blocks.ExistsIter(iter))
    return _blocks.GetKey(iter);
  _blocks.Insert(infos, 0);
  return 0;
}
psStateblock::psStateblock(const STATEINFO* infos, unsigned int numstates) : bss_util::cArray<STATEINFO, unsigned short>(numstates)
{
  memcpy(_array, infos, numstates*sizeof(STATEINFO));
  std::sort<STATEINFO*>(_array, _array+numstates, &STATEINFO::SILESS);
  _sb = psDriverHold::GetDriver()->CreateStateblock(_array);
}
psStateblock::~psStateblock()
{
  psDriverHold::GetDriver()->FreeResource(_sb, psDriver::RES_STATEBLOCK);
}
void psStateblock::DestroyThis() { delete this; }

psStateblock* BSS_FASTCALL psStateblock::Create(unsigned int numstates, ...)
{
  PROFILE_FUNC();
  DYNARRAY(STATEINFO, states, numstates);

  va_list vl;
  va_start(vl, numstates);
  for(unsigned int i = 0; i < numstates; ++i)
    states[i]=*va_arg(vl, const STATEINFO*);
  va_end(vl);

  return Create(states, numstates);
}
psStateblock* BSS_FASTCALL psStateblock::Create(const STATEINFO* infos, unsigned int numstates)
{
  PROFILE_FUNC();
  psStateblock* r = new psStateblock(infos, numstates);
  psStateblock* n;
  if(n = static_cast<psStateblock*>(STATEINFO::Exists(r)))
  {
    delete r;
    r = n;
  }
  r->Grab();
  return r;
}
psTexblock::psTexblock(const STATEINFO* infos, unsigned int numstates) : bss_util::cArray<STATEINFO, unsigned short>(numstates)
{
  memcpy(_array, infos, numstates*sizeof(STATEINFO));
  std::sort<STATEINFO*>(_array, _array+numstates, &STATEINFO::SILESS);
  _tb = psDriverHold::GetDriver()->CreateTexblock(_array);
}
psTexblock::~psTexblock()
{
  psDriverHold::GetDriver()->FreeResource(_tb, psDriver::RES_TEXBLOCK);
}
void psTexblock::DestroyThis() { delete this; }

psTexblock* BSS_FASTCALL psTexblock::Create(unsigned int numstates, ...)
{
  PROFILE_FUNC();
  DYNARRAY(STATEINFO, states, numstates);

  va_list vl;
  va_start(vl, numstates);
  for(unsigned int i = 0; i < numstates; ++i)
    states[i]=*va_arg(vl, const STATEINFO*);
  va_end(vl);

  return Create(states, numstates);
}
psTexblock* BSS_FASTCALL psTexblock::Create(const STATEINFO* infos, unsigned int numstates)
{
  PROFILE_FUNC();
  psTexblock* r = new psTexblock(infos, numstates);
  psTexblock* n;
  if(n = static_cast<psTexblock*>(STATEINFO::Exists(r)))
  {
    delete r;
    r = n;
  }
  r->Grab();
  return r;
}