// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in testbed.cpp

#include "testbed.h"
#include "psTex.h"
#include "psFont.h"
#include "psLayer.h"
#include "feathergui/fgButton.h"
#include "feathergui/fgResource.h"
#include "feathergui/fgCurve.h"
#include "feathergui/fgWindow.h"
#include "feathergui/fgRadioButton.h"
#include "feathergui/fgProgressbar.h"
#include "feathergui/fgSlider.h"
#include "feathergui/fgList.h"
#include "feathergui/fgRoot.h"
#include "feathergui/fgTextbox.h"
#include "feathergui/fgTabControl.h"
#include "feathergui/fgDebug.h"
#include "feathergui/fgLayout.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_feather()
{
  BEGINTEST;
  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  const fgTransform FILL_TRANSFORM = { { 0, 0, 0, 0, 0, 1, 0, 1 }, 0, { 0, 0, 0, 0 } };

  fgRegisterFunction("statelistener", [](fgElement* self, const FG_Msg*) { fgElement* progbar = fgGetID("#progbar"); progbar->SetValueF(self->GetValueF() / self->GetRangeF()); progbar->SetText(StrF("%i", self->GetValue(0))); });
  fgRegisterFunction("makepressed", [](fgElement* self, const FG_Msg*) { self->SetText("Pressed!"); });

  fgLayout layout;
  fgLayout_Init(&layout, 0, 0);
  fgLayout_LoadFileXML(&layout, "../media/feathertest.xml");
  fgSingleton()->gui->LayoutLoad(&layout);

  fgElement* tabfocus = fgGetID("#tabfocus");
  if(tabfocus)
    tabfocus->GetSelectedItem()->Action();
  //fgCreate("Radiobutton", &fgSingleton()->gui.element, 0, 0, FGELEMENT_EXPAND, &fgTransform { { 190, 0, 130, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } }, 0)->SetText("Radio Test 1");

  HighPrecisionTimer time;


  /*std::function<size_t(const FG_Msg&)> guicallback = [&](const FG_Msg& evt) -> size_t
  {
    if(evt.type == FG_KEYDOWN || evt.type == FG_KEYUP)
    {
      bool isdown = evt.type == FG_KEYDOWN;
      switch(evt.keycode)
      {
      case FG_KEY_DOWN:
        fgSendSubMsg<FG_ACTION, float, float>(boxholder, FGSCROLLBAR_CHANGE, 0.0, -1.0);
        break;
      case FG_KEY_ESCAPE:
        if(isdown) engine->Quit();
        break;
      case FG_KEY_RETURN:
        if(isdown && !evt.IsAltDown()) gotonext = true;
        break;
      case FG_KEY_F11:
        if(isdown)
        {
          if(fgDebug_Get() != 0 && !(fgDebug_Get()->element.flags&FGELEMENT_HIDDEN))
            fgDebug_Hide();
          else
            fgDebug_Show(200, 200);
        }
      }
    }
    return 0;
  };

  engine->SetPreprocess(guicallback);*/
  engine->GetLayer(0)->SetClearColor(0xFF000000);

  while(!gotonext && engine->Begin())
  {
    engine->GetGUI().Render(psTransform2D::Zero);
    engine->End();
    engine->FlushMessages();
    fgRoot_Update(fgSingleton(), time.Update()*0.001);
    updatefpscount(timer, fps);
  }
  
  fgElement_Clear(*fgSingleton());
  fgLayout_Destroy(&layout);

  ENDTEST;
}
