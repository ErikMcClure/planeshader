// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psColored.h"

using namespace planeshader;

psColored::psColored(const psColored& copy) : _color(copy._color) {}
psColored::psColored(const DEF_COLORED& def) : _color(def.color) {}
psColored::psColored(unsigned int color) : _color(color) {}
psColored::~psColored() {}
void BSS_FASTCALL psColored::SetColor(unsigned int color) { _color = color; }

void* psColored::operator new(std::size_t sz){ return _aligned_malloc(sz, 16); }
void psColored::operator delete(void* ptr, std::size_t sz) { _aligned_free(ptr); }