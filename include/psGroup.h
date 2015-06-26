// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __GROUP_H__PS__
#define __GROUP_H__PS__

#include "psInheritable.h"
#include "bss-util/cHash.h"

namespace planeshader {
  struct DEF_GROUP;

  // A group of inheritables that can be managed as one unit.
  class psGroup : public psInheritable
  {
  public:
    psGroup(const DEF_GROUP& def);
    psGroup(const psGroup& copy);
    psGroup(psGroup&& copy);
    explicit psGroup(const psVec3D& position=VEC3D_ZERO, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO, FLAG_TYPE flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, unsigned short pass=(unsigned short)-1, psInheritable* parent=0);
    virtual ~psGroup();
    // Clone function 
    inline virtual psGroup* BSS_FASTCALL Clone() const { return new psGroup(*this); }
    // Adds an inheritable by cloning and returns a pointer to the internally owned 
    psInheritable* BSS_FASTCALL AddClone(psInheritable* inheritable);
    psInheritable* BSS_FASTCALL AddDef(const DEF_INHERITABLE& inheritable);
    // Adds an inheritable as a child but does not retain ownership of it, so the inheritable won't be deleted if the group is deleted.
    void BSS_FASTCALL AddRef(psInheritable* inheritable);
    // Removes an inheritable
    bool BSS_FASTCALL Remove(psInheritable* inheritable);

  protected:
    virtual void _render();

    bss_util::cHash<psInheritable*, char, false> _list; // list of owned inheritables that will be deleted with the group
  };

  struct BSS_COMPILER_DLLEXPORT DEF_GROUP : DEF_INHERITABLE
  {
    inline DEF_GROUP() {}
    inline DEF_GROUP(const DEF_GROUP& copy) { operator=(copy); }
    inline virtual ~DEF_GROUP() { }
    inline virtual psGroup* BSS_FASTCALL Spawn() const { return new psGroup(*this); } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_GROUP* BSS_FASTCALL Clone() const { return new DEF_GROUP(*this); }
    inline size_t AddInheritable(const DEF_INHERITABLE& inheritable) { inheritables.push_back(std::unique_ptr<DEF_INHERITABLE>(inheritable.Clone())); return inheritables.size()-1; }
    inline bool RemoveInheritable(size_t index) { if(index>=inheritables.size()) return false; inheritables.erase(inheritables.begin()+index); return true; }
    inline const DEF_INHERITABLE* GetInheritable(size_t index) const { return (index<inheritables.size())?inheritables[index].get():0; }
    inline size_t NumInheritables() const { return inheritables.size(); }

    DEF_GROUP& operator =(const DEF_GROUP& right) { inheritables.clear(); size_t svar=right.inheritables.size(); for(size_t i = 0; i < svar; ++i) inheritables.push_back(std::unique_ptr<DEF_INHERITABLE>(right.inheritables[i]->Clone())); return *this; }
    const DEF_INHERITABLE* operator [](size_t index) const { return inheritables[index].get(); }

  protected:
    std::vector<std::unique_ptr<DEF_INHERITABLE>> inheritables;
  };
}

#endif