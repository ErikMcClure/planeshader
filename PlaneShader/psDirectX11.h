// Copyright �2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __DIRECTX11_H__PS__
#define __DIRECTX11_H__PS__

#include "psDriver.h"
#include "psShader.h"
#include "bss-util/bss_stack.h"
#include "bss-util/bss_win32_includes.h"
#include "directx/D3D11.h"
#include "directx/D3DX11.h"
#include "psDXGI.h"

namespace planeshader {
  // Vertex input to geometry shader for rect rendering (UV coordinates are added in batches of 4 floats after the end)
  struct DX11_rectvert
  {
    float x, y, z, rot;
    float w, h, pivot_x, pivot_y;
    unsigned int color;
  };
  // Vertex used for polygons, lines and point rendering
  struct DX11_simplevert
  {
    float x, y, z, w;
    unsigned int color;
  };
  struct DX11_SB
  {
    ID3D11RasterizerState* rs;
    ID3D11DepthStencilState* ds;
    ID3D11BlendState* bs;
    UINT stencilref;
    FLOAT blendfactor[4];
    UINT sampleMask;
  };

  // Represents a fake view so we can have a consistent method of accessing resources even if they aren't bound to anything.
  struct DX11_EmptyView : ID3D11View
  {
    DX11_EmptyView(ID3D11Resource* res) : _res(res), _ref(1) {}
    ~DX11_EmptyView() { if(_res) _res->Release(); }
    void STDMETHODCALLTYPE GetResource(ID3D11Resource **ppResource) { *ppResource = _res; }
    void STDMETHODCALLTYPE GetDevice(ID3D11Device **ppDevice) { *ppDevice = 0; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void *pData) { return E_NOTIMPL; }
    ULONG STDMETHODCALLTYPE AddRef() { return ++_ref; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { return !ppvObject ? E_POINTER : E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE Release() { ULONG r = --_ref; if(_ref <= 0) delete this; return r; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID  guid, const IUnknown *pData) { return E_NOTIMPL; }

    ID3D11Resource* _res;
    ULONG _ref;
  };

  class psDirectX11 : public psDriver, public psDriverHold, public psDXGI
  {
  public:
    // Constructors
    psDirectX11(const psVeciu& dim, unsigned int antialias, bool vsync, bool fullscreen, bool sRGB, HWND hwnd);
    ~psDirectX11();
    // Begins a scene
    virtual bool Begin();
    // Ends a scene
    virtual char End();
    // Draws a vertex object
    virtual void BSS_FASTCALL Draw(psVertObj* buf, FLAG_TYPE flags, const float(&transform)[4][4] = identity);
    // Draws a rectangle
    virtual void BSS_FASTCALL DrawRect(const psRectRotateZ rect, const psRect* uv, unsigned char numuv, unsigned int color, const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags, const float(&xform)[4][4] = identity);
    virtual void BSS_FASTCALL DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned char numuv, FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawRectBatch(const psRectRotateZ rect, const psRect* uv, unsigned int color, const float(&xform)[4][4] = identity);
    virtual void DrawRectBatchEnd(const float(&xform)[4][4] = identity);
    // Draws a polygon
    virtual void BSS_FASTCALL DrawPolygon(const psVec* verts, int num, psVec3D offset, unsigned long vertexcolor, FLAG_TYPE flags, const float(&transform)[4][4] = identity);
    virtual void BSS_FASTCALL DrawPolygon(const psVertex* verts, int num, FLAG_TYPE flags, const float(&transform)[4][4] = identity);
    // Draws points (which are always batch rendered)
    virtual void BSS_FASTCALL DrawPointsBegin(const psTex* const* texes, unsigned char numtex, float size, FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawPoints(psVertex* particles, unsigned int num);
    virtual void DrawPointsEnd();
    // Draws lines (which are also always batch rendered)
    virtual void BSS_FASTCALL DrawLinesStart(FLAG_TYPE flags);
    virtual void BSS_FASTCALL DrawLines(const psLine& line, float Z1, float Z2, unsigned long vertexcolor, const float(&transform)[4][4] = identity);
    virtual void DrawLinesEnd(const float(&transform)[4][4] = identity);
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void BSS_FASTCALL PushCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport);
    virtual void BSS_FASTCALL PushCamera3D(const float(&m)[4][4], const psRectiu& viewport);
    virtual void BSS_FASTCALL PopCamera();
    // Applies the camera transform (or it's inverse) according to the flags to a point.
    psVec3D BSS_FASTCALL TransformPoint(const psVec3D& point, FLAG_TYPE flags) const;
    psVec3D BSS_FASTCALL ReversePoint(const psVec3D& point, FLAG_TYPE flags) const;
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad();
    // Gets/Sets the extent
    inline virtual const psVec& GetExtent() const { return _extent; }
    virtual void BSS_FASTCALL SetExtent(float znear, float zfar);
    // Creates a vertex or index buffer
    virtual void* BSS_FASTCALL CreateBuffer(size_t bytes, unsigned int usage, const void* initdata = 0);
    virtual void* BSS_FASTCALL LockBuffer(void* target, unsigned int flags);
    virtual void BSS_FASTCALL UnlockBuffer(void* target);
    virtual void* BSS_FASTCALL LockTexture(void* target, unsigned int flags, unsigned int& pitch, unsigned char miplevel = 0);
    virtual void BSS_FASTCALL UnlockTexture(void* target, unsigned char miplevel = 0);
    // Creates a texture
    virtual void* BSS_FASTCALL CreateTexture(psVeciu dim, FORMATS format, unsigned int usage = USAGE_SHADER_RESOURCE, unsigned char miplevels = 0, const void* initdata = 0, void** additionalview = 0, psTexblock* texblock = 0);
    virtual void* BSS_FASTCALL LoadTexture(const char* path, unsigned int usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, unsigned char miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false);
    virtual void* BSS_FASTCALL LoadTextureInMemory(const void* data, size_t datasize, unsigned int usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, unsigned char miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false);
    virtual void BSS_FASTCALL CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, unsigned char miplevel = 0);
    // Pushes or pops a scissor rect on to the stack
    virtual void BSS_FASTCALL PushScissorRect(const psRect& rect);
    virtual void PopScissorRect();
    // Sets the rendertargets
    virtual void BSS_FASTCALL SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil = 0);
    // Sets shader constants
    virtual void BSS_FASTCALL SetShaderConstants(void* constbuf, SHADER_VER shader);
    // Sets textures for a given type of shader (in DX9 this is completely ignored)
    virtual void BSS_FASTCALL SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader = PIXEL_SHADER_1_1);
    // Builds a stateblock from the given set of state changes
    virtual void* BSS_FASTCALL CreateStateblock(const STATEINFO* states, unsigned int count);
    // Builds a texblock from the given set of sampler states
    virtual void* BSS_FASTCALL CreateTexblock(const STATEINFO* states, unsigned int count);
    // Sets a given stateblock
    virtual void BSS_FASTCALL SetStateblock(void* stateblock);
    // Create a vertex layout from several element descriptions
    virtual void* BSS_FASTCALL CreateLayout(void* shader, const ELEMENT_DESC* elements, unsigned char num);
    void* BSS_FASTCALL CreateLayout(void* shader, size_t sz, const ELEMENT_DESC* elements, unsigned char num);
    virtual void BSS_FASTCALL SetLayout(void* layout);
    // Frees a created resource of the specified type
    virtual TEXTURE_DESC BSS_FASTCALL GetTextureDesc(void* t);
    virtual void BSS_FASTCALL FreeResource(void* p, RESOURCE_TYPE t);
    virtual void BSS_FASTCALL GrabResource(void* p, RESOURCE_TYPE t);
    virtual void BSS_FASTCALL CopyResource(void* dest, void* src, RESOURCE_TYPE t);
    virtual void BSS_FASTCALL Resize(psVeciu dim, FORMATS format, char fullscreen);
    virtual void BSS_FASTCALL Clear(unsigned int color);
    // Gets a pointer to the driver implementation
    inline virtual RealDriver GetRealDriver() { RealDriver d; d.dx11 = this; d.type = RealDriver::DRIVERTYPE_DX11; return d; }
    inline virtual psTex* GetBackBuffer() { return _backbuffer; }
    // Compile a shader from a string
    virtual void* BSS_FASTCALL CompileShader(const char* source, SHADER_VER profile, const char* entrypoint = "");
    // Create an actual shader object from compiled shader source (either precompiled or from CompileShader())
    void* BSS_FASTCALL CreateShader(const void* data, size_t datasize, SHADER_VER profile);
    virtual void* BSS_FASTCALL CreateShader(const void* data, SHADER_VER profile);
    // Sets current shader
    virtual char BSS_FASTCALL SetShader(void* shader, SHADER_VER profile);
    // Returns true if shader version is supported
    virtual bool BSS_FASTCALL ShaderSupported(SHADER_VER profile);
    // Gets/Sets the effective DPI
    virtual void SetDPI(psVeciu dpi = psVeciu(BASE_DPI));
    virtual psVeciu GetDPI();

