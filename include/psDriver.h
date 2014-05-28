// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __DRIVER_H__PS__
#define __DRIVER_H__PS__

#include "psCamera.h"

namespace planeshader {
  struct psTex;
  template<class T> struct cLineT;
  class psDirectX9;
  class psDirectX10;
  class psOpenGL1;
  class psEmptyDriver;

  struct BSS_COMPILER_DLLEXPORT RealDriver
  {
    union {
      psDirectX9* dx9;
      psDirectX10* dx10;
      psOpenGL1* ogl1;
      psEmptyDriver* empty;
    };
    enum DRIVERTYPE : unsigned char
    {
      DRIVERTYPE_DX9,
      DRIVERTYPE_DX10,
      DRIVERTYPE_DX11,
      DRIVERTYPE_GL1,
      DRIVERTYPE_GL2,
      DRIVERTYPE_GL3,
      DRIVERTYPE_GL4,
      DRIVERTYPE_GL_ES,
      DRIVERTYPE_EMPTY,
      DRIVERTYPE_NUM
    } type;
  };

  enum PRIMITIVETYPE : unsigned char {
    TRIANGLELIST=0,
    TRIANGLESTRIP,
    LINELIST,
    LINESTRIP,
    POINTLIST
  };

  struct psVertObj
  {
    void* verts;
    void* indices;
    void* layout;
    unsigned int nvert; // Number of vertices 
    unsigned int nindice; // Number of indices
    unsigned int vsize; // Size of the vertices in this buffer
    PRIMITIVETYPE mode; // Mode to render in
  };

  struct psVertex
  {
    float x;
    float y;
    float z;
    unsigned int color;
  };

  enum USAGETYPES : unsigned int {
    USAGE_DEFAULT=0,
    USAGE_IMMUTABLE=1,
    USAGE_DYNAMIC=2,
    USAGE_STAGING=3,
    USAGE_AUTOGENMIPMAP=(1<<2),
    USAGE_TEXTURECUBE=(1<<3),
    USAGE_RENDERTARGET=(1<<4),
    USAGE_SHADER_RESOURCE=(1<<5),
    USAGE_DEPTH_STENCIL=(1<<6),
    USAGE_CONSTANT_BUFFER=(1<<7)
  };

  enum BUFFERTYPES : unsigned char {
    BUFFER_VERTEX=0,
    BUFFER_INDEX=1
  };

  enum FORMATS : unsigned char {
    FMT_UNKNOWN,
    FMT_A8R8G8B8,
    FMT_X8R8G8B8,
    FMT_R5G6B5,
    FMT_A1R5G5B5,
    FMT_A4R4G4B4,
    FMT_A8,
    FMT_A2B10G10R10,
    FMT_A8B8G8R8,
    FMT_G16R16,
    FMT_A16B16G16R16,
    FMT_V8U8,
    FMT_Q8W8V8U8,
    FMT_V16U16,
    FMT_R8G8_B8G8,
    FMT_G8R8_G8B8,
    FMT_DXT1,
    FMT_DXT2,
    FMT_DXT3,
    FMT_DXT4,
    FMT_DXT5,
    FMT_D16,
    FMT_D32F,
    FMT_S8D24,
    FMT_INDEX16,
    FMT_INDEX32,
    FMT_Q16W16V16U16,
    FMT_R16F,
    FMT_G16R16F,
    FMT_A16B16G16R16F,
    FMT_R32F,
    FMT_G32R32F,
    FMT_A32B32G32R32F,
    FMT_R8G8B8A8_UINT,
    FMT_R16G16_SINT,
    FMT_FORMAT_R16G16B16A16_SINT,
    FMT_R8G8B8A8_UNORM,
    FMT_R16G16_SNORM,
    FMT_R16G16B16A16_SNORM,
    FMT_R16G16_UNORM,
    FMT_R16G16B16A16_UNORM,
    FMT_R16G16_FLOAT,
    FMT_R16G16B16A16_FLOAT
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

  class BSS_COMPILER_DLLEXPORT psDriver
  {
  protected:
    inline psDriver(const psVeciu& Screendim) : screendim(Screendim) {}

