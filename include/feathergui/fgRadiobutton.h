// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_RADIOBUTTON_H__
#define __FG_RADIOBUTTON_H__

#include "fgCheckbox.h"

#ifdef  __cplusplus
extern "C" {
#endif

// A radio button is like a checkbox, but disables all other radio buttons that have the same parent control (use a non-styled fgElement to group them).
typedef struct _FG_RADIOBUTTON {
  fgCheckbox window; // A radio button must be a different class for styling purposes.
  struct _FG_RADIOBUTTON* radionext; // Used for the list of radiobuttons in a given fgElement grouping
  struct _FG_RADIOBUTTON* radioprev;
#ifdef  __cplusplus
  _FG_RADIOBUTTON(_FG_RADIOBUTTON&&) = default;
  _FG_RADIOBUTTON(const _FG_RADIOBUTTON&) = delete;
  _FG_RADIOBUTTON(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units);
  ~_FG_RADIOBUTTON();

  _FG_RADIOBUTTON& operator=(_FG_RADIOBUTTON&&) = default;
  _FG_RADIOBUTTON& operator=(const _FG_RADIOBUTTON&) = delete;

  inline operator fgElement*() { return &window.control.element; }
  inline fgElement* operator->() { return operator fgElement*(); }
#endif
} fgRadiobutton;

FG_EXTERN void fgRadiobutton_Init(fgRadiobutton* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN void fgRadiobutton_Destroy(fgRadiobutton* self);
FG_EXTERN size_t fgRadiobutton_Message(fgRadiobutton* self, const FG_Msg* msg);

#ifdef  __cplusplus
}
#endif



#endif