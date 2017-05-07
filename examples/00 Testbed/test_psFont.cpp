// Copyright ©2017 Black Sphere Studios

#include "testbed.h"
#include "psTex.h"
#include "psFont.h"

using namespace bss;
using namespace planeshader;

TESTDEF::RETPAIR test_psFont()
{
  BEGINTEST;

  psFont* font = psFont::Create("Arial", 400, false, 14, psFont::FAA_LCD);

  int fps = 0;
  auto timer = HighPrecisionTimer::OpenProfiler();
  psDriver* driver = engine->GetDriver();

  psStateblock* block = STATEBLOCK_LIBRARY::SUBPIXELBLEND->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 1, 0.0f));
  block = block->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 0, 0.0f));
  block = block->Append(STATEINFO(TYPE_BLEND_BLENDFACTOR, 2, 0.0f));
  //font->PreloadGlyphs("qwertyuiopasdfghjklzxcvbnm`1234567890-=[];',./~!@#$%^&*()_+{}|:");

  while(!gotonext && engine->Begin())
  {
    processGUI();
    driver->Clear(0xFF999999);
    driver->SetTextures(0, 0);
    driver->DrawRect(driver->library.IMAGE0, 0, psRectRotateZ(0, 0, 100, 110, 0), 0, 0, 0xFF999900, PSFLAG_FIXED);
    //font->DrawText("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla maximus sem at ante porttitor vehicula. Nulla a lorem imperdiet, consectetur metus id, congue enim. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin placerat a ipsum ac sodales. Curabitur vel neque scelerisque elit mollis convallis. Proin porta augue metus, sed pulvinar nisl mollis ut. Proin at aliquam erat. Quisque at diam tellus. Aenean facilisis justo ut mauris egestas dignissim. Maecenas scelerisque, ante ac blandit consectetur, magna sem pharetra massa, eu luctus orci ligula luctus augue. Integer at metus eros. Donec sed eros molestie, posuere nunc id, porta sem. \nMauris fermentum mauris ac eleifend ultrices.Fusce nec sollicitudin turpis, a ultricies metus.Nulla suscipit cursus orci, ac fringilla massa volutpat et.Nullam vestibulum dolor at tortor bibendum condimentum.Donec vitae faucibus risus, ut placerat mauris.Curabitur quis purus at urna pharetra lobortis.Pellentesque turpis velit, molestie aliquet elit sed, vestibulum rutrum nibh. \nSuspendisse ultricies leo nec ante accumsan ullamcorper.Suspendisse scelerisque molestie enim sit amet lacinia.Proin at lorem justo.Curabitur lectus ipsum, accumsan at quam eu, iaculis pellentesque felis.Fusce blandit feugiat dui, id placerat justo sollicitudin sed.Cras auctor lorem hendrerit leo facilisis porttitor.Sed vitae pulvinar purus, sed ornare ligula.\nPhasellus blandit, magna quis bibendum mattis, neque quam gravida quam, at tempus sem sapien eu mi.Phasellus ornare laoreet neque at blandit.Suspendisse vulputate fringilla fermentum.Fusce ante eros, laoreet ultricies eros sit amet, lobortis viverra elit.Curabitur consequat erat neque, in fringilla eros elementum eu.Quisque aliquam laoreet metus, volutpat vulputate tortor vehicula ut.Fusce sodales commodo justo, in condimentum ipsum aliquam at.Phasellus eget tellus ac arcu ultrices vehicula.Integer sagittis metus nibh, in varius mi scelerisque quis.Etiam ullamcorper gravida urna, et vestibulum velit posuere id.Aenean fermentum nibh ac dui rhoncus volutpat.Cras quis felis eget tortor vehicula interdum.In efficitur nulla quam, non condimentum ipsum pulvinar non.\nCras ultricies mi sed lacinia consequat.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Suspendisse potenti.Nam consectetur eleifend libero sed pharetra.Suspendisse in dolor dui.Sed imperdiet pellentesque fermentum.Vivamus ac tortor felis.Aliquam id turpis euismod, tincidunt sapien ac, varius sapien.Vivamus id nulla mauris.");
    font->DrawText(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, "the dog jumped \nover the lazy fox 1234567890", font->GetLineHeight(), 0, psRectRotateZ(0.6, 0.6, 100, 0, 0), 0xCCFFAAFF, PSFONT_WORDBREAK);

    const psTex* t = font->GetTex();
    driver->SetTextures(&t, 1);
    driver->DrawRect(driver->library.TEXT1, STATEBLOCK_LIBRARY::SUBPIXELBLEND1, psRectRotateZ(0, 110, t->GetDim().x, 110 + t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    //driver->DrawRect(driver->library.IMAGE, 0, psRectRotateZ(0, 100, t->GetDim().x, 100+t->GetDim().y, 0), &RECT_UNITRECT, 1, 0xFFFFFFFF, PSFLAG_FIXED);
    engine->End();
    engine->FlushMessages();

    updatefpscount(timer, fps);
  }
  ENDTEST;
}