  public:
    // Begins a scene
    virtual bool Begin()=0;
    // Ends a scene
    virtual void End()=0;
    // Draws a vertex object
    virtual void BSS_FASTCALL Draw(psVertObj* buf, float(&transform)[4][4], FLAG_TYPE flags)=0;
    // Draws a rectangle
    virtual void BSS_FASTCALL DrawRect(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned int numrects, FLAG_TYPE flags, const float xform[4][4])=0;
    virtual void BSS_FASTCALL DrawRectBatch(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], FLAG_TYPE flags)=0;
    virtual void DrawRectBatchEnd()=0;
    // Draws a circle
    virtual void BSS_FASTCALL DrawCircle()=0;
    // Draws a polygon
    virtual void BSS_FASTCALL DrawPolygon(const psVec* verts, FNUM Z, int num, unsigned long vertexcolor, FLAG_TYPE flags)=0;
    // Draws points (which are always batch rendered)
    virtual void BSS_FASTCALL DrawPointsBegin(const psTex* texture, float size, FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawPoints(psVertex* particles, unsigned int num)=0;
    virtual void DrawPointsEnd()=0;
    // Draws lines (which are also always batch rendered)
    virtual void BSS_FASTCALL DrawLinesStart(int num, FLAG_TYPE flags)=0;
    virtual void BSS_FASTCALL DrawLines(const cLineT<float>& line, float Z1, float Z2, unsigned long vertexcolor, FLAG_TYPE flags)=0;
    virtual void DrawLinesEnd()=0;
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void BSS_FASTCALL ApplyCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport)=0;
    virtual void BSS_FASTCALL ApplyCamera3D(float (&m)[4][4], const psRectiu& viewport)=0;
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad()=0;
    // Gets/Sets the extent
    virtual const psVec& GetExtent() const=0;
    virtual void BSS_FASTCALL SetExtent(float znear, float zfar)=0;
    // Creates a vertex or index buffer
    virtual void* BSS_FASTCALL CreateBuffer(unsigned short bytes, USAGETYPES usage, BUFFERTYPES type, void* initdata=0)=0;
    // Creates a texture
    virtual void* BSS_FASTCALL CreateTexture(psVeciu dim, FORMATS format, USAGETYPES usage=USAGE_SHADER_RESOURCE, unsigned char miplevels=0, void* initdata=0, void** additionalview=0)=0;
    virtual void* BSS_FASTCALL LoadTexture(const char* path, USAGETYPES usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, unsigned char miplevels=0, void** additionalview=0)=0;
    virtual void* BSS_FASTCALL LoadTextureInMemory(const void* data, size_t datasize, USAGETYPES usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, unsigned char miplevels=0, void** additionalview=0)=0;
    // Pushes or pops a scissor rect on to the stack
    virtual void BSS_FASTCALL PushScissorRect(const psRectl& rect)=0;
    virtual void PopScissorRect()=0;
    // Sets the current rendertargets, setting all the rest to null.
    virtual void BSS_FASTCALL SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil=0)=0;
    // Sets shader constants
    virtual void BSS_FASTCALL SetShaderConstants(void* constbuf, SHADER_VER shader)=0;
    // Sets textures for a given type of shader (in DX9 this is completely ignored)
    virtual void BSS_FASTCALL SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader=PIXEL_SHADER_1_1)=0;
    // Builds a stateblock from the given set of state changes
    virtual void* BSS_FASTCALL BuildStateblock(STATECHANGE* states);
    // Sets a given stateblock
    virtual void BSS_FASTCALL BuildStateblock(void* stateblock);
    // Frees a created resource of the specified type
    enum RESOURCE_TYPE : unsigned char { RES_TEXTURE, RES_SURFACE, RES_SHADERVS, RES_SHADERPS, RES_SHADERGS, RES_STATEBLOCK, RES_INDEXBUF, RES_VERTEXBUF, };
    virtual void BSS_FASTCALL FreeResource(void* p, RESOURCE_TYPE t)=0;
    // Gets a pointer to the driver implementation
    virtual RealDriver GetRealDriver()=0;

    // Compile a shader from a string
    virtual void* BSS_FASTCALL CompileShader(const char* source, SHADER_VER profile, const char* entrypoint="")=0;
    // Create an actual shader object from compiled shader source (either precompiled or from CompileShader())
    virtual void* BSS_FASTCALL CreateShader(const void* data, SHADER_VER profile)=0;
    // Sets current shader
    virtual char BSS_FASTCALL SetShader(void* shader, SHADER_VER profile)=0;
    // Returns true if shader version is supported
    virtual bool BSS_FASTCALL ShaderSupported(SHADER_VER profile)=0;

    BSS_FORCEINLINE static void _MatrixTranslate(float(&out)[4][4], float x, float y, float z) { out[3][0]=x; out[3][1]=y; out[3][2]=z; } //This is the transpose of what is NORMALLY done, presumably due to the order of multiplication
    BSS_FORCEINLINE static void _MatrixScale(float(&out)[4][4], float x, float y, float z) { out[0][0]=x; out[1][1]=y; out[2][2]=z; }
    BSS_FORCEINLINE static void _MatrixRotateZ(float(&out)[4][4], float angle) { float ca=cos(angle); float sa=sin(angle); out[0][0]=ca; out[1][0]=-sa; out[0][1]=sa; out[1][1]=ca; } //Again, we need the transpose
    BSS_FORCEINLINE static void BSS_FASTCALL _inversetransform(float(&mat)[4][4]) { mat[3][0]=(-mat[3][0]); mat[3][1]=(-mat[3][1]); }
    BSS_FORCEINLINE static void BSS_FASTCALL _inversetransformadd(float(&mat)[4][4], const float(&add)[4][4]) { mat[3][0]=add[3][0]-mat[3][0]; mat[3][1]=add[3][1]-mat[3][1]; mat[3][2]=add[3][2]; }

    psVeciu screendim;
  };

  class BSS_COMPILER_DLLEXPORT psDriverHold
  { 
  protected:
    static psDriver* _driver; //public pointer to the driver
  };
}

#endif