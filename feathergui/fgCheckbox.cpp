// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#include "feathercpp.h"
#include "feathergui/fgCheckbox.h"
#include "feathergui/fgSkin.h"

_FG_CHECKBOX::_FG_CHECKBOX(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units)
  : _FG_CHECKBOX(parent, next, name, flags, transform, units, (fgDestroy)&fgCheckbox_Destroy, (fgMessage)&fgCheckbox_Message) {
}

_FG_CHECKBOX::_FG_CHECKBOX(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units, fgDestroy destroy, fgMessage message)
  : control(parent, next, name, flags, transform, units, destroy, message),
  text(*this, nullptr, "Checkbox$text", FGELEMENT_EXPAND | FGELEMENT_IGNORE | FGFLAGS_INTERNAL, fgTransform_CENTER, 0)
{
}

_FG_CHECKBOX::~_FG_CHECKBOX() {
  control->message = (fgMessage)fgControl_HoverMessage;
}


void fgCheckbox_Init(fgCheckbox* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units)
{
  new(self) fgCheckbox(parent, next, name, flags, into_optional(transform), units);
}

void fgCheckbox_Destroy(fgCheckbox* self)
{
  self->~_FG_CHECKBOX();
}

size_t fgCheckbox_Message(fgCheckbox* self, const FG_Msg* msg)
{
  assert(self != 0 && msg != 0);
  switch (msg->type)
  {
  case FG_CONSTRUCT:
    fgControl_HoverMessage(&self->control, msg);
    (*self)->SetStyle("unchecked");
    self->checked = FGCHECKED_NONE;
    return FG_ACCEPT;
  case FG_CLONE:
    if (msg->e)
    {
      fgCheckbox* hold = reinterpret_cast<fgCheckbox*>(msg->e);
      fgControl_ActionMessage(&self->control, msg);
      self->text->Clone(hold->text);
      _sendmsg<FG_ADDCHILD, fgElement*>(msg->e, hold->text);
      hold->checked = self->checked;
    }
    return sizeof(fgCheckbox);
  case FG_ACTION:
    _sendmsg<FG_SETVALUE, size_t>(*self, !_sendmsg<FG_GETVALUE>(*self));
    return FG_ACCEPT;
  case FG_SETVALUE:
    if (msg->subtype != 0 && msg->subtype != FGVALUE_INT64)
    {
      fgLog(FGLOG_INFO, "%s set invalid value type: %hu", fgGetFullName(*self).c_str(), msg->subtype);
      return 0;
    }
    self->checked = (char)msg->i;
    switch (self->checked)
    {
    case FGCHECKED_CHECKED: (*self)->SetStyle("checked"); break;
    case FGCHECKED_INDETERMINATE: (*self)->SetStyle("indeterminate"); break;
    default: (*self)->SetStyle("unchecked"); break;
    }
    return FG_ACCEPT;
  case FG_GETVALUE:
    if (!msg->subtype || msg->subtype == FGVALUE_INT64)
      return self->checked;
    fgLog(FGLOG_INFO, "%s requested invalid value type: %hu", fgGetFullName(*self).c_str(), msg->subtype);
    return 0;
  case FG_CLEAR:
    fgSendMessage(self->text, msg);
    break;
  case FG_SETASSET:
    return fgSendMessage(fgCreate("Resource", *self, 0, "Checkbox$Asset", FGELEMENT_EXPAND, &fgTransform_CENTER, 0), msg);
  case FG_SETTEXT:
  case FG_SETFONT:
  case FG_SETLINEHEIGHT:
  case FG_SETLETTERSPACING:
  case FG_SETCOLOR:
  case FG_GETTEXT:
  case FG_GETFONT:
  case FG_GETLINEHEIGHT:
  case FG_GETLETTERSPACING:
  case FG_GETCOLOR:
    return fgSendMessage(self->text, msg);
  case FG_GETCLASSNAME:
    return (size_t)"Checkbox";
  }
  return fgControl_ActionMessage(&self->control, msg);
}