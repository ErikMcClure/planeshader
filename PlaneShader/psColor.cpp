// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psColor.h"

using namespace planeshader;

uint16_t psColor::BitsPerPixel(FORMATS format)
{
  switch(format)
  {
  case FMT_R32G32B32A32X:
  case FMT_R32G32B32A32F:
  case FMT_R32G32B32A32U:
  case FMT_R32G32B32A32S:
    return 128;
  case FMT_R32G32B32X:
  case FMT_R32G32B32F:
  case FMT_R32G32B32U:
  case FMT_R32G32B32S:
    return 96;
  case FMT_R16G16B16A16X:
  case FMT_R16G16B16A16F:
  case FMT_R16G16B16A16:
  case FMT_R16G16B16A16U:
  case FMT_U16V16W16Q16:
  case FMT_R16G16B16A16S:
  case FMT_R32G32X:
  case FMT_R32G32F:
  case FMT_R32G32U:
  case FMT_R32G32S:
  case FMT_R32G8X24X:
  case FMT_D32S8X24:
  case FMT_R32X8X24:
  case FMT_X32G8X24:
    return 64;
  case FMT_R10G10B10A2X:
  case FMT_R10G10B10A2:
  case FMT_R10G10B10A2U:
  case FMT_R11G11B10F:
  case FMT_R8G8B8A8X:
  case FMT_R8G8B8A8:
  case FMT_R8G8B8A8U:
  case FMT_U8V8W8Q8:
  case FMT_R8G8B8A8S:
  case FMT_R16G16X:
  case FMT_R16G16F:
  case FMT_R16G16:
  case FMT_R16G16U:
  case FMT_U16V16:
  case FMT_R16G16S:
  case FMT_R32X:
  case FMT_D32F:
  case FMT_R32F:
  case FMT_INDEX32:
  case FMT_R32S:
  case FMT_R24G8X:
  case FMT_D24S8:
  case FMT_R24X8:
  case FMT_X24G8:
  case FMT_R8G8_B8G8:
  case FMT_G8R8_G8B8:
  case FMT_B8G8R8A8:
  case FMT_B8G8R8X8:
  case FMT_B8G8R8A8X:
  case FMT_B8G8R8X8X:
    return 32;
  case FMT_R8G8X:
  case FMT_R8G8:
  case FMT_R8G8U:
  case FMT_U8V8:
  case FMT_R8G8S:
  case FMT_R16X:
  case FMT_R16F:
  case FMT_D16:
  case FMT_R16:
  case FMT_INDEX16:
  case FMT_U16:
  case FMT_R16S:
  case FMT_B5G6R5:
  case FMT_B5G5R5A1:
  case FMT_B4G4R4A4:
    return 16;
  case FMT_R8X:
  case FMT_R8:
  case FMT_R8U:
  case FMT_U8:
  case FMT_R8S:
  case FMT_A8:
    return 8;
  case FMT_R1:
    return 1;
  case FMT_BC1X:
  case FMT_BC1:
  case FMT_BC4X:
  case FMT_BC4:
  case FMT_WC4:
    return 4;
  case FMT_BC2X:
  case FMT_BC2:
  case FMT_BC3X:
  case FMT_BC3:
  case FMT_BC5X:
  case FMT_BC5:
  case FMT_WC5:
  case FMT_BC6HX:
  case FMT_BC6H_UF16:
  case FMT_BC6H_SF16:
  case FMT_BC7X:
  case FMT_BC7:
    return 8;
  }
  return 0;
}

// Adapted from: http://stackoverflow.com/a/3542975
uint16_t float_to_half(float f)
{
  union Bits
  {
    float f;
    int32_t si;
    uint32_t ui;
  };

  static int const shift = 13;
  static int32_t const infN = 0x7F800000; // flt32 infinity
  static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
  static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
  static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
  static int32_t const maxD = (infN >> shift) - (maxN >> shift) - 1;
  static int32_t const minD = (minN >> shift) - subC - 1;

  Bits v, s;
  v.f = f;
  uint32_t sign = v.si & 0x80000000;
  v.si ^= sign;
  sign >>= 16; // logical shift
  s.si = 0x52000000;
  s.si = s.f * v.f; // correct subnormals
  v.si ^= (s.si ^ v.si) & -(minN > v.si);
  v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
  v.ui >>= shift; // logical shift
  v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > (maxN >> shift));
  v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
  return v.ui | sign;
}

