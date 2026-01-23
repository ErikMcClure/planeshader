// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_CHECKBOX_H__
#define __FG_CHECKBOX_H__

#include "fgControl.h"
#include "fgText.h"

#ifdef  __cplusplus
extern "C" {
#endif

// A checkbox is a toggleable control with text alongside it.
typedef struct _FG_CHECKBOX {
  fgControl control;
  fgText text; // text displayed
  char checked;
#ifdef  __cplusplus
  _FG_CHECKBOX(_FG_CHECKBOX&&) = default;
  _FG_CHECKBOX(const _FG_CHECKBOX&) = delete;
  _FG_CHECKBOX(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units);
  _FG_CHECKBOX(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units, fgDestroy destroy, fgMessage message);
  ~_FG_CHECKBOX();

  _FG_CHECKBOX& operator=(_FG_CHECKBOX&&) = default;
  _FG_CHECKBOX& operator=(const _FG_CHECKBOX&) = delete;
  inline operator fgElement*() { return &control.element; }
  inline fgElement* operator->() { return operator fgElement*(); }
#endif
} fgCheckbox;

FG_EXTERN void fgCheckbox_Init(fgCheckbox* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN void fgCheckbox_Destroy(fgCheckbox* self);
FG_EXTERN size_t fgCheckbox_Message(fgCheckbox* self, const FG_Msg* msg);

#ifdef  __cplusplus
}
#endif



#endif