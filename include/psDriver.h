// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __DRIVER_H__PS__
#define __DRIVER_H__PS__

#include "psLine.h"

namespace planeshader {
  class psTex;
  class psDirectX9;
  class psDirectX11;
  class psOpenGL1;
  class psNullDriver;
  struct STATEINFO;
  class psShader;
  class psTexblock;

  struct BSS_COMPILER_DLLEXPORT RealDriver
  {
    union {
      psDirectX9* dx9;
      psDirectX11* dx11;
      psOpenGL1* ogl1;
      psNullDriver* nul;
    };
    enum DRIVERTYPE : unsigned char
    {
      DRIVERTYPE_DX9,
      DRIVERTYPE_DX10,
      DRIVERTYPE_DX11,
      DRIVERTYPE_DX12,
      DRIVERTYPE_GL1,
      DRIVERTYPE_GL2,
      DRIVERTYPE_GL3,
      DRIVERTYPE_GL4,
      DRIVERTYPE_GL_ES2,
      DRIVERTYPE_GL_ES3,
      DRIVERTYPE_VULKAN,
      DRIVERTYPE_WEBGL,
      DRIVERTYPE_NULL,
      DRIVERTYPE_NUM
    } type;
  };

  struct psVertex
  {
    float x;
    float y;
    float z;
    float w; // We have a w element to pad this to 32 bytes
    unsigned int color;
  };

  enum PRIMITIVETYPE : unsigned char {
    TRIANGLELIST=0,
    TRIANGLESTRIP,
    LINELIST,
    LINESTRIP,
    POINTLIST
  };

  enum FORMATS : unsigned char {
    FMT_UNKNOWN,
    FMT_R32G32B32A32X,
    FMT_R32G32B32A32F,
    FMT_R32G32B32A32U,
    FMT_R32G32B32A32S,
    FMT_R32G32B32X,
    FMT_R32G32B32F,
    FMT_R32G32B32U,
    FMT_R32G32B32S,
    FMT_R16G16B16A16X,
    FMT_R16G16B16A16F,
    FMT_R16G16B16A16,
    FMT_R16G16B16A16U,
    FMT_U16V16W16Q16,
    FMT_R16G16B16A16S,
    FMT_R32G32X,
    FMT_R32G32F,
    FMT_R32G32U,
    FMT_R32G32S,
    FMT_R32G8X24X,
    FMT_D32S8X24,
    FMT_R32X8X24,
    FMT_X32G8X24,
    FMT_R10G10B10A2X,
    FMT_R10G10B10A2,
    FMT_R10G10B10A2U,
    FMT_R11G11B10F,
    FMT_R8G8B8A8X,
    FMT_R8G8B8A8,
    FMT_R8G8B8A8_SRGB,
    FMT_R8G8B8A8U,
    FMT_U8V8W8Q8,
    FMT_R8G8B8A8S,
    FMT_R16G16X,
    FMT_R16G16F,
    FMT_R16G16,
    FMT_R16G16U,
    FMT_U16V16,
    FMT_R16G16S,
    FMT_R32X,
    FMT_D32F,
    FMT_R32F,
    FMT_R32S,
    FMT_R24G8X,
    FMT_D24S8,
    FMT_R24X8,
    FMT_X24G8,
    FMT_R8G8_B8G8,
    FMT_G8R8_G8B8,
    FMT_B8G8R8A8,
    FMT_B8G8R8A8_SRGB,
    FMT_B8G8R8X8,
    FMT_B8G8R8X8_SRGB,
    FMT_B8G8R8A8X,
    FMT_B8G8R8X8X,
    FMT_R8G8X,
    FMT_R8G8,
    FMT_R8G8U,
    FMT_U8V8,
    FMT_R8G8S,
    FMT_R16X,
    FMT_R16F,
    FMT_D16,
    FMT_R16,
    FMT_U16,
    FMT_R16S,
    FMT_B5G6R5,
    FMT_B5G5R5A1,
    FMT_B4G4R4A4,
    FMT_R8X,
    FMT_R8,
    FMT_R8U,
    FMT_U8,
    FMT_R8S,
    FMT_A8,
    FMT_R1,
    FMT_BC1X,
    FMT_BC1,
    FMT_BC1_SRGB,
    FMT_BC2X,
    FMT_BC2,
    FMT_BC2_SRGB,
    FMT_BC3X,
    FMT_BC3,
    FMT_BC3_SRGB,
    FMT_BC4X,
    FMT_BC4,
    FMT_WC4,
    FMT_BC5X,
    FMT_BC5,
    FMT_WC5,
    FMT_BC6HX,
    FMT_BC6H_UF16,
    FMT_BC6H_SF16,
    FMT_BC7X,
    FMT_BC7,
    FMT_BC7_SRGB,
    FMT_INDEX16,
    FMT_INDEX32,
  };

