// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psStateblock.h"
#include "psDriver.h"
#include <stdarg.h>
#include <algorithm>

using namespace planeshader;

psStateblock::BLOCKHASH psStateblock::_blocks;

psStateblock::psStateblock(const STATEINFO* infos, unsigned int numstates) : _infos(numstates)
{
  memcpy((STATEINFO*)_infos, infos, numstates*sizeof(STATEINFO));
  std::sort<STATEINFO*>((STATEINFO*)_infos, ((STATEINFO*)_infos)+numstates, &SBLESS);
  _sb = psDriverHold::GetDriver()->CreateStateblock(_infos);
}
psStateblock::~psStateblock()
{
  psDriverHold::GetDriver()->FreeResource(_sb, psDriver::RES_STATEBLOCK);
}
void psStateblock::DestroyThis() { delete this; }

psStateblock* BSS_FASTCALL psStateblock::Create(unsigned int numstates, ...)
{
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
  psStateblock* r = new psStateblock(infos, numstates);
  khiter_t iter = _blocks.Iterator(r);
  if(_blocks.ExistsIter(iter))
  {
    psStateblock* ret = _blocks.GetKey(iter);
    delete r;
    ret->Grab();
    return ret;
  }
  _blocks.Insert(r, 0);
  r->Grab();
  return r;
}