    inline long GetLastError() const { return _lasterr; }

    static const int BATCHSIZE = 512;
    static const int RECTBUFSIZE = sizeof(DX11_rectvert)*(BATCHSIZE + (BATCHSIZE / 2));

    static void* operator new(std::size_t sz);
    static void operator delete(void* ptr, std::size_t sz);

    struct CamDef { bss_util::Matrix<float, 4, 4> viewproj; bss_util::Matrix<float, 4, 4> proj; psRectiu viewport; };

  protected:
    static inline unsigned int BSS_FASTCALL _usagetodxtype(unsigned int types);
    static inline unsigned int BSS_FASTCALL _usagetocpuflag(unsigned int types);
    static inline unsigned int BSS_FASTCALL _usagetomisc(unsigned int types, bool multisampled);
    static inline unsigned int BSS_FASTCALL _usagetobind(unsigned int types);
    static inline unsigned int BSS_FASTCALL _filtertodx11(FILTERS filter);
    static inline unsigned int BSS_FASTCALL _reverseusage(unsigned int usage, unsigned int misc, unsigned int bind, bool multisample); //reassembles DX11 flags into a generic usage flag
    static inline const char* BSS_FASTCALL _geterror(HRESULT err);
    static inline D3D11_PRIMITIVE_TOPOLOGY BSS_FASTCALL _getdx11topology(PRIMITIVETYPE type);
    static void BSS_FASTCALL _loadtexture(D3DX11_IMAGE_LOAD_INFO* info, unsigned int usage, FORMATS format, unsigned char miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, bool sRGB);
    static ID3D11Resource* _textotex2D(void* t);
    static bool _customfilter(FILTERS filter);
    psShader* _getfiltershader(FILTERS filter);
    void _setcambuf(ID3D11Buffer* buf, const float(&cam)[4][4], const float(&world)[4][4]);
    ID3D11View* _createshaderview(ID3D11Resource* src);
    ID3D11View* _creatertview(ID3D11Resource* src);
    ID3D11View* _createdepthview(ID3D11Resource* src);
    long _lasterr;
    void _processdebugqueue();
    void _getbackbufferref();
    void _resetscreendim();
    void _applycamera();
    void _applyrendertargets();
    void BSS_FASTCALL _applyloadshader(ID3D11Texture2D* tex, psShader* shader, bool sRGB);
    void BSS_FASTCALL _applymipshader(ID3D11Texture2D* tex, psShader* shader);
    void BSS_FASTCALL _processdebugmessage(UINT64 index, SIZE_T len);
    psVeciu _getrendertargetsize(ID3D11RenderTargetView* view);

    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    IDXGISwapChain* _swapchain;
    D3D_FEATURE_LEVEL _featurelevel;
    psTex* _backbuffer;
    void* _rectvertbuf;
    void* _batchvertbuf;
    void* _batchindexbuf;
    psVertObj _batchobjbuf;
    psVertObj _ptobjbuf;
    psVertObj _lineobjbuf;
    psVertObj _rectobjbuf;
    DX11_rectvert* _lockedrectbuf;
    DX11_simplevert* _lockedptbuf;
    DX11_simplevert* _lockedlinebuf;
    unsigned int _lockedcount;
    unsigned int _lockedrectuv;
    FLAG_TYPE _lockedflag;
    psVec _extent;
    psVeciu _dpi;
    bool _zerocamrot;
    bool _vsync;
    bss_util::cStack<psRectl> _scissorstack;
    bss_util::cStack<CamDef> _camstack;
    bss_util::cDynArray<const psTex*> _lastrt;
    const psTex* _lastdepth;
    ID3D11VertexShader* _fsquadVS;
    ID3D11VertexShader* _defaultVS;
    ID3D11VertexShader* _lastVS;
    ID3D11GeometryShader* _lastGS;
    ID3D11PixelShader* _lastPS;
    ID3D11PixelShader* _defaultPS; //single texture pixel shader
    ID3D11SamplerState* _defaultSS; //default sampler state
    DX11_SB* _defaultSB; // default stateblock
    ID3D11Buffer* _cam_def; //viewproj matrix and identity world matrix
    ID3D11Buffer* _cam_usr; //viewproj matrix and custom world matrix
    ID3D11Buffer* _proj_def; //proj matrix and identity world matrix
    ID3D11Buffer* _proj_usr; //proj matrix and custom world matrix
    ID3D11InfoQueue * _infoqueue;
    psShader* _alphaboxfilter;
    psShader* _premultiplyfilter;
  };
}

#endif