  struct psVertObj
  {
    void* verts;
    void* indices;
    unsigned int nvert; // Number of vertices 
    unsigned int nindice; // Number of indices
    unsigned int vsize; // Size of the vertices in this buffer
    FORMATS ifmt; // Either FMT_INDEX16 or FMT_INDEX32
    PRIMITIVETYPE mode; // Mode to render in
  };

  enum USAGETYPES : unsigned int {
    USAGE_DEFAULT = 0,
    USAGE_IMMUTABLE = 1,
    USAGE_DYNAMIC = 2,
    USAGE_STAGING = 3,
    USAGE_VERTEX = 4,
    USAGE_INDEX = 8,
    USAGE_CONSTANT_BUFFER = 12,
    USAGE_TEXTURECUBE = (1 << 4),
    USAGE_RENDERTARGET = (1 << 5),
    USAGE_SHADER_RESOURCE = (1 << 6),
    USAGE_DEPTH_STENCIL = (1 << 7),
    USAGE_MULTISAMPLE = (1 << 8),
    USAGE_USAGEMASK = 3,
    USAGE_BINDMASK = 12
  };

  enum ELEMENT_SEMANTICS : unsigned char {
    ELEMENT_BINORMAL=0,
    ELEMENT_BLENDINDICES,
    ELEMENT_BLENDWEIGHT,
    ELEMENT_COLOR,
    ELEMENT_NORMAL,
    ELEMENT_POSITION,
    ELEMENT_POSITIONT,
    ELEMENT_PSIZE,
    ELEMENT_TANGENT,
    ELEMENT_TEXCOORD
  };

  struct ELEMENT_DESC
  {
    ELEMENT_SEMANTICS semantic;
    unsigned char semanticIndex;
    FORMATS format;
    unsigned char IAslot;
    unsigned int byteOffset; // Set to -1 to define it as being directly after the next one
  };

  // A list of shader versions that we recognize
  enum SHADER_VER : unsigned char
  {
    VERTEX_SHADER_1_1=0, //vs_1_1
    VERTEX_SHADER_2_0,
    VERTEX_SHADER_2_a,
    VERTEX_SHADER_3_0,
    VERTEX_SHADER_4_0,
    VERTEX_SHADER_4_1,
    VERTEX_SHADER_5_0,
    PIXEL_SHADER_1_1, //ps_1_1
    PIXEL_SHADER_1_2,
    PIXEL_SHADER_1_3,
    PIXEL_SHADER_1_4,
    PIXEL_SHADER_2_0,
    PIXEL_SHADER_2_a,
    PIXEL_SHADER_2_b,
    PIXEL_SHADER_3_0,
    PIXEL_SHADER_4_0,
    PIXEL_SHADER_4_1,
    PIXEL_SHADER_5_0,
    GEOMETRY_SHADER_4_0, //gs_4_0
    GEOMETRY_SHADER_4_1,
    GEOMETRY_SHADER_5_0,
    COMPUTE_SHADER_4_0, //cs_4_0
    COMPUTE_SHADER_4_1,
    COMPUTE_SHADER_5_0,
    DOMAIN_SHADER_5_0, //ds_5_0
    HULL_SHADER_5_0, //hs_5_0
    NUM_SHADER_VERSIONS
  };

  enum LOCK_FLAGS : unsigned char
  {
    LOCK_READ=1,
    LOCK_WRITE=2,
    LOCK_READ_WRITE=3,
    LOCK_WRITE_DISCARD=4,
    LOCK_WRITE_NO_OVERWRITE=5,
    LOCK_DONOTWAIT=8,
    LOCK_TYPEMASK=LOCK_READ|LOCK_WRITE|LOCK_READ_WRITE|LOCK_WRITE_DISCARD|LOCK_WRITE_NO_OVERWRITE,
  };

