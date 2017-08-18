// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psColored.h"

using namespace planeshader;

psColored::psColored(const psColored& copy) : _color(copy._color) {}
psColored::psColored(uint32_t color) : _color(color) {}
psColored::~psColored() {}
void psColored::SetColor(uint32_t color) { _color = color; }