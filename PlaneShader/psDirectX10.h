// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __DIRECTX10_H__PS__
#define __DIRECTX10_H__PS__

#include "psDriver.h"
#include "bss-util/bss_stack.h"
#include "bss-util/bss_win32_includes.h"
#include "directx/D3D10.h"
#include "directx/D3DX10.h"

namespace planeshader {
  struct DX10_rectvert 
  {
    float x, y, z, rot;
    float w, h, pivot_x, pivot_y;
    float u, v, u2, v2;
    unsigned int color;
  };
  class psDirectX10 : public psDriver
  {
  public:
    // Constructors
    psDirectX10(const psVeciu& dim);
    ~psDirectX10();
    // Begins a scene
    virtual bool Begin();
    // Ends a scene
    virtual void End();
    // Draws a vertex object
    virtual void BSS_FASTCALL Draw(psVertObj* buf, float(&transform)[4][4], FLAG_TYPE flags);
    // Draws a rectangle
    virtual void BSS_FASTCALL DrawRect(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned int numrects, FLAG_TYPE flags, const float xform[4][4]);
    virtual void BSS_FASTCALL DrawRectBatch(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], FLAG_TYPE flags);
    virtual void DrawRectBatchEnd();
    // Draws a circle
    virtual void BSS_FASTCALL DrawCircle();
    // Draws a polygon
    virtual void BSS_FASTCALL DrawPolygon(const psVec* verts, FNUM Z, int num, unsigned long vertexcolor, FLAG_TYPE flags);
    // Draws points (which are always batch rendered)
    virtual void BSS_FASTCALL DrawPointsBegin(const psTex* texture, float size, FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawPoints(psVertex* particles, unsigned int num);
    virtual void DrawPointsEnd();
    // Draws lines (which are also always batch rendered)
    virtual void BSS_FASTCALL DrawLinesStart(int num, FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawLines(const cLineT<float>& line, float Z1, float Z2, unsigned long vertexcolor, FLAG_TYPE flags);
    virtual void DrawLinesEnd();
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void BSS_FASTCALL ApplyCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport);
    virtual void BSS_FASTCALL ApplyCamera3D(float(&m)[4][4], const psRectiu& viewport);
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad();
    // Gets/Sets the extent
    inline virtual const psVec& GetExtent() const { return _extent; }
    virtual void BSS_FASTCALL SetExtent(float znear, float zfar);
    // Creates a vertex or index buffer
    virtual void* BSS_FASTCALL CreateBuffer(unsigned short bytes, USAGETYPES usage, BUFFERTYPES type, void* initdata=0);
    // Creates a texture
    virtual void* BSS_FASTCALL CreateTexture(psVeciu dim, FORMATS format, USAGETYPES usage=USAGE_SHADER_RESOURCE, unsigned char miplevels=0, void* initdata=0, void** additionalview=0);
    virtual void* BSS_FASTCALL LoadTexture(const char* path, USAGETYPES usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, unsigned char miplevels=0, void** additionalview=0);
    virtual void* BSS_FASTCALL LoadTextureInMemory(const void* data, size_t datasize, USAGETYPES usage=USAGE_SHADER_RESOURCE, FORMATS format=FMT_UNKNOWN, unsigned char miplevels=0, void** additionalview=0);
    // Pushes or pops a scissor rect on to the stack
    virtual void BSS_FASTCALL PushScissorRect(const psRectl& rect);
    virtual void PopScissorRect();
    // Sets the rendertargets
    virtual void BSS_FASTCALL SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil=0);
    // Sets shader constants
    virtual void BSS_FASTCALL SetShaderConstants(void* constbuf, SHADER_VER shader);
    // Sets textures for a given type of shader (in DX9 this is completely ignored)
    virtual void BSS_FASTCALL SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader=PIXEL_SHADER_1_1);
    // Frees a created resource of the specified type
    virtual void BSS_FASTCALL FreeResource(void* p, RESOURCE_TYPE t);
    // Gets a pointer to the driver implementation
    inline virtual RealDriver GetRealDriver() { RealDriver d; d.dx10=this; d.type=RealDriver::DRIVERTYPE_DX10; return d; }

    // Compile a shader from a string
    virtual void* BSS_FASTCALL CompileShader(const char* source, SHADER_VER profile, const char* entrypoint="");
    // Create an actual shader object from compiled shader source (either precompiled or from CompileShader())
    void* BSS_FASTCALL CreateShader(const void* data, size_t datasize, SHADER_VER profile);
    virtual void* BSS_FASTCALL CreateShader(const void* data, SHADER_VER profile);
    // Sets current shader
    virtual char BSS_FASTCALL SetShader(void* shader, SHADER_VER profile);
    // Returns true if shader version is supported
    virtual bool BSS_FASTCALL ShaderSupported(SHADER_VER profile);

  protected:
    static inline unsigned int BSS_FASTCALL _usagetodxtype(USAGETYPES types);
    static inline unsigned int BSS_FASTCALL _usagetocpuflag(USAGETYPES types);
    static inline unsigned int BSS_FASTCALL _usagetomisc(USAGETYPES types);
    static inline unsigned int BSS_FASTCALL _usagetobind(USAGETYPES types);
    static inline unsigned int BSS_FASTCALL _buftypetodxtype(BUFFERTYPES types);
    static inline unsigned int BSS_FASTCALL _fmttodx10(FORMATS format);
    static inline unsigned int BSS_FASTCALL _getbitsperpixel(FORMATS format);
    static inline D3D10_PRIMITIVE_TOPOLOGY BSS_FASTCALL _getdx10topology(PRIMITIVETYPE type);
    void* _createshaderview(ID3D10Resource* src);
    void* _creatertview(ID3D10Resource* src);
    void* _createdepthview(ID3D10Resource* src);

    ID3D10Device* _device;
    D3DXMATRIX matProj;
    D3DXMATRIX matCamPos_NoScale;
    D3DXMATRIX matCamPos;
    D3DXMATRIX matCamRotate;
    D3DXMATRIX matCamRotPos;
    D3DXMATRIX matScaleZ;
    void* _fastvbuf;
    void* _rectvbuf;
    void* _batchvbuf;
    psVec _extent;
    bool _zerocamrot;
    bss_util::cStack<psRectl> _scissorstack;
    ID3D10VertexShader* _fsquadVS;
  };
}

#endif