  struct TEXTURE_DESC
  {
    psVec3Diu dim;
    USAGETYPES usage;
    FORMATS format;
    unsigned char miplevels;
  };
  
  enum FILTERS : unsigned char 
  {
    FILTER_NONE=0,
    FILTER_NEAREST,
    FILTER_LINEAR,
    FILTER_BOX,
    FILTER_TRIANGLE,
    FILTER_PREMULTIPLY, // Premultiplies the texture on load
    FILTER_PREMULTIPLY_SRGB, // Forces the premultiplication to happen in SRGB space even if using gamma correct textures.
    FILTER_ALPHABOX,
    FILTER_DEBUG, // Creates a black and white checkerboard pattern for debugging
    NUM_FILTERS
  };

  class PS_DLLEXPORT psDriver
  {
  protected:
    inline psDriver(const psVeciu& Screendim) : screendim(Screendim) {}

  public:
    inline virtual ~psDriver() {}
    // Begins a scene
    virtual bool Begin()=0;
    // Ends a scene
    virtual char End()=0;
    // Draws a vertex object
    virtual void BSS_FASTCALL Draw(psVertObj* buf, FLAG_TYPE flags, const float(&transform)[4][4]=identity)=0;
    // Draws a rectangle
    virtual void BSS_FASTCALL DrawRect(const psRectRotateZ rect, const psRect* uv, unsigned char numuv, unsigned int color, const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags, const float(&xform)[4][4]=identity)=0;
    virtual void BSS_FASTCALL DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned char numuv, FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawRectBatch(const psRectRotateZ rect, const psRect* uv, unsigned int color, const float(&xform)[4][4]=identity)=0;
    virtual void DrawRectBatchEnd(const float(&xform)[4][4]=identity)=0;
    // Draws a polygon
    virtual void BSS_FASTCALL DrawPolygon(const psVec* verts, int num, psVec3D offset, unsigned long vertexcolor, FLAG_TYPE flags, const float(&transform)[4][4] = identity)=0;
    virtual void BSS_FASTCALL DrawPolygon(const psVertex* verts, int num, FLAG_TYPE flags, const float(&transform)[4][4] = identity)=0;
    // Draws points (which are always batch rendered)
    virtual void BSS_FASTCALL DrawPointsBegin(const psTex* const* texes, unsigned char numtex, float size, FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawPoints(psVertex* particles, unsigned int num)=0;
    virtual void DrawPointsEnd()=0;
    // Draws lines (which are also always batch rendered)
    virtual void BSS_FASTCALL DrawLinesStart(FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawLines(const psLine& line, float Z1, float Z2, unsigned long vertexcolor, const float(&transform)[4][4] = identity)=0;
    virtual void DrawLinesEnd(const float(&transform)[4][4] = identity)=0;
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void BSS_FASTCALL PushCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport)=0;
    virtual void BSS_FASTCALL PushCamera3D(const float(&m)[4][4], const psRectiu& viewport)=0;
    virtual void BSS_FASTCALL PopCamera() = 0;
    // Applies the camera transform (or it's inverse) according to the flags to a point.
    virtual psVec3D BSS_FASTCALL TransformPoint(const psVec3D& point, FLAG_TYPE flags) const=0;
    virtual psVec3D BSS_FASTCALL ReversePoint(const psVec3D& point, FLAG_TYPE flags) const=0;
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad()=0;
    // Gets/Sets the extent
    virtual const psVec& GetExtent() const=0;
    virtual void BSS_FASTCALL SetExtent(float znear, float zfar)=0;
    // Creates a vertex or index buffer
    virtual void* BSS_FASTCALL CreateBuffer(size_t bytes, unsigned int usage, const void* initdata=0)=0;
    virtual void* BSS_FASTCALL LockBuffer(void* target, unsigned int flags)=0;
    virtual void BSS_FASTCALL UnlockBuffer(void* target)=0;
    virtual void* BSS_FASTCALL LockTexture(void* target, unsigned int flags, unsigned int& pitch, unsigned char miplevel = 0)=0;
    virtual void BSS_FASTCALL UnlockTexture(void* target, unsigned char miplevel = 0)=0;
    // Creates a texture
    virtual void* BSS_FASTCALL CreateTexture(psVeciu dim, FORMATS format, unsigned int usage=USAGE_SHADER_RESOURCE, unsigned char miplevels=0, const void* initdata=0, void** additionalview=0, psTexblock* texblock=0)=0;
    virtual void* BSS_FASTCALL LoadTexture(const char* path, unsigned int usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, void** additionalview=0, unsigned char miplevels=0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock=0, bool sRGB = false)=0;
    virtual void* BSS_FASTCALL LoadTextureInMemory(const void* data, size_t datasize, unsigned int usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, void** additionalview=0, unsigned char miplevels=0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock=0, bool sRGB = false)=0;
    virtual void BSS_FASTCALL CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, unsigned char miplevel = 0)=0;
    // Pushes or pops a scissor rect on to the stack
    virtual void BSS_FASTCALL PushScissorRect(const psRect& rect)=0;
    virtual void PopScissorRect()=0;
    // Sets the current rendertargets, setting all the rest to null.
    virtual void BSS_FASTCALL SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil=0)=0;
    // Sets shader constants
    virtual void BSS_FASTCALL SetShaderConstants(void* constbuf, SHADER_VER shader)=0;
    // Sets textures for a given type of shader (in DX9 this is completely ignored)
    virtual void BSS_FASTCALL SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader=PIXEL_SHADER_1_1)=0;
    // Builds a stateblock from the given set of state changes
    virtual void* BSS_FASTCALL CreateStateblock(const STATEINFO* states, unsigned int count)=0;
    // Builds a texblock from the given set of sampler states
    virtual void* BSS_FASTCALL CreateTexblock(const STATEINFO* states, unsigned int count)=0;
    // Sets a given stateblock
    virtual void BSS_FASTCALL SetStateblock(void* stateblock)=0;
    // Create a vertex layout from several element descriptions
    virtual void* BSS_FASTCALL CreateLayout(void* shader, const ELEMENT_DESC* elements, unsigned char num)=0;
    virtual void BSS_FASTCALL SetLayout(void* layout)=0;
    // Frees a created resource of the specified type
    enum RESOURCE_TYPE : unsigned char { RES_TEXTURE, RES_SURFACE, RES_DEPTHVIEW, RES_SHADERVS, RES_SHADERPS, RES_SHADERGS, RES_SHADERCS, RES_SHADERDS, RES_SHADERHS, RES_STATEBLOCK, RES_TEXBLOCK, RES_INDEXBUF, RES_VERTEXBUF, RES_CONSTBUF, RES_LAYOUT };
    virtual TEXTURE_DESC BSS_FASTCALL GetTextureDesc(void* t)=0;
    virtual void BSS_FASTCALL FreeResource(void* p, RESOURCE_TYPE t)=0;
    virtual void BSS_FASTCALL GrabResource(void* p, RESOURCE_TYPE t)=0;
    virtual void BSS_FASTCALL CopyResource(void* dest, void* src, RESOURCE_TYPE t)=0;
    virtual void BSS_FASTCALL Resize(psVeciu dim, FORMATS format, char fullscreen)=0;
    // Clears everything to a specified color
    virtual void BSS_FASTCALL Clear(unsigned int color)=0;
    // Gets the backbuffer texture
    virtual psTex* GetBackBuffer()=0;
    // Gets a pointer to the driver implementation
    virtual RealDriver GetRealDriver()=0;
    // Gets/Sets the effective DPI
    virtual void SetDPI(psVeciu dpi = psVeciu(BASE_DPI))=0;
    virtual psVeciu GetDPI() = 0;
    inline psVec GetDPIScale() { psVeciu dpi = GetDPI(); return ((dpi == psVeciu(BASE_DPI)) ? psVec(1.0f) : (psVec(GetDPI()) / psVec((float)BASE_DPI))); }
    inline psVec GetInvDPIScale() { psVeciu dpi = GetDPI(); return ((dpi == psVeciu(BASE_DPI)) ? psVec(1.0f) : (psVec((float)BASE_DPI) / psVec(GetDPI()))); }
    // Snaps a coordinate to a pixel value that will correspond to a pixel after it gets scaled by DPI for rendering on the backbuffer
    inline psVec SnapToDPI(const psVec& p)
    { 
      psVec s = (GetDPI() == psVeciu(BASE_DPI))?psVec(1.0f):(GetDPI() / psVec((float)BASE_DPI));
      psVec r = p*s; // Scale to actual destination coordinates
      r.x = roundf(r.x); // Round to nearest integer
      r.y = roundf(r.y);
      return r / s; // Convert back to scaled coordinates
    }
    // Compile a shader from a string
    virtual void* BSS_FASTCALL CompileShader(const char* source, SHADER_VER profile, const char* entrypoint="")=0;
    // Create an actual shader object from compiled shader source (either precompiled or from CompileShader())
    virtual void* BSS_FASTCALL CreateShader(const void* data, SHADER_VER profile)=0;
    // Sets current shader
    virtual char BSS_FASTCALL SetShader(void* shader, SHADER_VER profile)=0;
    // Returns true if shader version is supported
    virtual bool BSS_FASTCALL ShaderSupported(SHADER_VER profile)=0;

    BSS_FORCEINLINE static FORMATS ToSRGBFormat(FORMATS format)
    {
      switch(format)
      {
      case FMT_R8G8B8A8: return FMT_R8G8B8A8_SRGB;
      case FMT_B8G8R8A8: return FMT_B8G8R8A8_SRGB;
      case FMT_B8G8R8X8: return FMT_B8G8R8X8_SRGB;
      case FMT_BC1: return FMT_BC1_SRGB;
      case FMT_BC2: return FMT_BC2_SRGB;
      case FMT_BC3: return FMT_BC3_SRGB;
      case FMT_BC7: return FMT_BC7_SRGB;
      }
      return format;
    }
    BSS_FORCEINLINE static FORMATS FromSRGBFormat(FORMATS format)
    {
      switch(format)
      {
      case FMT_R8G8B8A8_SRGB: return FMT_R8G8B8A8;
      case FMT_B8G8R8A8_SRGB: return FMT_B8G8R8A8;
      case FMT_B8G8R8X8_SRGB: return FMT_B8G8R8X8;
      case FMT_BC1_SRGB: return FMT_BC1;
      case FMT_BC2_SRGB: return FMT_BC2;
      case FMT_BC3_SRGB: return FMT_BC3;
      case FMT_BC7_SRGB: return FMT_BC7;
      }
      return format;
    }
    BSS_FORCEINLINE static void _MatrixTranslate(float(&out)[4][4], float x, float y, float z) { out[3][0]=x; out[3][1]=y; out[3][2]=z; } //This is the transpose of what is NORMALLY done, presumably due to the order of multiplication
    BSS_FORCEINLINE static void _MatrixScale(float(&out)[4][4], float x, float y, float z) { out[0][0]=x; out[1][1]=y; out[2][2]=z; }
    BSS_FORCEINLINE static void _MatrixRotateZ(float(&out)[4][4], float angle) { float ca=cos(angle); float sa=sin(angle); out[0][0]=ca; out[1][0]=-sa; out[0][1]=sa; out[1][1]=ca; } //Again, we need the transpose
    BSS_FORCEINLINE static void BSS_FASTCALL _inversetransform(float(&mat)[4][4]) { mat[3][0]=(-mat[3][0]); mat[3][1]=(-mat[3][1]); }
    BSS_FORCEINLINE static void BSS_FASTCALL _inversetransformadd(float(&mat)[4][4], const float(&add)[4][4]) { mat[3][0]=add[3][0]-mat[3][0]; mat[3][1]=add[3][1]-mat[3][1]; mat[3][2]=add[3][2]; }
    static const float identity[4][4];
    static const int BASE_DPI = 96;

    struct SHADER_LIBRARY
    {
      psShader* IMAGE0;
      psShader* IMAGE;
      psShader* IMAGE2;
      psShader* IMAGE3;
      psShader* IMAGE4;
      psShader* IMAGE5;
      psShader* IMAGE6;
      psShader* CIRCLE;
      psShader* POLYGON;
      psShader* LINE;
      psShader* PARTICLE;
      psShader* TEXT1;
      psShader* DEBUG;
    } library;

    psVec screendim; // DPI scaled screen dimensions, which can theoretically be fractional due to 1.5x DPI scaling
    psVeciu rawscreendim;
  };

  struct PS_DLLEXPORT psDriverHold
  { 
    static psDriver* GetDriver(); // Cannot be inline'd for DLL reasons.

  protected:
    static psDriver* _driver; //public pointer to the driver
  };
}

#endif