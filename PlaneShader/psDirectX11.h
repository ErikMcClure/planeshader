// Copyright ©2016 Black Sphere Studios
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
#include <array>

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
    // Flush draw buffer
    virtual void BSS_FASTCALL Flush();
    virtual psBatchObj& BSS_FASTCALL FlushPreserve();
    // Draws a vertex object
    virtual void BSS_FASTCALL Draw(psVertObj* buf, psFlag flags, const float(&transform)[4][4] = identity);
    // Draws a rectangle
    virtual psBatchObj& BSS_FASTCALL DrawRect(psShader* shader, const psStateblock* stateblock, const psRectRotateZ& rect, const psRect* uv, unsigned char numuv, unsigned int color, psFlag flags, const float(&xform)[4][4] = identity);
    virtual psBatchObj& BSS_FASTCALL DrawRectBatchBegin(psShader* shader, const psStateblock* stateblock, unsigned char numuv, psFlag flags, const float(&xform)[4][4] = identity);
    virtual void BSS_FASTCALL DrawRectBatch(psBatchObj& o, const psRectRotateZ& rect, const psRect* uv, unsigned int color);
    // Draws a polygon
    virtual psBatchObj& BSS_FASTCALL DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVec* verts, uint32_t num, psVec3D offset, unsigned long vertexcolor, psFlag flags, const float(&transform)[4][4] = identity);
    virtual psBatchObj& BSS_FASTCALL DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVertex* verts, uint32_t num, psFlag flags, const float(&transform)[4][4] = identity);
    // Draws points
    virtual psBatchObj& BSS_FASTCALL DrawPoints(psShader* shader, const psStateblock* stateblock, psVertex* particles, uint32_t num, psFlag flags, const float(&transform)[4][4] = identity);
    // Draws lines
    virtual psBatchObj& BSS_FASTCALL DrawLinesStart(psShader* shader, const psStateblock* stateblock, psFlag flags, const float(&xform)[4][4] = identity);
    virtual void BSS_FASTCALL DrawLines(psBatchObj& obj, const psLine& line, float Z1, float Z2, unsigned long vertexcolor);
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void BSS_FASTCALL PushCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport, const psVec& extent);
    virtual void BSS_FASTCALL PushCamera3D(const float(&m)[4][4], const psRectiu& viewport);
    virtual void BSS_FASTCALL PopCamera();
    // Applies the camera transform (or it's inverse) according to the flags to a point.
    psVec3D BSS_FASTCALL TransformPoint(const psVec3D& point, psFlag flags) const;
    psVec3D BSS_FASTCALL ReversePoint(const psVec3D& point, psFlag flags) const;
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad();
    // Creates a vertex or index buffer
    virtual void* BSS_FASTCALL CreateBuffer(uint32_t capacity, uint32_t element, unsigned int usage, const void* initdata);
    virtual void* BSS_FASTCALL LockBuffer(void* target, unsigned int flags);
    virtual void BSS_FASTCALL UnlockBuffer(void* target);
    virtual void* BSS_FASTCALL LockTexture(void* target, unsigned int flags, unsigned int& pitch, unsigned char miplevel = 0);
    virtual void BSS_FASTCALL UnlockTexture(void* target, unsigned char miplevel = 0);
    // Creates a texture
    virtual void* BSS_FASTCALL CreateTexture(psVeciu dim, FORMATS format, unsigned int usage = USAGE_SHADER_RESOURCE, unsigned char miplevels = 0, const void* initdata = 0, void** additionalview = 0, psTexblock* texblock = 0);
    virtual void* BSS_FASTCALL LoadTexture(const char* path, unsigned int usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, unsigned char miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false);
    virtual void* BSS_FASTCALL LoadTextureInMemory(const void* data, size_t datasize, unsigned int usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, unsigned char miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false);
    virtual void BSS_FASTCALL CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, unsigned char miplevel = 0);
    // Pushes or pops a clip rect on to the stack
    virtual void BSS_FASTCALL PushClipRect(const psRect& rect);
    virtual psRect PeekClipRect();
    virtual void PopClipRect();
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
    // Returns an index to an internal state snapshot
    virtual unsigned int BSS_FASTCALL GetSnapshot();

    inline long GetLastError() const { return _lasterr; }

    static const int BATCHSIZE = 512;
    static const int RECTBUFSIZE = BATCHSIZE + (BATCHSIZE / 2);

    static void* operator new(std::size_t sz);
    static void operator delete(void* ptr, std::size_t sz);

    struct CamDef { bss_util::Matrix<float, 4, 4> viewproj; bss_util::Matrix<float, 4, 4> proj; psRectiu viewport; };
    struct Snapshot { uint32_t tex[3]; uint32_t ntex[3]; uint32_t rt; uint32_t nrt; ID3D11DepthStencilView* depth; psRectl cliprect; };

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
    void _processdebugqueue();
    void _getbackbufferref();
    void _resetscreendim();
    void _applycamera();
    void _applyrendertargets();
    void _applysnapshot(const Snapshot& s);
    void BSS_FASTCALL _applyloadshader(ID3D11Texture2D* tex, psShader* shader, bool sRGB);
    void BSS_FASTCALL _applymipshader(ID3D11Texture2D* tex, psShader* shader);
    void BSS_FASTCALL _processdebugmessage(UINT64 index, SIZE_T len);
    psVeciu _getrendertargetsize(ID3D11RenderTargetView* view);
    bool _checksnapshot(Snapshot& s);
    psBatchObj& _checkflush(psBatchObj& obj, uint32_t num);

    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    IDXGISwapChain* _swapchain;
    D3D_FEATURE_LEVEL _featurelevel;
    psTex* _backbuffer;
    psBufferObj _rectvertbuf;
    psBufferObj _batchvertbuf;
    psBufferObj _batchindexbuf;
    psVec _extent;
    psVeciu _dpi;
    bool _zerocamrot;
    bool _vsync;
    bss_util::cStack<psRectl> _clipstack;
    bss_util::cStack<CamDef> _camstack;
    bss_util::cDynArray<Snapshot, uint32_t> _snapshotstack;
    bss_util::cDynArray<void*, uint32_t> _texstack;
    std::array<bss_util::cDynArray<ID3D11Texture2D*, uint32_t>,3> _lasttex;
    bss_util::cDynArray<ID3D11RenderTargetView*, uint32_t> _lastrt;
    ID3D11DepthStencilView* _lastdepth;
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
    long _lasterr;
  };
}

#endif