template<uint8_t TYPE, uint8_t BITS, uint16_t OFFSET>
void BSS_FORCEINLINE _psColor_WriteComponent(float c, void* target);

template<> void BSS_FORCEINLINE _psColor_WriteComponent<0, 32, 0>(float c, void* target) { ((uint32_t*)target)[0] = bss_util::fFastRound(bssclamp(c, 0.0f, 1.0f) * (std::numeric_limits<uint32_t>::max())); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<1, 32, 0>(float c, void* target) { ((int32_t*)target)[0] = bss_util::fFastRound(bssclamp(c, -1.0f, 1.0f) * (std::numeric_limits<int32_t>::max())); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<2, 32, 0>(float c, void* target) { ((float*)target)[0] = c; }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<0, 16, 0>(float c, void* target) { ((uint16_t*)target)[0] = bss_util::fFastRound(bssclamp(c, 0.0f, 1.0f) * (std::numeric_limits<uint16_t>::max())); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<1, 16, 0>(float c, void* target) { ((int16_t*)target)[0] = bss_util::fFastRound(bssclamp(c, -1.0f, 1.0f) * (std::numeric_limits<int16_t>::max())); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<2, 16, 0>(float c, void* target) { ((uint16_t*)target)[0] = float_to_half(c); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<0, 8, 0>(float c, void* target) { ((uint8_t*)target)[0] = bss_util::fFastRound(bssclamp(c, 0.0f, 1.0f) * (std::numeric_limits<uint8_t>::max())); }
template<> void BSS_FORCEINLINE _psColor_WriteComponent<1, 8, 0>(float c, void* target) { ((int8_t*)target)[0] = bss_util::fFastRound(bssclamp(c, -1.0f, 1.0f) * (std::numeric_limits<int8_t>::max())); }

uint16_t psColor::WriteFormat(FORMATS format, void* target) const
{
  uint8_t* t = reinterpret_cast<uint8_t*>(target);
  switch(format)
  {
  case FMT_R32G32B32A32F:
    _psColor_WriteComponent<2, 32, 0>(r, t + 0);
    _psColor_WriteComponent<2, 32, 0>(g, t + 4);
    _psColor_WriteComponent<2, 32, 0>(b, t + 8);
    _psColor_WriteComponent<2, 32, 0>(a, t + 12);
    return 128;
  case FMT_R32G32B32F:
    _psColor_WriteComponent<2, 32, 0>(r, t + 0);
    _psColor_WriteComponent<2, 32, 0>(g, t + 4);
    _psColor_WriteComponent<2, 32, 0>(b, t + 8);
    return 96;
  case FMT_R16G16B16A16F:
    _psColor_WriteComponent<2, 16, 0>(r, t + 0);
    _psColor_WriteComponent<2, 16, 0>(g, t + 2);
    _psColor_WriteComponent<2, 16, 0>(b, t + 4);
    _psColor_WriteComponent<2, 16, 0>(a, t + 6);
    return 64;
  case FMT_R16G16B16A16:
    _psColor_WriteComponent<0, 16, 0>(r, t + 0);
    _psColor_WriteComponent<0, 16, 0>(g, t + 2);
    _psColor_WriteComponent<0, 16, 0>(b, t + 4);
    _psColor_WriteComponent<0, 16, 0>(a, t + 6);
    return 64;
  case FMT_U16V16W16Q16:
    _psColor_WriteComponent<1, 16, 0>(r, t + 0);
    _psColor_WriteComponent<1, 16, 0>(g, t + 2);
    _psColor_WriteComponent<1, 16, 0>(b, t + 4);
    _psColor_WriteComponent<1, 16, 0>(a, t + 6);
    return 64;
  case FMT_R32G32F:
    _psColor_WriteComponent<2, 32, 0>(r, t + 0);
    _psColor_WriteComponent<2, 32, 0>(g, t + 4);
    return 64;
  case FMT_R8G8B8A8:
    _psColor_WriteComponent<0, 8, 0>(r, t + 0);
    _psColor_WriteComponent<0, 8, 0>(g, t + 1);
    _psColor_WriteComponent<0, 8, 0>(b, t + 2);
    _psColor_WriteComponent<0, 8, 0>(a, t + 3);
    return 32;
  case FMT_U8V8W8Q8:
    _psColor_WriteComponent<1, 8, 0>(r, t + 0);
    _psColor_WriteComponent<1, 8, 0>(g, t + 1);
    _psColor_WriteComponent<1, 8, 0>(b, t + 2);
    _psColor_WriteComponent<1, 8, 0>(a, t + 3);
    return 32;
  case FMT_R16G16F:
    _psColor_WriteComponent<0, 16, 0>(r, t + 0);
    _psColor_WriteComponent<0, 16, 0>(g, t + 2);
    return 32;
  case FMT_R16G16:
    _psColor_WriteComponent<0, 16, 0>(r, t + 0);
    _psColor_WriteComponent<0, 16, 0>(g, t + 2);
    return 32;
  case FMT_U16V16:
    _psColor_WriteComponent<1, 16, 0>(r, t + 0);
    _psColor_WriteComponent<1, 16, 0>(g, t + 2);
    return 32;
  case FMT_R32F:
    _psColor_WriteComponent<2, 32, 0>(r, t + 0);
    return 32;
  //case FMT_R24X8:
  case FMT_X24G8:
    _psColor_WriteComponent<1, 8, 0>(g, t + 3);
    return 32;
  case FMT_R8G8_B8G8:
    _psColor_WriteComponent<0, 8, 0>(r, t + 0);
    _psColor_WriteComponent<0, 8, 0>(g, t + 1);
    _psColor_WriteComponent<0, 8, 0>(b, t + 2);
    _psColor_WriteComponent<0, 8, 0>(g, t + 3);
    return 32;
  case FMT_G8R8_G8B8:
    _psColor_WriteComponent<0, 8, 0>(g, t + 0);
    _psColor_WriteComponent<0, 8, 0>(r, t + 1);
    _psColor_WriteComponent<0, 8, 0>(g, t + 2);
    _psColor_WriteComponent<0, 8, 0>(b, t + 3);
    return 32;
  case FMT_B8G8R8A8:
    _psColor_WriteComponent<0, 8, 0>(b, t + 0);
    _psColor_WriteComponent<0, 8, 0>(g, t + 1);
    _psColor_WriteComponent<0, 8, 0>(r, t + 2);
    _psColor_WriteComponent<0, 8, 0>(a, t + 3);
    return 32;
  case FMT_B8G8R8X8:
    _psColor_WriteComponent<0, 8, 0>(b, t + 0);
    _psColor_WriteComponent<0, 8, 0>(g, t + 1);
    _psColor_WriteComponent<0, 8, 0>(r, t + 2);
    return 32;
  case FMT_R8G8:
    _psColor_WriteComponent<0, 8, 0>(r, t + 0);
    _psColor_WriteComponent<0, 8, 0>(g, t + 1);
    return 16;
  case FMT_U8V8:
    _psColor_WriteComponent<1, 8, 0>(r, t + 0);
    _psColor_WriteComponent<1, 8, 0>(g, t + 1);
    return 16;
  case FMT_R16F:
    _psColor_WriteComponent<2, 16, 0>(r, t + 0);
    return 16;
  case FMT_R16:
    _psColor_WriteComponent<0, 16, 0>(r, t + 0);
    return 16;
  case FMT_U16:
    _psColor_WriteComponent<1, 16, 0>(r, t + 0);
    return 16;
    //case FMT_B5G6R5:
  //case FMT_B5G5R5A1:
  //case FMT_B4G4R4A4:
    //return 16;
  case FMT_R8:
    _psColor_WriteComponent<0, 8, 0>(r, t + 0);
    return 8;
  case FMT_U8:
    _psColor_WriteComponent<1, 8, 0>(r, t + 0);
    return 8;
  case FMT_A8:
    _psColor_WriteComponent<0, 8, 0>(a, t + 0);
    return 8;
  }

  return 0;
}

uint16_t psColor32::WriteFormat(FORMATS format, void* target) const
{
  float* f32 = reinterpret_cast<float*>(target);
  uint16_t* u16 = reinterpret_cast<uint16_t*>(target);
  uint8_t* u8 = reinterpret_cast<uint8_t*>(target);

  switch(format)
  {
  case FMT_R32G32B32A32F:
    f32[0] = r;
    f32[1] = g;
    f32[2] = b;
    f32[3] = a;
    return 128;
  case FMT_R32G32B32F:
    f32[0] = r;
    f32[1] = g;
    f32[2] = b;
    return 96;
  case FMT_R16G16B16A16F:
    u16[0] = float_to_half(r / 255.0f);
    u16[1] = float_to_half(g / 255.0f);
    u16[2] = float_to_half(b / 255.0f);
    u16[3] = float_to_half(a / 255.0f);
    return 64;
  case FMT_R16G16B16A16:
    u16[0] = (r * 257); // we multiply by 257, not 256, because multiplying 255 by 256 gives us 0b1111111100000000, not 0b1111111111111111. This works because 257 is 0b0000000100000001
    u16[1] = (g * 257);
    u16[2] = (b * 257);
    u16[3] = (a * 257);
    return 64;
  case FMT_U16V16W16Q16:
    u16[0] = (r * 257)>>1;
    u16[1] = (g * 257)>>1;
    u16[2] = (b * 257)>>1;
    u16[3] = (a * 257)>>1;
    return 64;
  case FMT_R32G32F:
    f32[0] = r;
    f32[1] = g;
    return 64;
  case FMT_R8G8B8A8: // native storage format
    reinterpret_cast<uint32_t*>(target)[0] = color;
    return 32;
  case FMT_U8V8W8Q8:
    u8[0] = r>>1;
    u8[1] = g>>1;
    u8[2] = b>>1;
    u8[3] = a>>1;
    return 32;
  case FMT_R16G16F:
    u16[0] = float_to_half(r / 255.0f);
    u16[1] = float_to_half(g / 255.0f);
    return 32;
  case FMT_R16G16:
    u16[0] = (r * 257);
    u16[1] = (g * 257);
    return 32;
  case FMT_U16V16:
    u16[0] = (r * 257) >> 1;
    u16[1] = (g * 257) >> 1;
    return 32;
  case FMT_R32F:
    f32[0] = r;
    return 32;
    //case FMT_R24X8:
  case FMT_X24G8:
    u8[3] = g;
    return 32;
  case FMT_R8G8_B8G8:
    u8[0] = r;
    u8[1] = g;
    u8[2] = b;
    u8[3] = g;
    return 32;
  case FMT_G8R8_G8B8:
    u8[0] = g;
    u8[1] = r;
    u8[2] = g;
    u8[3] = b;
    return 32;
  case FMT_B8G8R8A8:
    u8[0] = b;
    u8[1] = g;
    u8[2] = r;
    u8[3] = a;
    return 32;
  case FMT_B8G8R8X8:
    u8[0] = b;
    u8[1] = g;
    u8[2] = r;
    return 32;
  case FMT_R8G8:
    u8[0] = r;
    u8[1] = g;
    return 16;
  case FMT_U8V8:
    u8[0] = r >> 1;
    u8[1] = g >> 1;
    return 16;
  case FMT_R16F:
    u16[0] = float_to_half(r / 255.0f);
    return 16;
  case FMT_R16:
    u16[0] = (r * 257);
    return 16;
  case FMT_U16:
    u16[0] = (r * 257) >> 1;
    return 16;
  case FMT_B5G6R5:
    u16[0] = (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
    return 16;
  case FMT_B5G5R5A1:
    u16[0] = (b >> 3) | ((g >> 3) << 5) | ((r >> 3) << 10) | ((a > 0) << 15);
    return 16;
  case FMT_B4G4R4A4:
    u16[0] = (b >> 4) | ((g >> 4) << 4) | ((r >> 4) << 8) | ((a >> 4) << 12);
    return 16;
  case FMT_R8:
    u8[0] = r;
    return 8;
  case FMT_U8:
    u8[0] = r >> 1;
    return 8;
  case FMT_A8:
    u8[0] = a;
    return 8;
  }

  return 0;
}

uint16_t BSS_FASTCALL psColor::ReadFormat(FORMATS format, const void* target)
{
  const float* f32 = reinterpret_cast<const float*>(target);
  const uint16_t* u16 = reinterpret_cast<const uint16_t*>(target);
  const int16_t* i16 = reinterpret_cast<const int16_t*>(target);
  const uint8_t* u8 = reinterpret_cast<const uint8_t*>(target);
  const int8_t* i8 = reinterpret_cast<const int8_t*>(target);
  switch(format)
  {
  case FMT_R32G32B32A32F:
    r = f32[0];
    g = f32[1];
    b = f32[2];
    a = f32[3];
    return 128;
  case FMT_R32G32B32F:
    r = f32[0];
    g = f32[1];
    b = f32[2];
    return 96;
  //case FMT_R16G16B16A16F:
  //  _psColor_WriteComponent<2, 16, 0>(r, t + 0);
  //  _psColor_WriteComponent<2, 16, 0>(g, t + 2);
  //  _psColor_WriteComponent<2, 16, 0>(b, t + 4);
  //  _psColor_WriteComponent<2, 16, 0>(a, t + 6);
  //  return 64;
  case FMT_R16G16B16A16:
    r = u16[0] / 65535.0f;
    g = u16[1] / 65535.0f;
    b = u16[2] / 65535.0f;
    a = u16[3] / 65535.0f;
    return 64;
  case FMT_U16V16W16Q16:
    r = i16[0] / 32767.0f; // Note: This technically only maps to approximately [-1.0,0.98]
    g = i16[1] / 32767.0f;
    b = i16[2] / 32767.0f;
    a = i16[3] / 32767.0f;
    return 64;
  case FMT_R32G32F:
    r = f32[0];
    g = f32[1];
    return 64;
  case FMT_R8G8B8A8:
    r = u8[0] / 255.0f;
    g = u8[1] / 255.0f;
    b = u8[2] / 255.0f;
    a = u8[3] / 255.0f;
    return 32;
  case FMT_U8V8W8Q8:
    r = i8[0] / 127.0f; // Note: This technically only maps to approximately [-1.0,0.98]
    g = i8[1] / 127.0f;
    b = i8[2] / 127.0f;
    a = i8[3] / 127.0f;
    return 32;
  //case FMT_R16G16F:
  //  _psColor_WriteComponent<0, 16, 0>(r, t + 0);
  //  _psColor_WriteComponent<0, 16, 0>(g, t + 2);
  //  return 32;
  case FMT_R16G16:
    r = u16[0] / 65535.0f;
    g = u16[1] / 65535.0f;
    return 32;
  case FMT_U16V16:
    r = i16[0] / 32767.0f;
    g = i16[1] / 32767.0f;
    return 32;
  case FMT_R32F:
    r = f32[0];
    return 32;
    //case FMT_R24X8:
  case FMT_X24G8:
    g = u16[2] / 255.0f;
    return 32;
  case FMT_R8G8_B8G8:
    r = u16[0] / 255.0f;
    g = u16[1] / 255.0f;
    b = u16[2] / 255.0f;
    g = u16[3] / 255.0f; // same value as above
    return 32;
  case FMT_G8R8_G8B8:
    g = u16[0] / 255.0f;
    r = u16[1] / 255.0f;
    g = u16[2] / 255.0f; // we could just ignore this, it's supposed to be the same value as above
    b = u16[3] / 255.0f;
    return 32;
  case FMT_B8G8R8A8:
    b = u16[0] / 255.0f;
    g = u16[1] / 255.0f;
    r = u16[2] / 255.0f;
    a = u16[3] / 255.0f;
    return 32;
  case FMT_B8G8R8X8:
    b = u16[0] / 255.0f;
    g = u16[1] / 255.0f;
    r = u16[2] / 255.0f;
    return 32;
  case FMT_R8G8:
    r = u16[0] / 255.0f;
    g = u16[1] / 255.0f;
    return 16;
  case FMT_U8V8:
    r = i8[0] / 127.0f;
    g = i8[1] / 127.0f;
    return 16;
  //case FMT_R16F:
  //  _psColor_WriteComponent<2, 16, 0>(r, t + 0);
  //  return 16;
  case FMT_R16:
    r = u16[0] / 65535.0f;
    return 16;
  case FMT_U16:
    r = i16[0] / 32767.0f;
    return 16;
    //case FMT_B5G6R5:
    //case FMT_B5G5R5A1:
    //case FMT_B4G4R4A4:
    //return 16;
  case FMT_R8:
    r = u8[0] / 255.0f;
    return 8;
  case FMT_U8:
    r = i8[0] / 127.0f;
    return 8;
  case FMT_A8:
    a = u8[0] / 255.0f;
    return 8;
  }

  return 0;
}

uint16_t BSS_FASTCALL psColor32::ReadFormat(FORMATS format, const void* target)
{
  const float* f32 = reinterpret_cast<const float*>(target);
  const uint16_t* u16 = reinterpret_cast<const uint16_t*>(target);
  const int16_t* i16 = reinterpret_cast<const int16_t*>(target);
  const uint8_t* u8 = reinterpret_cast<const uint8_t*>(target);
  const int8_t* i8 = reinterpret_cast<const int8_t*>(target);

  switch(format)
  {
  case FMT_R32G32B32A32F:
    _psColor_WriteComponent<0, 8, 0>(f32[0], &r);
    _psColor_WriteComponent<0, 8, 0>(f32[1], &g);
    _psColor_WriteComponent<0, 8, 0>(f32[2], &b);
    _psColor_WriteComponent<0, 8, 0>(f32[3], &a);
    return 128;
  case FMT_R32G32B32F:
    _psColor_WriteComponent<0, 8, 0>(f32[0], &r);
    _psColor_WriteComponent<0, 8, 0>(f32[1], &g);
    _psColor_WriteComponent<0, 8, 0>(f32[2], &b);
    return 96;
  //case FMT_R16G16B16A16F:
    //u16[0] = float_to_half(r / 255.0f);
    //u16[1] = float_to_half(g / 255.0f);
    //u16[2] = float_to_half(b / 255.0f);
    //u16[3] = float_to_half(a / 255.0f);
    //return 64;
  case FMT_R16G16B16A16:
    r = u16[0] >> 8; // Division truncates, so dividing 65535/257 gives the same end result as 65535/256
    g = u16[1] >> 8;
    b = u16[2] >> 8;
    a = u16[3] >> 8;
    return 64;
  case FMT_U16V16W16Q16:
    r = (i16[0] >> 8) + 0x80; // shifts the range of the signed int into the unsigned int range
    g = (i16[1] >> 8) + 0x80;
    b = (i16[2] >> 8) + 0x80;
    a = (i16[3] >> 8) + 0x80;
    return 64;
  case FMT_R32G32F:
    _psColor_WriteComponent<0, 8, 0>(f32[0], &r);
    _psColor_WriteComponent<0, 8, 0>(f32[1], &g);
    return 64;
  case FMT_R8G8B8A8: // native storage format
    color = reinterpret_cast<const uint32_t*>(target)[0];
    return 32;
  case FMT_U8V8W8Q8:
    r = i8[0] + 0x80;
    g = i8[1] + 0x80;
    b = i8[2] + 0x80;
    a = i8[3] + 0x80;
    return 32;
  //case FMT_R16G16F:
    //u16[0] = float_to_half(r / 255.0f);
    //u16[1] = float_to_half(g / 255.0f);
    //return 32;
  case FMT_R16G16:
    r = u16[0] >> 8;
    g = u16[1] >> 8;
    return 32;
  case FMT_U16V16:
    r = (i16[0] >> 8) + 0x80;
    g = (i16[1] >> 8) + 0x80;
    return 32;
  case FMT_R32F:
    _psColor_WriteComponent<0, 8, 0>(f32[0], &r);
    return 32;
    //case FMT_R24X8:
  case FMT_X24G8:
    g = u8[3];
    return 32;
  case FMT_R8G8_B8G8:
    r = u8[0];
    g = u8[1];
    b = u8[2];
    g = u8[3];
    return 32;
  case FMT_G8R8_G8B8:
    g = u8[0];
    r = u8[1];
    g = u8[2];
    b = u8[3];
    return 32;
  case FMT_B8G8R8A8:
    b = u8[0];
    g = u8[1];
    r = u8[2];
    a = u8[3];
    return 32;
  case FMT_B8G8R8X8:
    b = u8[0];
    g = u8[1];
    r = u8[2];
    return 32;
  case FMT_R8G8:
    r = u8[0];
    g = u8[1];
    return 16;
  case FMT_U8V8:
    r = i8[0] + 0x80;
    g = i8[1] + 0x80;
    return 16;
  //case FMT_R16F:
    //u16[0] = float_to_half(r / 255.0f);
    //return 16;
  case FMT_R16:
    r = u16[0] >> 8;
    return 16;
  case FMT_U16:
    r = (i16[0] >> 8) + 0x80;
    return 16;
  //case FMT_B5G6R5:
  //  u16[0] = (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
  //  return 16;
  //case FMT_B5G5R5A1:
  //  u16[0] = (b >> 3) | ((g >> 3) << 5) | ((r >> 3) << 10) | ((a > 0) << 15);
  //  return 16;
  //case FMT_B4G4R4A4:
  //  u16[0] = (b >> 4) | ((g >> 4) << 4) | ((r >> 4) << 8) | ((a >> 4) << 12);
  //  return 16;
  case FMT_R8:
    r = u8[0];
    return 8;
  case FMT_U8:
    r = i8[0] + 0x80;
    return 8;
  case FMT_A8:
    a = u8[0];
    return 8;
  }

  return 0;
}

