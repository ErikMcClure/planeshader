// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDirectX11.h"
#include "psDirectX11_fsquadVS.h"
#include "psEngine.h"
#include "psTex.h"
#include "psStateblock.h"
#include "bss-util/cStr.h"
#include "bss-util/cDynArray.h"
#include "bss-util/profiler.h"
#include "psColor.h"
#include "psCamera.h"

using namespace planeshader;
using namespace bss_util;

#ifdef BSS_CPU_x86_64
#pragma comment(lib, "../lib/dxlib/x64/d3d11.lib")
#pragma comment(lib, "../lib/dxlib/x64/dxguid.lib")
#pragma comment(lib, "../lib/dxlib/x64/dxgi.lib")
#pragma comment(lib, "../lib/dxlib/x64/DxErr.lib")
#ifdef BSS_DEBUG
#pragma comment(lib, "../lib/dxlib/x64/d3dx11d.lib")
#else
#pragma comment(lib, "../lib/dxlib/x64/d3dx11.lib")
#endif
#else
#pragma comment(lib, "../lib/dxlib/d3d11.lib")
#pragma comment(lib, "../lib/dxlib/dxguid.lib")
#pragma comment(lib, "../lib/dxlib/DxErr.lib")
#pragma comment(lib, "../lib/dxlib/dxgi.lib")
#ifdef BSS_DEBUG
#pragma comment(lib, "../lib/dxlib/d3dx11d.lib")
#else
#pragma comment(lib, "../lib/dxlib/d3dx11.lib")
#endif
#endif

#ifdef BSS_DEBUG
#define PROCESSQUEUE() _processdebugqueue()
#else
#define PROCESSQUEUE()
#endif

#define LOGEMPTY  
//#define LOGFAILURERET(fn,rn,errmsg,...) { HRESULT hr = fn; if(FAILED(hr)) { return rn; } }
#define LOGFAILURERET(fn,rn,...) { _lasterr = (fn); if(FAILED(_lasterr)) { PSLOGV(1,__VA_ARGS__); PROCESSQUEUE(); return rn; } }
#define LOGFAILURE(fn,...) LOGFAILURERET(fn,LOGEMPTY,__VA_ARGS__)
#define LOGFAILURERETNULL(fn,...) LOGFAILURERET(fn,0,__VA_ARGS__)

// Constructors
psDirectX11::psDirectX11(const psVeciu& dim, uint32_t antialias, bool vsync, bool fullscreen, bool sRGB, HWND hwnd) : psDriver(dim), _device(0), _vsync(vsync), _lasterr(0),
_backbuffer(0), _dpiscale(1.0f), _infoqueue(0), _lastdepth(0)
{
  PROFILE_FUNC();
  memset(&library, 0, sizeof(SHADER_LIBRARY));

  _cam_def = 0;
  _cam_usr = 0;
  _proj_def = 0;
  _proj_usr = 0;
  _fsquadVS = 0;
  _defaultSB = 0;
  _defaultSS = 0;
  _driver = this;

#ifdef BSS_DEBUG
  const UINT DEVICEFLAGS = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
#else
  const UINT DEVICEFLAGS = D3D11_CREATE_DEVICE_SINGLETHREADED;
#endif

  IDXGIOutput* output;
  IDXGIAdapter* adapter = _createfactory(hwnd, output);

  PSLOG(4) << "DEVICEFLAGS: " << DEVICEFLAGS << std::endl;

  LOGFAILURE(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, DEVICEFLAGS, NULL, 0, D3D11_SDK_VERSION, &_device, &_featurelevel, &_context), "D3D11CreateDevice failed with error: ", _lasterr);
  const char* strfeature = "Unknown feature level";
  switch(_featurelevel)
  {
  case D3D_FEATURE_LEVEL_9_1: strfeature = "DirectX 9.1"; break;
  case D3D_FEATURE_LEVEL_9_2: strfeature = "DirectX 9.2"; break;
  case D3D_FEATURE_LEVEL_9_3: strfeature = "DirectX 9.3"; break;
  case D3D_FEATURE_LEVEL_10_0: strfeature = "DirectX 10.0"; break;
  case D3D_FEATURE_LEVEL_10_1: strfeature = "DirectX 10.1"; break;
  case D3D_FEATURE_LEVEL_11_0: strfeature = "DirectX 11.0"; break;
  }

  PSLOG(3) << "DX11 Feature Level: " << strfeature << std::endl;

#ifdef BSS_DEBUG
  _device->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)&_infoqueue);
#endif

  DXGI_MODE_DESC target = {
    dim.x,
    dim.y,
    { 0, 0 },
    sRGB?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
    DXGI_MODE_SCALING_UNSPECIFIED
  };
  
  DXGI_MODE_DESC match = { 0 };
  if(FAILED(_lasterr = output->FindClosestMatchingMode(&target, &match, _device)))
  {
    PSLOGV(2, "FindClosestMatchingMode failed with error ", _geterror(_lasterr), ", using target instead");
    memcpy(&match, &target, sizeof(DXGI_MODE_DESC));
  }

  UINT quality = 0;
  while(antialias > 0)
  {
    if(SUCCEEDED(_device->CheckMultisampleQualityLevels(match.Format, (1 << antialias), &quality)))
    {
      if(quality > 0)
      {
        --quality;
        break;
      }
    }
    --antialias;
  }

  _samples.Count = (1 << antialias);
  _samples.Quality = quality;

  DXGI_SWAP_CHAIN_DESC swapdesc = {
    match,
    _samples,
    DXGI_USAGE_RENDER_TARGET_OUTPUT,
    1,
    hwnd,
    !fullscreen,
    DXGI_SWAP_EFFECT_DISCARD,
    0
  };

  PSLOG(4) << cStrF("Creating swap chain with: { { %i, %i, { %i, %i }, %i, %i, %i }, { %i, %i }, %i, %i, %p, ",
    swapdesc.BufferDesc.Width,
    swapdesc.BufferDesc.Height,
    swapdesc.BufferDesc.RefreshRate,
    swapdesc.BufferDesc.Format,
    swapdesc.BufferDesc.ScanlineOrdering,
    swapdesc.BufferDesc.Scaling,
    swapdesc.SampleDesc.Count,
    swapdesc.SampleDesc.Quality,
    swapdesc.BufferUsage,
    swapdesc.BufferCount,
    swapdesc.OutputWindow).c_str()
    << (swapdesc.Windowed ? "true" : "false")

    << cStrF(", %i, %i }",
    swapdesc.SwapEffect,
    swapdesc.Flags).c_str() << std::endl;

  LOGFAILURE(_factory->CreateSwapChain(_device, &swapdesc, &_swapchain), "CreateSwapChain failed with error: ", _geterror(_lasterr));
  _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

  _cam_def = (ID3D11Buffer*)CreateBuffer(4 * 4 * 2, sizeof(float), USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC, 0);
  _cam_usr = (ID3D11Buffer*)CreateBuffer(4 * 4 * 2, sizeof(float), USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC, 0);
  _proj_def = (ID3D11Buffer*)CreateBuffer(4 * 4 * 2, sizeof(float), USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC, 0);
  _proj_usr = (ID3D11Buffer*)CreateBuffer(4 * 4 * 2, sizeof(float), USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC, 0);
  _clipstack.Push(RECT_ZERO); //push the initial clip rect (it'll get set in PushCamera, inside _resetscreendim)

  _getbackbufferref();
  _resetscreendim();

  _fsquadVS = (ID3D11VertexShader*)CreateShader(fsquadVS_main, sizeof(fsquadVS_main), VERTEX_SHADER_4_0);

  auto fnload = [](const char* file) -> std::unique_ptr<char[]>
  {
    FILE* f = 0;
    FOPEN(f, file, "rb");
    fseek(f, 0, SEEK_END);
    long ln = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::unique_ptr<char[]> a(new char[ln + 1]);
    fread(a.get(), 1, ln, f);
    fclose(f);
    a[ln] = 0;
    return a;
  };

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage.hlsl");

    ELEMENT_DESC desc[4] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint32_t)-1 }
    };

    library.IMAGE = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage0.hlsl");

    ELEMENT_DESC desc[3] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 }
    };

    library.IMAGE0 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage2.hlsl");

    ELEMENT_DESC desc[5] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32G32B32A32F, 0, (uint32_t)-1 }
    };

    library.IMAGE2 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage3.hlsl");

    ELEMENT_DESC desc[6] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 3, FMT_R32G32B32A32F, 0, (uint32_t)-1 }
    };

    library.IMAGE3 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fstext.hlsl");

    library.TEXT1 = psShader::MergeShaders(2, library.IMAGE, psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0)));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsdebug.hlsl");

    library.DEBUG = psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsalphabox.hlsl");

    _alphaboxfilter = psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fspremultiply.hlsl");

    _premultiplyfilter = psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsline.hlsl");

    ELEMENT_DESC desc[2] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 }
    };

    library.LINE = psShader::CreateShader(desc, 2,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
    library.POLYGON = library.LINE;
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fspoint.hlsl");

    ELEMENT_DESC desc[2] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 }
    };

    library.PARTICLE = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<float>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/curve.hlsl");

    ELEMENT_DESC desc[5] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 }
    };

    library.CURVE = psShader::CreateShader(desc, 2,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsrectround.hlsl");

    ELEMENT_DESC desc[6] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint32_t)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32F, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint32_t)-1 },
      { ELEMENT_COLOR, 1, FMT_R8G8B8A8, 0, (uint32_t)-1 }
    };

    library.ROUNDRECT = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<float>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fscircle.hlsl");

    library.CIRCLE = psShader::MergeShaders(2, library.ROUNDRECT, psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0)));
  }

  CreateBufferObj(&_rectvertbuf, RECTBUFSIZE, sizeof(DX11_rectvert), USAGE_VERTEX | USAGE_DYNAMIC, 0);

  uint16_t ind[BATCHSIZE * 3];
  uint32_t k = 0;
  for(uint16_t i = 0; i < BATCHSIZE - 3; ++i)
  {
    ind[k++] = 0;
    ind[k++] = 1 + i;
    ind[k++] = 2 + i;
  }
  CreateBufferObj(&_batchindexbuf, BATCHSIZE * 3, sizeof(uint16_t), USAGE_INDEX | USAGE_IMMUTABLE, ind);
  CreateBufferObj(&_batchvertbuf, BATCHSIZE, sizeof(DX11_simplevert), USAGE_VERTEX | USAGE_DYNAMIC, 0);

  // Create default stateblock and sampler state, then activate the default stateblock.
  _defaultSB = (DX11_SB*)CreateStateblock(0, 0);
  _defaultSS = (ID3D11SamplerState*)CreateTexblock(0, 0);
  SetStateblock(NULL);
  Clear(0x00000000);
  _swapchain->Present(0, 0);
}
psDirectX11::~psDirectX11()
{
  PROFILE_FUNC();
  PROCESSQUEUE(); // Because many errors happen during resource deallocation, process any messages we currently have before cleaning up

  if(_cam_def)
    FreeResource(_cam_def, RES_CONSTBUF);
  if(_cam_usr)
    FreeResource(_cam_usr, RES_CONSTBUF);
  if(_proj_def)
    FreeResource(_proj_def, RES_CONSTBUF);
  if(_proj_usr)
    FreeResource(_proj_usr, RES_CONSTBUF);
  if(_rectvertbuf.buffer)
    FreeResource(_rectvertbuf.buffer, RES_VERTEXBUF);
  if(_batchindexbuf.buffer)
    FreeResource(_batchindexbuf.buffer, RES_INDEXBUF);
  if(_batchvertbuf.buffer)
    FreeResource(_batchvertbuf.buffer, RES_VERTEXBUF);
  if(_fsquadVS)
    FreeResource(_fsquadVS, RES_SHADERVS);
  if(_defaultSB)
    FreeResource(_defaultSB, RES_STATEBLOCK);
  if(_defaultSS)
    FreeResource(_defaultSS, RES_TEXBLOCK);
  if(library.IMAGE)
    library.IMAGE->Drop();
  if(library.PARTICLE)
    library.PARTICLE->Drop();

  if(_backbuffer)
    delete _backbuffer;

  PROCESSQUEUE();
  int r;
  if(_device)
    r = _device->Release();
  if(_factory)
    r = _factory->Release();
}
void* psDirectX11::operator new(std::size_t sz) { return _aligned_malloc(sz, 16); }
void psDirectX11::operator delete(void* ptr, std::size_t sz) { _aligned_free(ptr); }
bool psDirectX11::Begin()
{
  PROCESSQUEUE();
  return true;
}
char psDirectX11::End()
{
  PROFILE_FUNC();
  Flush();
  HRESULT hr = _swapchain->Present(_vsync, 0); // DXGI_PRESENT_DO_NOT_SEQUENCE doesn't seem to do anything other than break everything.

  PROCESSQUEUE();

  switch(hr)
  {
  case DXGI_ERROR_DEVICE_RESET:
    return -1;
  case DXGI_ERROR_DEVICE_REMOVED:
    //case D3DDDIERR_DEVICEREMOVED:
    return -2;
  case DXGI_STATUS_OCCLUDED:
    return 1;
  }
  return 0;
}
void BSS_FASTCALL psDirectX11::Flush()
{
  for(uint32_t i = 0; i < _jobstack.Length(); ++i)
  {
    psBatchObj& obj = _jobstack[i];
    obj.shader->Activate();
    SetStateblock(obj.stateblock);
    _applysnapshot(_snapshotstack[obj.snapshot]);

    if(obj.buffer.verts->mem != 0)
    {
      UnlockBuffer(obj.buffer.verts->buffer);
      obj.buffer.verts->mem = 0;
      obj.buffer.verts->length = 0;
    }
    if(obj.buffer.indices && obj.buffer.indices->mem != 0)
    {
      UnlockBuffer(obj.buffer.indices->buffer);
      obj.buffer.indices->mem = 0;
      obj.buffer.indices->length = 0;
    }

    if(obj.buffer.nvert > 0)
      Draw(&obj.buffer, obj.flags, obj.transform);
  }

  _jobstack.Clear();
  _snapshotstack.Clear();
  _texstack.Clear();
}
psBatchObj* BSS_FASTCALL psDirectX11::FlushPreserve()
{
  assert(_jobstack.Length() > 0);

  psBatchObj obj(_jobstack.Back());
  //MEMCPY(&obj, sizeof(psBatchObj), &, sizeof(psBatchObj));
  Flush();
  return DrawBatchBegin(obj.shader, obj.stateblock, obj.flags, obj.buffer.verts, obj.buffer.indices, obj.buffer.mode, obj.transform);
}

void BSS_FASTCALL psDirectX11::Draw(psVertObj* buf, psFlag flags, const float(&transform)[4][4])
{
  static ID3D11Buffer* lastvert = 0;
  PROFILE_FUNC();
  uint32_t offset = 0;

  //if(buf->verts->buffer != lastvert)
    _context->IASetVertexBuffers(0, 1, &(lastvert = (ID3D11Buffer*)buf->verts->buffer), &buf->verts->element, &offset);
  _context->IASetPrimitiveTopology(_getdx11topology(buf->mode));

  ID3D11Buffer* cam = (flags&PSFLAG_FIXED) ? _proj_def : _cam_def;
  if(transform != identity)
    _setcambuf(cam = ((flags&PSFLAG_FIXED) ? _proj_usr : _cam_usr), _camstack.Peek().viewproj.v, transform);

  _context->VSSetConstantBuffers(0, 1, (ID3D11Buffer**)&cam);
  _context->GSSetConstantBuffers(0, 1, (ID3D11Buffer**)&cam);

  if(!buf->indices)
    _context->Draw(buf->nvert, buf->vert);
  else
  {
    _context->IASetIndexBuffer((ID3D11Buffer*)buf->indices->buffer, (buf->indices->element==2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    _context->DrawIndexed(buf->nindice, buf->indice, buf->vert);
  }
}

psBatchObj* BSS_FASTCALL psDirectX11::DrawRect(psShader* shader, const psStateblock* stateblock, const psRectRotateZ& rect, const psRect* uv, uint8_t numuv, uint32_t color, psFlag flags, const float(&xform)[4][4])
{ // Because we have to send the rect position in SOMEHOW and DX11 forces us to send matrices through the shaders, we will lock a buffer no matter what we do. 
  PROFILE_FUNC();
  psBatchObj* obj = DrawRectBatchBegin(shader, stateblock, numuv, flags, xform);
  DrawRectBatch(obj, rect, uv, color);
  return obj;
}
psBatchObj* BSS_FASTCALL psDirectX11::DrawRectBatchBegin(psShader* shader, const psStateblock* stateblock, uint8_t numuv, psFlag flags, const float(&xform)[4][4])
{
  uint32_t size = sizeof(DX11_rectvert) + sizeof(psRect)*numuv;
  if(size != _rectvertbuf.element)
  {
    Flush();
    _rectvertbuf.element = size;
  }

  return DrawBatchBegin(shader, !stateblock ? 0 : stateblock->GetSB(), flags, &_rectvertbuf, 0, POINTLIST, xform);
}
psBatchObj* psDirectX11::_checkflush(psBatchObj* obj, uint32_t num)
{
  if((obj->buffer.vert + obj->buffer.nvert + num)*obj->buffer.verts->element > obj->buffer.verts->capacity)
    return FlushPreserve();
  return obj;
}
void BSS_FASTCALL psDirectX11::DrawRectBatch(psBatchObj*& o, const psRectRotateZ& rect, const psRect* uv, uint32_t color)
{
  o = _checkflush(o, 1);
  const uint32_t numuv = (o->buffer.verts->element - sizeof(DX11_rectvert)) / sizeof(psRect);

  DX11_rectvert* buf = (DX11_rectvert*)o->buffer.get();
  *buf = { rect.left, rect.top, rect.z, rect.rotation,
    rect.right - rect.left, rect.bottom - rect.top, rect.pivot.x, rect.pivot.y,
    color };
  memcpy(buf + 1, uv, sizeof(psRect)*numuv);
  ++o->buffer.nvert;
}
psBatchObj* BSS_FASTCALL psDirectX11::DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVec* verts, uint32_t num, psVec3D offset, unsigned long vertexcolor, psFlag flags, const float(&transform)[4][4])
{
  psBatchObj* obj = DrawBatchBegin(shader, !stateblock ? 0 : stateblock->GetSB(), flags|PSFLAG_DONOTBATCH, &_batchvertbuf, &_batchindexbuf, TRIANGLELIST, transform, num);
  psBufferObj* v = obj->buffer.verts;

  if(num*v->element > v->capacity) return obj;
  DX11_simplevert* buf = (DX11_simplevert*)obj->buffer.get();
  assert(v->element == sizeof(DX11_simplevert));
  for(uint32_t i = 0; i < num; ++i)
  {
    buf[i].x = verts[i].x + offset.x;
    buf[i].y = verts[i].y + offset.y;
    buf[i].z = offset.z;
    buf[i].w = 1;
    buf[i].color = vertexcolor;
  }
  obj->buffer.nvert += num;
  obj->buffer.indice = 0;
  obj->buffer.nindice = (num - 2) * 3;
  return obj;
}
psBatchObj* BSS_FASTCALL psDirectX11::DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVertex* verts, uint32_t num, psFlag flags, const float(&transform)[4][4])
{
  static_assert(sizeof(psVertex) == sizeof(DX11_simplevert), "Error, psVertex is not equal to DX11_simplevert");
  psBatchObj* obj = DrawBatchBegin(shader, !stateblock ? 0 : stateblock->GetSB(), flags|PSFLAG_DONOTBATCH, &_batchvertbuf, &_batchindexbuf, TRIANGLELIST, transform, num);
  psBufferObj* v = obj->buffer.verts;

  if(num*v->element > v->capacity) return obj;
  assert(v->element == sizeof(DX11_simplevert));
  memcpy(obj->buffer.get(), verts, num*sizeof(psVertex));
  obj->buffer.nvert = num;
  obj->buffer.indice = 0;
  obj->buffer.nindice = (num - 2) * 3;
  return obj;
}
psBatchObj* BSS_FASTCALL psDirectX11::DrawPoints(psShader* shader, const psStateblock* stateblock, psVertex* particles, uint32_t num, psFlag flags, const float(&transform)[4][4])
{
  static_assert(sizeof(psVertex) == sizeof(DX11_simplevert), "Error, psVertex is not equal to DX11_simplevert");
  assert(sizeof(DX11_simplevert) == _batchvertbuf.element);
  return DrawArray(shader, stateblock, particles, num, &_batchvertbuf, 0, POINTLIST, flags, transform);
}
psBatchObj* BSS_FASTCALL psDirectX11::DrawLinesStart(psShader* shader, const psStateblock* stateblock, psFlag flags, const float(&xform)[4][4])
{
  return DrawBatchBegin(shader, !stateblock ? 0 : stateblock->GetSB(), flags, &_batchvertbuf, 0, LINELIST, xform);
}
void BSS_FASTCALL psDirectX11::DrawLines(psBatchObj*& o, const psLine& line, float Z1, float Z2, unsigned long vertexcolor)
{
  o = _checkflush(o, 2);

  DX11_simplevert* linebuf = (DX11_simplevert*)o->buffer.get();
  linebuf[0] = { line.x1, line.y1, Z1, 1, vertexcolor };
  linebuf[1] = { line.x2, line.y2, Z2, 1, vertexcolor };
  o->buffer.nvert += 2;
}
void BSS_FASTCALL psDirectX11::PushCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport, const psVec& extent)
{
  PROFILE_FUNC();

  // Build a custom projection matrix that discards the znear scaling. The formula is:
  // 2/w  0     0              0
  // 0    -2/h  0              0
  // 0    0     zf/(zf-zn)     1
  // 0    0     zn*zf/(zn-zf)  0
  // We negate the y axis to get our right handed coordinate system, where +y points down, +x points
  // to the right, and +z points into the screen.
  BSS_ALIGN(16) Matrix<float, 4, 4> matProj;
  memset((float*)matProj.v, 0, sizeof(Matrix<float, 4, 4>));
  float znear = extent.x;
  float zfar = extent.y;
  matProj.v[0][0] = 2.0f / viewport.right;
  matProj.v[1][1] = -2.0f / viewport.bottom;
  matProj.v[2][2] = zfar / (zfar - znear);
  matProj.v[3][2] = (znear*zfar) / (znear - zfar);
  matProj.v[2][3] = 1;

  // Cameras have an origin and rotational pivot in their center, so we have to adjust our effective pivot
  psVec adjust = pivot - (psVec(viewport.right, viewport.bottom) * 0.5f);

  if(_dpiscale != VEC_ONE) // Do we need to do DPI scaling?
  {
    Matrix<float, 4, 4> m;
    Matrix<float, 4, 4>::AffineScaling(_dpiscale.x, _dpiscale.y, 1, m.v);
    matProj *= m;
  }

  BSS_ALIGN(16) Matrix<float, 4, 4> cam;
  Matrix<float, 4, 4>::AffineTransform_T(pos.x - adjust.x, pos.y - adjust.y, pos.z, rotation, adjust.x, adjust.y, cam);
  BSS_ALIGN(16) Matrix<float, 4, 4> matView;
  cam.Inverse(matView.v); // inverse cam and store the result in matView

  BSS_ALIGN(16) Matrix<float, 4, 4> defaultCam;
  Matrix<float, 4, 4>::Translation_T(viewport.right*-0.5f, viewport.bottom*-0.5f, 1, defaultCam);

  CamDef camdef = { matView * matProj, defaultCam*matProj, matView, viewport };
  _camstack.Push(camdef);
  _applycamera();
}
void BSS_FASTCALL psDirectX11::PushCamera3D(const float(&m)[4][4], const psRectiu& viewport)
{
  PROFILE_FUNC();
  CamDef camdef = { Matrix<float, 4, 4>(m), Matrix<float, 4, 4>(m), Matrix<float, 4, 4>(identity), viewport };
  _camstack.Push(camdef);
  _applycamera();
}
void psDirectX11::_applycamera()
{
  const psRectiu& viewport = _camstack.Peek().viewport; // We can't make this viewport dynamic because half the problem is the projection has the width/height embedded, and that has to be adjusted to the rendertarget as well.
  D3D11_VIEWPORT vp = { viewport.left, viewport.top, viewport.right, viewport.bottom, 0.0f, 1.0f };
  _context->RSSetViewports(1, &vp);

  _clipstack[0].left = viewport.left;
  _clipstack[0].top = viewport.top;
  _clipstack[0].right = viewport.right;
  _clipstack[0].bottom = viewport.bottom;

  if(_clipstack.Length() <= 1) // if we are currently using the default clip rect, re-apply with new dimensions
    _context->RSSetScissorRects(1, (D3D11_RECT*)&_clipstack[0]);

  _setcambuf(_cam_def, _camstack.Peek().viewproj.v, identity); // matViewProj is identical to matProj here so PSFLAG_FIXED will have no effect
  _setcambuf(_cam_usr, _camstack.Peek().viewproj.v, identity);
  _setcambuf(_proj_def, _camstack.Peek().proj.v, identity);
  _setcambuf(_proj_usr, _camstack.Peek().proj.v, identity);
}
void BSS_FASTCALL psDirectX11::PopCamera()
{
  if(_camstack.Length() > 1)
  {
    _camstack.Discard();
    _applycamera();
  }
}
// Applies the camera transform (or it's inverse) according to the flags to a point.
psVec3D BSS_FASTCALL psDirectX11::TransformPoint(const psVec3D& point) const
{
  Vector<float, 4> v = { point.x, point.y, point.z, 1 };
  Vector<float, 4> out = v * _camstack.Peek().view;
  return out.xyz;
}
psVec3D BSS_FASTCALL psDirectX11::ReversePoint(const psVec3D& point) const
{
  Vector<float, 4> v = { point.x, point.y, point.z, 1 };
  Vector<float, 4> out = v * _camstack.Peek().view.Inverse();
  return out.xyz;
}
void psDirectX11::DrawFullScreenQuad()
{
  PROFILE_FUNC();
  if(_lastVS != _fsquadVS) _context->VSSetShader(_fsquadVS, 0, 0);
  _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _context->Draw(3, 0);
  if(_lastVS != _fsquadVS) _context->VSSetShader(_lastVS, 0, 0); // restore last shader
}

void* BSS_FASTCALL psDirectX11::CreateBuffer(uint32_t capacity, uint32_t element, uint32_t usage, const void* initdata)
{
  PROFILE_FUNC();
  uint32_t bytes = T_NEXTMULTIPLE(capacity*element, 15);
  D3D11_BUFFER_DESC desc = {
    bytes,
    (D3D11_USAGE)_usagetodxtype(usage),
    _usagetobind(usage),
    _usagetocpuflag(usage),
    _usagetomisc(usage, _samples.Count > 1 && (usage & USAGE_STAGING) != USAGE_STAGING && (usage & USAGE_MULTISAMPLE)), // It's crucial to use != for the USAGE_STAGING check, becuase it is NOT a flag.
    element,
  };

  D3D11_SUBRESOURCE_DATA subdata = { initdata, 0, 0 };

  ID3D11Buffer* buf;
  LOGFAILURERETNULL(_device->CreateBuffer(&desc, !initdata ? 0 : &subdata, &buf), "CreateBuffer failed");
  return buf;
}
void* BSS_FASTCALL psDirectX11::LockBuffer(void* target, uint32_t flags)
{
  PROFILE_FUNC();
  D3D11_MAPPED_SUBRESOURCE r;
  LOGFAILURERETNULL(_context->Map((ID3D11Buffer*)target, 0, (D3D11_MAP)(flags&LOCK_TYPEMASK), (flags&LOCK_DONOTWAIT) ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &r), "LockBuffer failed with error ", _geterror(_lasterr))
  return r.pData;
}
void BSS_FASTCALL psDirectX11::UnlockBuffer(void* target)
{
  PROFILE_FUNC();
  _context->Unmap((ID3D11Buffer*)target, 0);
  PROCESSQUEUE();
}

ID3D11Resource* psDirectX11::_textotex2D(void* t)
{
  ID3D11Resource* r;
  ((ID3D11ShaderResourceView*)t)->GetResource(&r);
  return r;
}

void* BSS_FASTCALL psDirectX11::LockTexture(void* target, uint32_t flags, uint32_t& pitch, uint8_t miplevel)
{
  PROFILE_FUNC();
  D3D11_MAPPED_SUBRESOURCE tex;

  _context->Map(_textotex2D(target), D3D11CalcSubresource(miplevel, 0, 1), (D3D11_MAP)(flags&LOCK_TYPEMASK), (flags&LOCK_DONOTWAIT) ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &tex);
  pitch = tex.RowPitch;
  PROCESSQUEUE();
  return tex.pData;
}
void BSS_FASTCALL psDirectX11::UnlockTexture(void* target, uint8_t miplevel)
{
  PROFILE_FUNC();
  _context->Unmap(_textotex2D(target), D3D11CalcSubresource(miplevel, 0, 1));
  PROCESSQUEUE();
}

inline const char* BSS_FASTCALL psDirectX11::_geterror(HRESULT err)
{
  switch(err)
  {
  case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
  case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
  case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL";
  case D3DERR_WASSTILLDRAWING: return "D3DERR_WASSTILLDRAWING";
  }
  return GetDXGIError(err);
}

// DX11 ignores the texblock parameter because it doesn't bind those states to the texture at creation time.
void* BSS_FASTCALL psDirectX11::CreateTexture(psVeciu dim, FORMATS format, uint32_t usage, uint8_t miplevels, const void* initdata, void** additionalview, psTexblock* texblock)
{
  PROFILE_FUNC();
  ID3D11Texture2D* tex = 0;

  bool multisample = (_samples.Count > 1) && (usage & USAGE_STAGING) != USAGE_STAGING && (usage & USAGE_MULTISAMPLE);
  if(multisample) { miplevels = 1; initdata = 0; }
  D3D11_TEXTURE2D_DESC desc = { dim.x, dim.y, miplevels, 1, FMTtoDXGI(format), multisample ? _samples : DEFAULT_SAMPLE_DESC, (D3D11_USAGE)_usagetodxtype(usage), _usagetobind(usage), _usagetocpuflag(usage), _usagetomisc(usage, multisample) };
  D3D11_SUBRESOURCE_DATA subdata = { initdata, (((psColor::BitsPerPixel(format)*dim.x) + 7) >> 3), 0 }; // The +7 here is to force the per-line bits to correctly round up the number of bytes.

  LOGFAILURERETNULL(_device->CreateTexture2D(&desc, !initdata ? 0 : &subdata, &tex), "CreateTexture failed, error: ", _geterror(_lasterr))
    if(usage&USAGE_RENDERTARGET)
      *additionalview = _creatertview(tex);
    else if(usage&USAGE_DEPTH_STENCIL)
      *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}

void BSS_FASTCALL psDirectX11::_loadtexture(D3DX11_IMAGE_LOAD_INFO* info, uint32_t usage, FORMATS format, uint8_t miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, bool sRGB)
{
  memset(info, D3DX11_DEFAULT, sizeof(D3DX11_IMAGE_LOAD_INFO));
  info->MipLevels = miplevels;
  info->FirstMipLevel = 0;
  info->Filter = _filtertodx11(loadfilter);
  if(sRGB) info->Filter |= D3DX11_FILTER_SRGB_IN;
  info->MipFilter = _filtertodx11(FILTERS(mipfilter&FILTER_MASK));
  if(sRGB || (mipfilter&FILTER_SRGB_MIPMAP)) info->MipFilter |= D3DX11_FILTER_SRGB;
  if(dim.x != 0) info->Width = dim.x;
  if(dim.y != 0) info->Height = dim.y;
  info->Usage = (D3D11_USAGE)_usagetodxtype(usage);
  info->BindFlags = _usagetobind(usage);
  info->CpuAccessFlags = _usagetocpuflag(usage);
  info->MiscFlags = _usagetomisc(usage, false);
  info->pSrcInfo = 0;
  if(format != FMT_UNKNOWN) info->Format = FMTtoDXGI(format);
  else if(sRGB) info->Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}

void BSS_FASTCALL psDirectX11::_applyloadshader(ID3D11Texture2D* tex, psShader* shader, bool sRGB)
{
  D3D11_TEXTURE2D_DESC desc;
  tex->GetDesc(&desc);
  ID3D11Texture2D* loadtex;
  if(sRGB) desc.Format = FMTtoDXGI(psDriver::FromSRGBFormat(DXGItoFMT(desc.Format)));
  desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  ID3D11Texture2D* casttex;
  _device->CreateTexture2D(&desc, 0, &casttex);
  _context->CopyResource(casttex, tex);

  desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
  desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
  _device->CreateTexture2D(&desc, 0, &loadtex);

  ID3D11ShaderResourceView* view = static_cast<ID3D11ShaderResourceView*>(_createshaderview(casttex));
  _context->PSSetShaderResources(0, 1, &view);
  ID3D11SamplerState* samp = (ID3D11SamplerState*)STATEBLOCK_LIBRARY::POINTSAMPLE->GetSB();
  _context->PSSetSamplers(0, 1, &samp);
  ID3D11RenderTargetView* rtview = static_cast<ID3D11RenderTargetView*>(_creatertview(loadtex));
  _context->OMSetRenderTargets(1, &rtview, 0);
  SetStateblock(STATEBLOCK_LIBRARY::REPLACE->GetSB());
  shader->Activate();
  PushCamera(psVec3D(0, 0, -1.0f), VEC_ZERO, 0, psRectiu(0, 0, desc.Width, desc.Height), psCamera::default_extent);
  DrawFullScreenQuad();
  PopCamera();
  _applyrendertargets(); //re-apply whatever render target we had before
  ID3D11ShaderResourceView* loadview = static_cast<ID3D11ShaderResourceView*>(_createshaderview(loadtex));
  _context->GenerateMips(loadview);
  _context->CopyResource(tex, loadtex);

  loadview->Release();
  rtview->Release();
  view->Release();
  loadtex->Release();
  casttex->Release();
}
void BSS_FASTCALL psDirectX11::_applymipshader(ID3D11Texture2D* tex, psShader* shader)
{
  D3D11_TEXTURE2D_DESC desc2d;
  tex->GetDesc(&desc2d);
  desc2d.BindFlags |= D3D11_BIND_RENDER_TARGET;

  ID3D11Texture2D* buftex;
  _device->CreateTexture2D(&desc2d, 0, &buftex);

  D3D11_SHADER_RESOURCE_VIEW_DESC desc;
  desc.Format = desc2d.Format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

  D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
  rtDesc.Format = desc2d.Format;
  rtDesc.ViewDimension = (desc2d.SampleDesc.Count > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;

  SetStateblock(STATEBLOCK_LIBRARY::REPLACE->GetSB());
  shader->Activate();

  ID3D11SamplerState* samp = (ID3D11SamplerState*)STATEBLOCK_LIBRARY::POINTSAMPLE->Combine(STATEBLOCK_LIBRARY::UVCLAMP)->GetSB();
  // Go through each mipmap slice and render it using the slice above it, then copy it back to our source.
  for(UINT i = 1; i < desc2d.MipLevels; ++i)
  {
    // Get view for previous level
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.MostDetailedMip = i - 1;
    ID3D11ShaderResourceView* view = 0;
    _device->CreateShaderResourceView(tex, &desc, &view);

    // Get rendertarget for this level in our buffer texture
    rtDesc.Texture2D.MipSlice = i;
    ID3D11RenderTargetView* rtview = 0;
    _device->CreateRenderTargetView(buftex, &rtDesc, &rtview);

    // Set everything and render
    _context->PSSetShaderResources(0, 1, &view);
    _context->PSSetSamplers(0, 1, &samp);
    _context->OMSetRenderTargets(1, &rtview, 0);
    PushCamera(psVec3D(0, 0, -1.0f), VEC_ZERO, 0, psRectiu(0, 0, desc2d.Width/(1<<i), desc2d.Height/(1<<i)), psCamera::default_extent);
    DrawFullScreenQuad();
    PopCamera();
    _applyrendertargets();

    // Copy the subresource back to our real texture and release views
    _context->CopySubresourceRegion(tex, D3D11CalcSubresource(i, 0, desc2d.MipLevels), 0, 0, 0, buftex, D3D11CalcSubresource(i, 0, desc2d.MipLevels), 0);
    view->Release();
    rtview->Release();
  }

  buftex->Release();
}

void* BSS_FASTCALL psDirectX11::LoadTexture(const char* path, uint32_t usage, FORMATS format, void** additionalview, uint8_t miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, psTexblock* texblock, bool sRGB)
{
  PROFILE_FUNC();
  D3DX11_IMAGE_LOAD_INFO info;
  if(_customfilter(mipfilter)) usage |= USAGE_SHADER_RESOURCE;
  _loadtexture(&info, usage, format, miplevels, mipfilter, loadfilter, dim, sRGB);

  ID3D11Resource* tex = 0;
  LOGFAILURERETNULL(D3DX11CreateTextureFromFileW(_device, cStrW(path), &info, 0, &tex, 0), "LoadTexture failed with error ", _geterror(_lasterr), " for ", path);

  if(_customfilter(loadfilter))
    _applyloadshader(static_cast<ID3D11Texture2D*>(tex), _getfiltershader(loadfilter), loadfilter == FILTER_PREMULTIPLY_SRGB);

  if(_customfilter(mipfilter))
    _applymipshader(static_cast<ID3D11Texture2D*>(tex), _getfiltershader(mipfilter));

#ifdef BSS_DEBUG
  tex->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(path), path);
#endif
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}

void* BSS_FASTCALL psDirectX11::LoadTextureInMemory(const void* data, size_t datasize, uint32_t usage, FORMATS format, void** additionalview, uint8_t miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, psTexblock* texblock, bool sRGB)
{
  PROFILE_FUNC();
  D3DX11_IMAGE_LOAD_INFO info;
  if(_customfilter(mipfilter)) usage |= USAGE_SHADER_RESOURCE;
  _loadtexture(&info, usage, format, miplevels, mipfilter, loadfilter, dim, sRGB);

  ID3D11Resource* tex = 0;
  LOGFAILURERETNULL(D3DX11CreateTextureFromMemory(_device, data, datasize, &info, 0, &tex, 0), "LoadTexture failed for ", data);

  if(_customfilter(loadfilter))
    _applyloadshader(static_cast<ID3D11Texture2D*>(tex), _getfiltershader(loadfilter), loadfilter == FILTER_PREMULTIPLY_SRGB);

  if(_customfilter(mipfilter))
    _applymipshader(static_cast<ID3D11Texture2D*>(tex), _getfiltershader(mipfilter));

  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}
void BSS_FASTCALL psDirectX11::CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, uint8_t miplevel)
{
  if(!srcrect)
  {
    _context->CopySubresourceRegion(_textotex2D(dest), D3D11CalcSubresource(miplevel, 0, 1), destpos.x, destpos.y, 0, _textotex2D(src), D3D11CalcSubresource(miplevel, 0, 1), 0);
    return;
  }

  D3D11_BOX box = {
    srcrect->left,
    srcrect->top,
    0,
    srcrect->right,
    srcrect->bottom,
    1,
  };
  _context->CopySubresourceRegion(_textotex2D(dest), D3D11CalcSubresource(miplevel, 0, 1), destpos.x, destpos.y, 0, _textotex2D(src), D3D11CalcSubresource(miplevel, 0, 1), &box);
}

void BSS_FASTCALL psDirectX11::PushClipRect(const psRect& rect)
{
  PROFILE_FUNC();
  psVec scale = GetDPIScale();
  psRectl r(fFastTruncate(floor(rect.left*scale.x)), fFastTruncate(floor(rect.top*scale.y)), fFastTruncate(ceilf(rect.right*scale.x)), fFastTruncate(ceil(rect.bottom*scale.y)));
  _clipstack.Push(r);
}

psRect psDirectX11::PeekClipRect()
{
  if(!_clipstack.Length())
    return psRect(VEC_ZERO, psVec(rawscreendim)/GetDPIScale());
  return psRect(_clipstack.Peek());
}

void psDirectX11::PopClipRect()
{
  PROFILE_FUNC();
  if(_clipstack.Length()>1)
    _clipstack.Discard();
}

void BSS_FASTCALL psDirectX11::SetRenderTargets(const psTex* const* texes, uint8_t num, const psTex* depthstencil)
{
  PROFILE_FUNC();
  _lastdepth = !depthstencil ? 0 : (ID3D11DepthStencilView*)depthstencil->GetView(); 
  _lastrt.SetLength(num);
  for(uint8_t i = 0; i < num; ++i) _lastrt[i] = (ID3D11RenderTargetView*)texes[i]->GetView();
}

void BSS_FASTCALL psDirectX11::SetShaderConstants(void* constbuf, SHADER_VER shader)
{
  PROFILE_FUNC();
  if(shader <= VERTEX_SHADER_5_0)
    _context->VSSetConstantBuffers(1, 1, (ID3D11Buffer**)&constbuf);
  else if(shader <= PIXEL_SHADER_5_0)
    _context->PSSetConstantBuffers(1, 1, (ID3D11Buffer**)&constbuf);
  else if(shader <= GEOMETRY_SHADER_5_0)
    _context->GSSetConstantBuffers(1, 1, (ID3D11Buffer**)&constbuf);
}

void BSS_FASTCALL psDirectX11::SetTextures(const psTex* const* texes, uint8_t num, SHADER_VER shader)
{
  PROFILE_FUNC();
  auto& d = _lasttex[(shader >= PIXEL_SHADER_1_1) + (shader >= GEOMETRY_SHADER_4_0)];
  d.SetLength(num);
  memcpy(d.begin(), texes, sizeof(psTex*)*num);
}
// Builds a stateblock from the given set of state changes
void* BSS_FASTCALL psDirectX11::CreateStateblock(const STATEINFO* states, uint32_t count)
{
  PROFILE_FUNC();
  // Build each type of stateblock and independently check for duplicates.
  DX11_SB* sb = new DX11_SB { 0, 0, 0, 0, { 1.0f, 1.0f, 1.0f, 1.0f }, 0xffffffff };
  D3D11_BLEND_DESC bs = { 0, 0, { 1, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL } };
  D3D11_DEPTH_STENCIL_DESC ds = { 1, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, 0, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
  { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
  { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS } };
  D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, 0, 0, 0.0f, 0.0f, 1, 1, 0, 1 };
  
  const STATEINFO* cur = states;
  const STATEINFO* end = states + count;
  if(cur != end && cur->type == TYPE_BLEND_ALPHATOCOVERAGEENABLE) bs.AlphaToCoverageEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_BLEND_INDEPENDENTBLENDENABLE) bs.IndependentBlendEnable = (cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_BLENDENABLE && cur->index <= 7) bs.RenderTarget[cur->index].BlendEnable = (cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_SRCBLEND && cur->index <= 7) bs.RenderTarget[cur->index].SrcBlend = (D3D11_BLEND)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_DESTBLEND && cur->index <= 7) bs.RenderTarget[cur->index].DestBlend = (D3D11_BLEND)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_BLENDOP && cur->index <= 7) bs.RenderTarget[cur->index].BlendOp = (D3D11_BLEND_OP)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_SRCBLENDALPHA && cur->index <= 7) bs.RenderTarget[cur->index].SrcBlendAlpha = (D3D11_BLEND)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_DESTBLENDALPHA && cur->index <= 7) bs.RenderTarget[cur->index].DestBlendAlpha = (D3D11_BLEND)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_BLENDOPALPHA && cur->index <= 7) bs.RenderTarget[cur->index].BlendOpAlpha = (D3D11_BLEND_OP)(cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_RENDERTARGETWRITEMASK && cur->index <= 7) bs.RenderTarget[cur->index].RenderTargetWriteMask = (cur++)->value;
  while(cur != end && cur->type == TYPE_BLEND_BLENDFACTOR && cur->index <= 3) sb->blendfactor[cur->index] = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_BLEND_SAMPLEMASK) sb->sampleMask = (cur++)->value;

  if(cur != end && cur->type == TYPE_DEPTH_DEPTHENABLE) ds.DepthEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_DEPTHWRITEMASK) ds.DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_DEPTHFUNC) ds.DepthFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_STENCILENABLE) ds.StencilEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_STENCILREADMASK) ds.StencilReadMask = (cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_STENCILWRITEMASK) ds.StencilWriteMask = (cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_FRONT_STENCILFAILOP) ds.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_FRONT_STENCILDEPTHFAILOP) ds.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_FRONT_STENCILPASSOP) ds.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_FRONT_STENCILFUNC) ds.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_BACK_STENCILFAILOP) ds.BackFace.StencilFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_BACK_STENCILDEPTHFAILOP) ds.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_BACK_STENCILPASSOP) ds.BackFace.StencilPassOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_BACK_STENCILFUNC) ds.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != end && cur->type == TYPE_DEPTH_STENCILVALUE) sb->stencilref = (cur++)->value;

  if(cur != end && cur->type == TYPE_RASTER_FILLMODE) rs.FillMode = (D3D11_FILL_MODE)(cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_CULLMODE) rs.CullMode = (D3D11_CULL_MODE)(cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_FRONTCOUNTERCLOCKWISE) rs.FrontCounterClockwise = (cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_DEPTHBIAS) rs.DepthBias = (cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_DEPTHBIASCLAMP) rs.DepthBiasClamp = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_RASTER_SLOPESCALEDDEPTHBIAS) rs.SlopeScaledDepthBias = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_RASTER_DEPTHCLIPENABLE) rs.DepthClipEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_SCISSORENABLE) rs.ScissorEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_MULTISAMPLEENABLE) rs.MultisampleEnable = (cur++)->value;
  if(cur != end && cur->type == TYPE_RASTER_ANTIALIASEDLINEENABLE) rs.AntialiasedLineEnable = (cur++)->value;

  LOGFAILURERETNULL(_device->CreateBlendState(&bs, &sb->bs), "CreateBlendState failed")
    LOGFAILURERETNULL(_device->CreateDepthStencilState(&ds, &sb->ds), "CreateDepthStencilState failed")
    LOGFAILURERETNULL(_device->CreateRasterizerState(&rs, &sb->rs), "CreateRasterizerState failed")

    return sb;
}

void* BSS_FASTCALL psDirectX11::CreateTexblock(const STATEINFO* states, uint32_t count)
{
  D3D11_SAMPLER_DESC ss = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 16, D3D11_COMPARISON_NEVER, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, FLT_MAX };

  ID3D11SamplerState* ret;
  const STATEINFO* cur = states;
  const STATEINFO* end = states + count;
  if(cur != end && cur->type == TYPE_SAMPLER_FILTER) ss.Filter = (D3D11_FILTER)(cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_ADDRESSU) ss.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_ADDRESSV) ss.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_ADDRESSW) ss.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_MIPLODBIAS) ss.MipLODBias = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_MAXANISOTROPY) ss.MaxAnisotropy = (cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_COMPARISONFUNC) ss.ComparisonFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != end && cur->type == TYPE_SAMPLER_BORDERCOLOR1) ss.BorderColor[0] = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_BORDERCOLOR2) ss.BorderColor[1] = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_BORDERCOLOR3) ss.BorderColor[2] = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_BORDERCOLOR4) ss.BorderColor[3] = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_MINLOD) ss.MinLOD = (cur++)->valuef;
  if(cur != end && cur->type == TYPE_SAMPLER_MAXLOD) ss.MaxLOD = (cur++)->valuef;
  LOGFAILURERETNULL(_device->CreateSamplerState(&ss, &ret));
  return ret;
}

// Sets a given stateblock
void BSS_FASTCALL psDirectX11::SetStateblock(void* stateblock)
{
  PROFILE_FUNC();
  DX11_SB* sb = !stateblock ? _defaultSB : (DX11_SB*)stateblock;
  _context->RSSetState(sb->rs);
  _context->OMSetDepthStencilState(sb->ds, sb->stencilref);
  _context->OMSetBlendState(sb->bs, sb->blendfactor, sb->sampleMask);
}
void* BSS_FASTCALL psDirectX11::CreateLayout(void* shader, const ELEMENT_DESC* elements, uint8_t num)
{
  PROFILE_FUNC();
  ID3D10Blob* blob = (ID3D10Blob*)shader;
  return CreateLayout(blob->GetBufferPointer(), blob->GetBufferSize(), elements, num);
}
void* BSS_FASTCALL psDirectX11::CreateLayout(void* shader, size_t sz, const ELEMENT_DESC* elements, uint8_t num)
{
  PROFILE_FUNC();
  DYNARRAY(D3D11_INPUT_ELEMENT_DESC, descs, num);

  for(uint8_t i = 0; i < num; ++i)
  {
    switch(elements[i].semantic)
    {
    case ELEMENT_BINORMAL: descs[i].SemanticName = "BINORMAL"; break;
    case ELEMENT_BLENDINDICES: descs[i].SemanticName = "BLENDINDICES"; break;
    case ELEMENT_BLENDWEIGHT: descs[i].SemanticName = "BLENDWEIGHT"; break;
    case ELEMENT_COLOR: descs[i].SemanticName = "COLOR"; break;
    case ELEMENT_NORMAL: descs[i].SemanticName = "NORMAL"; break;
    case ELEMENT_POSITION: descs[i].SemanticName = "POSITION"; break;
    case ELEMENT_POSITIONT: descs[i].SemanticName = "POSITIONT"; break;
    case ELEMENT_PSIZE: descs[i].SemanticName = "PSIZE"; break;
    case ELEMENT_TANGENT: descs[i].SemanticName = "TANGENT"; break;
    case ELEMENT_TEXCOORD: descs[i].SemanticName = "TEXCOORD"; break;
    }
    descs[i].SemanticIndex = elements[i].semanticIndex;
    descs[i].Format = FMTtoDXGI(elements[i].format);
    descs[i].AlignedByteOffset = elements[i].byteOffset;
    descs[i].InputSlot = elements[i].IAslot;
    descs[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    descs[i].InstanceDataStepRate = 0;
  }

  ID3D11InputLayout* r;
  LOGFAILURERETNULL(_device->CreateInputLayout(descs, num, shader, sz, &r), "CreateInputLayout failed, error: ", _geterror(_lasterr))
    return r;
}
void BSS_FASTCALL psDirectX11::SetLayout(void* layout)
{
  PROFILE_FUNC();
  _context->IASetInputLayout((ID3D11InputLayout*)layout);
}

TEXTURE_DESC BSS_FASTCALL psDirectX11::GetTextureDesc(void* t)
{
  TEXTURE_DESC ret = {};
  ID3D11Resource* res = 0;
  ((ID3D11ShaderResourceView*)t)->GetResource(&res);
  D3D11_RESOURCE_DIMENSION type;
  res->GetType(&type);

  switch(type)
  {
  case D3D11_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
  {
    D3D11_TEXTURE1D_DESC desc;
    ((ID3D11Texture1D*)res)->GetDesc(&desc);

    ret.dim.x = desc.Width;
    ret.dim.y = 1;
    ret.dim.z = 1;
    ret.format = DXGItoFMT(desc.Format);
    ret.miplevels = desc.MipLevels;
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags, false);
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc;
    ((ID3D11Texture2D*)res)->GetDesc(&desc);

    ret.dim.x = desc.Width;
    ret.dim.y = desc.Height;
    ret.dim.z = 1;
    ret.format = DXGItoFMT(desc.Format);
    ret.miplevels = desc.MipLevels;
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags, desc.SampleDesc.Count > 1);
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  {
    D3D11_TEXTURE3D_DESC desc;
    ((ID3D11Texture3D*)res)->GetDesc(&desc);

    ret.dim.x = desc.Width;
    ret.dim.y = desc.Height;
    ret.dim.z = desc.Depth;
    ret.format = DXGItoFMT(desc.Format);
    ret.miplevels = desc.MipLevels;
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags, false);
  }
  break;
  }

  return ret;
}

void BSS_FASTCALL psDirectX11::FreeResource(void* p, RESOURCE_TYPE t)
{
  PROFILE_FUNC();
  switch(t)
  {
  case RES_SHADERPS:
    ((ID3D11PixelShader*)p)->Release();
    break;
  case RES_SHADERVS:
    ((ID3D11VertexShader*)p)->Release();
    break;
  case RES_SHADERGS:
    ((ID3D11GeometryShader*)p)->Release();
    break;
  case RES_TEXTURE:
    ((ID3D11ShaderResourceView*)p)->Release();
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    ((ID3D11Buffer*)p)->Release();
    break;
  case RES_SURFACE:
    ((ID3D11RenderTargetView*)p)->Release();
    break;
  case RES_DEPTHVIEW:
    ((ID3D11DepthStencilView*)p)->Release();
    break;
  case RES_STATEBLOCK:
  {
    DX11_SB* sb = (DX11_SB*)p;
    sb->rs->Release();
    sb->ds->Release();
    sb->bs->Release();
    delete sb;
  }
  break;
  case RES_TEXBLOCK:
    ((ID3D11SamplerState*)p)->Release();
    break;
  case RES_LAYOUT:
    ((ID3D11InputLayout*)p)->Release();
    break;
  }
}
void BSS_FASTCALL psDirectX11::GrabResource(void* p, RESOURCE_TYPE t)
{
  PROFILE_FUNC();
  switch(t)
  {
  case RES_SHADERPS:
    ((ID3D11PixelShader*)p)->AddRef();
    break;
  case RES_SHADERVS:
    ((ID3D11VertexShader*)p)->AddRef();
    break;
  case RES_SHADERGS:
    ((ID3D11GeometryShader*)p)->AddRef();
    break;
  case RES_TEXTURE:
    ((ID3D11ShaderResourceView*)p)->AddRef();
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    ((ID3D11Buffer*)p)->AddRef();
    break;
  case RES_SURFACE:
    ((ID3D11RenderTargetView*)p)->AddRef();
    break;
  case RES_DEPTHVIEW:
    ((ID3D11DepthStencilView*)p)->AddRef();
    break;
  case RES_LAYOUT:
    ((ID3D11InputLayout*)p)->AddRef();
    break;
  }
}
void BSS_FASTCALL psDirectX11::CopyResource(void* dest, void* src, RESOURCE_TYPE t)
{
  PROFILE_FUNC();
  ID3D11Resource* rsrc = 0;
  ID3D11Resource* rdest = 0;
  switch(t)
  {
  case RES_TEXTURE:
    ((ID3D11ShaderResourceView*)src)->GetResource(&rsrc);
    ((ID3D11ShaderResourceView*)dest)->GetResource(&rdest);
    break;
  case RES_SURFACE:
    ((ID3D11RenderTargetView*)src)->GetResource(&rsrc);
    ((ID3D11RenderTargetView*)dest)->GetResource(&rdest);
    break;
  case RES_DEPTHVIEW:
    ((ID3D11DepthStencilView*)src)->GetResource(&rsrc);
    ((ID3D11DepthStencilView*)dest)->GetResource(&rdest);
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    rsrc = ((ID3D11Buffer*)src);
    rdest = ((ID3D11Buffer*)dest);
    break;
  }
  assert(rsrc != 0 && rdest != 0);
  _context->CopyResource(rdest, rsrc);
}

void BSS_FASTCALL psDirectX11::Resize(psVeciu dim, FORMATS format, char fullscreen)
{
  PROFILE_FUNC();
  if(rawscreendim != dim || _backbuffer->GetFormat() != format)
  {
    if(_backbuffer)
      _backbuffer->~psTex();
    _context->OMSetRenderTargets(0, 0, 0);
    _context->ClearState();
    LOGFAILURE(_swapchain->ResizeBuffers(1, dim.x, dim.y, FMTtoDXGI(format), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH), "ResizeBuffers failed with error code: ", _lasterr);
    _getbackbufferref();
    _resetscreendim();
  }

  if(fullscreen >= 0)
    LOGFAILURE(_swapchain->SetFullscreenState(fullscreen > 0, 0));
}
void psDirectX11::_getbackbufferref()
{
  ID3D11Texture2D* backbuffer = 0;
  LOGFAILURE(_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer));

  D3D11_TEXTURE2D_DESC backbuffer_desc;
  backbuffer->GetDesc(&backbuffer_desc);
  if(!_backbuffer)
    _backbuffer = new psTex(0,
      _creatertview(backbuffer),
      psVeciu(backbuffer_desc.Width, backbuffer_desc.Height),
      DXGItoFMT(backbuffer_desc.Format),
      (USAGETYPES)_reverseusage(backbuffer_desc.Usage, backbuffer_desc.MiscFlags, backbuffer_desc.BindFlags, backbuffer_desc.SampleDesc.Count == 1),
      backbuffer_desc.MipLevels);
  else
    new(_backbuffer) psTex(0,
      _creatertview(backbuffer),
      psVeciu(backbuffer_desc.Width, backbuffer_desc.Height),
      DXGItoFMT(backbuffer_desc.Format),
      (USAGETYPES)_reverseusage(backbuffer_desc.Usage, backbuffer_desc.MiscFlags, backbuffer_desc.BindFlags,backbuffer_desc.SampleDesc.Count == 1),
      backbuffer_desc.MipLevels);

  backbuffer->Release();
}
void psDirectX11::_resetscreendim()
{
  _applyrendertargets();
  rawscreendim = _backbuffer->GetDim();
  screendim = psVec(rawscreendim) * _dpiscale;
  _camstack.Clear();
  PushCamera(psVec3D(0, 0, -1.0f), VEC_ZERO, 0, psRectiu(0, 0, rawscreendim.x, rawscreendim.y), psCamera::default_extent);
}
void psDirectX11::_applyrendertargets()
{
  if(!_lastrt.Length()) {
    ID3D11RenderTargetView* v = (ID3D11RenderTargetView*)_backbuffer->GetView();
    _context->OMSetRenderTargets(1, &v, _lastdepth);
  }
  else
    _context->OMSetRenderTargets(_lastrt.Length(), _lastrt.begin(), _lastdepth);
}
void BSS_FASTCALL psDirectX11::Clear(uint32_t color)
{
  PROFILE_FUNC();
  psColor colors = psColor(color).rgba();
  ID3D11RenderTargetView* views[8];
  ID3D11DepthStencilView* dview;
  _context->OMGetRenderTargets(8, views, &dview);
  if(dview)
  {
    _context->ClearDepthStencilView(dview, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    dview->Release(); // GetRenderTargets calls Get on everything
  }
  for(uint8_t i = 0; i < 8; ++i)
    if(views[i])
    {
      _context->ClearRenderTargetView(views[i], colors);
      views[i]->Release(); // GetRenderTargets calls Get on everything
    }
}

void* BSS_FASTCALL psDirectX11::CompileShader(const char* source, SHADER_VER profile, const char* entrypoint)
{
  PROFILE_FUNC();
  ID3D10Blob* ret = 0;
  ID3D10Blob* err = 0;
  if(FAILED(_lasterr = D3DX11CompileFromMemory(source, strlen(source), 0, 0, 0, entrypoint, shader_profiles[profile], 0, 0, 0, &ret, &err, 0)))
  {
    if(!err) PSLOG(2) << "The effect could not be compiled! ERR: " << _geterror(_lasterr) << " Source: \n" << source << std::endl;
    else if(_lasterr == 0x8007007e) PSLOG(2) << "The effect cannot be loaded because a module cannot be found (?) Source: \n" << source << std::endl;
    else PSLOG(2) << "The effect failed to compile (Errors: " << (const char*)err->GetBufferPointer() << ") Source: \n" << source << std::endl;
    return 0;
  }
  return ret;
}
void* BSS_FASTCALL psDirectX11::CreateShader(const void* data, SHADER_VER profile)
{
  PROFILE_FUNC();
  ID3D10Blob* blob = (ID3D10Blob*)data;
  return CreateShader(blob->GetBufferPointer(), blob->GetBufferSize(), profile);
}
void* BSS_FASTCALL psDirectX11::CreateShader(const void* data, size_t datasize, SHADER_VER profile)
{
  PROFILE_FUNC();
  void* p;

  if(profile <= VERTEX_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreateVertexShader(data, datasize, NULL, (ID3D11VertexShader**)&p), "CreateVertexShader failed")
  else if(profile <= PIXEL_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreatePixelShader(data, datasize, NULL, (ID3D11PixelShader**)&p), "CreatePixelShader failed")
  else if(profile <= GEOMETRY_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreateGeometryShader(data, datasize, NULL, (ID3D11GeometryShader**)&p), "CreateGeometryShader failed")
    //else if(profile<=COMPUTE_SHADER_5_0)
    //  _device->Create((DWORD*)blob->GetBufferPointer(), blob->GetBufferSize(), (ID3D11PixelShader**)&p);
  else
    return 0;
  return p;
}
char BSS_FASTCALL psDirectX11::SetShader(void* shader, SHADER_VER profile)
{
  PROFILE_FUNC();
  if(profile <= VERTEX_SHADER_5_0 && _lastVS != shader) {
#ifdef BSS_DEBUG
    if(!shader) shader = _fsquadVS;
#endif
    _context->VSSetShader(_lastVS = (ID3D11VertexShader*)shader, 0, 0);
  }
  else if(profile <= PIXEL_SHADER_5_0 && _lastPS != shader) {
#ifdef BSS_DEBUG
    if(!shader) shader = library.DEBUG->GetInternalPrograms()[1];
#endif
    _context->PSSetShader(_lastPS = (ID3D11PixelShader*)shader, 0, 0);
  }
  else if(profile <= GEOMETRY_SHADER_5_0 && _lastGS != shader)
    _context->GSSetShader(_lastGS = (ID3D11GeometryShader*)shader, 0, 0);
  else
    return -1;
  return 0;
}
bool BSS_FASTCALL psDirectX11::ShaderSupported(SHADER_VER profile) //With DX11 shader support is known at compile time.
{
  switch(profile)
  {
  case VERTEX_SHADER_4_1:
  case VERTEX_SHADER_5_0:
  case PIXEL_SHADER_4_1:
  case PIXEL_SHADER_5_0:
  case GEOMETRY_SHADER_4_1:
  case GEOMETRY_SHADER_5_0:
  case COMPUTE_SHADER_4_1:
  case COMPUTE_SHADER_5_0:
  case DOMAIN_SHADER_5_0:
  case HULL_SHADER_5_0:
    return false;
  }
  return true;
}
void psDirectX11::SetDPIScale(psVec dpiscale)
{
  _dpiscale = dpiscale;
}
psVec psDirectX11::GetDPIScale()
{
  return _dpiscale;
}

bool psDirectX11::_checksnapshot(Snapshot& s)
{
  psRectl& lastrect = _clipstack.Peek();
  if(_lasttex[0].Length() == s.ntex[0] &&
    _lasttex[1].Length() == s.ntex[1] &&
    _lasttex[2].Length() == s.ntex[2] &&
    _lastrt.Length() == s.nrt &&
    _lastdepth == s.depth &&
    lastrect.left == s.cliprect.left &&
    lastrect.top == s.cliprect.top &&
    lastrect.right == s.cliprect.right &&
    lastrect.bottom == s.cliprect.bottom)
  {
    for(uint32_t j = 0; j < 3; ++j)
      for(uint32_t i = 0; i < s.ntex[j]; ++i)
        if(_lasttex[j][i] != _texstack[s.tex[j] + i])
          return false;
    for(uint32_t i = 0; i < s.nrt; ++i)
      if(_lastrt[i] != _texstack[s.rt + i])
        return false;
    return true;
  }
  return false;
}
uint32_t BSS_FASTCALL psDirectX11::GetSnapshot()
{
  if(_snapshotstack.Length() > 0 && _checksnapshot(_snapshotstack.Back())) // Check if we can return the last snapshot
    return _snapshotstack.Length() - 1;

  // Snapshot textures (one for each shader type), rendertargets, depth target, and clipping rect
  _snapshotstack.AddConstruct();
  Snapshot& s = _snapshotstack.Back();
  s.cliprect = _clipstack.Peek();
  s.depth = _lastdepth;
  s.nrt = _lastrt.Length();
  s.rt = _texstack.Length();
  for(uint32_t i = 0; i < s.nrt; ++i) _texstack.Add(_lastrt[i]);
  for(uint32_t j = 0; j < 3; ++j)
  {
    s.ntex[j] = _lasttex[j].Length();
    s.tex[j] = _texstack.Length();
    for(uint32_t i = 0; i < s.ntex[j]; ++i) _texstack.Add(_lasttex[j][i]);
  }

  return _snapshotstack.Length() - 1;
}

void psDirectX11::_applysnapshot(const Snapshot& s)
{
  for(int k = 0; k < 3; ++k)
  {
    int num = s.ntex[k];
    DYNARRAY(ID3D11ShaderResourceView*, views, num);
    DYNARRAY(ID3D11SamplerState*, states, num);
    for(int i = 0; i < num; ++i)
    {
      psTex* t = (psTex*)(_texstack[s.tex[i] + i]);
      views[i] = (ID3D11ShaderResourceView*)t->GetRes();
      const psTexblock* texblock = t->GetTexblock();
      states[i] = !texblock ? _defaultSS : (ID3D11SamplerState*)texblock->GetSB();
    }
    switch(k)
    {
    case 0:
      _context->VSSetShaderResources(0, num, views);
      _context->VSSetSamplers(0, num, states);
      break;
    case 1:
      _context->PSSetShaderResources(0, num, views);
      _context->PSSetSamplers(0, num, states);
      break;
    case 2:
      _context->GSSetShaderResources(0, num, views);
      _context->GSSetSamplers(0, num, states);
      break;
    }
  }
  
  if(!s.nrt) {
    ID3D11RenderTargetView* v = (ID3D11RenderTargetView*)_backbuffer->GetView();
    _context->OMSetRenderTargets(1, &v, s.depth);
  } else
    _context->OMSetRenderTargets(s.nrt, (ID3D11RenderTargetView**)&_texstack[s.rt], s.depth);

  _context->RSSetScissorRects(1, (D3D11_RECT*)&s.cliprect);
}

void psDirectX11::_processdebugqueue()
{
  if(!_infoqueue) return;
  UINT64 max = _infoqueue->GetNumStoredMessagesAllowedByRetrievalFilter();
  HRESULT hr;

  for(UINT64 i = 0; i <= max; ++i) {
    SIZE_T len = 0;
    if(FAILED(hr = _infoqueue->GetMessageA(i, NULL, &len)))
      continue;

    _processdebugmessage(i, len); // We do this in a seperate function call because we can't dynamically allocate on the stack inside of a for loop.
  }

  _infoqueue->ClearStoredMessages();
}
void BSS_FASTCALL psDirectX11::_processdebugmessage(UINT64 index, SIZE_T len)
{
  HRESULT hr;
  DYNARRAY(char, buf, len);
  D3D11_MESSAGE* message = (D3D11_MESSAGE*)buf;
  if(FAILED(hr = _infoqueue->GetMessageA(index, message, &len)))
    return;

  int level = 5;
  switch(message->Severity)
  {
  case D3D11_MESSAGE_SEVERITY_INFO: level = 4; break; // INFO
  case D3D11_MESSAGE_SEVERITY_WARNING: level = 2; break; // WARNING
  case D3D11_MESSAGE_SEVERITY_ERROR: level = 1; break; // ERROR
  case D3D11_MESSAGE_SEVERITY_CORRUPTION: level = 0; break; // FATAL ERROR
  }

  const char* category = "";
  switch(message->Category)
  {
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_APPLICATION_DEFINED: category = ":Application"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_CLEANUP: category = ":Cleanup"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_COMPILATION: category = ":Compilation"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_EXECUTION: category = ":Execution"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_INITIALIZATION: category = ":Initialization"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_MISCELLANEOUS: category = ":Misc"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: category = ":Resource"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_STATE_CREATION: category = ":State Creation"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_STATE_GETTING: category = ":State Getting"; break;
  case D3D11_MESSAGE_CATEGORY::D3D11_MESSAGE_CATEGORY_STATE_SETTING: category = ":State Setting"; break;
  }

  PSLOG(level) << "(DirectX11" << category << ") " << cStr(message->pDescription, message->DescriptionByteLength) << std::endl;
}

uint32_t BSS_FASTCALL psDirectX11::_usagetodxtype(uint32_t types)
{
  switch(types&USAGE_USAGEMASK)
  {
  case USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
  case USAGE_IMMUTABLE: return D3D11_USAGE_IMMUTABLE;
  case USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
  case USAGE_STAGING: return D3D11_USAGE_STAGING;
  }
  return -1;
}
uint32_t BSS_FASTCALL psDirectX11::_usagetocpuflag(uint32_t types)
{
  switch(types&USAGE_USAGEMASK)
  {
  case USAGE_DEFAULT: return 0;
  case USAGE_IMMUTABLE: return 0;
  case USAGE_DYNAMIC: return D3D11_CPU_ACCESS_WRITE;
  case USAGE_STAGING: return D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
  }
  return 0;
}
uint32_t BSS_FASTCALL psDirectX11::_usagetomisc(uint32_t types, bool multisampled)
{
  uint32_t r = 0;
  if((types&USAGE_RENDERTARGET) && (types&USAGE_SHADER_RESOURCE) && !multisampled) r |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
  if(types&USAGE_TEXTURECUBE) r |= D3D11_RESOURCE_MISC_TEXTURECUBE;
  return r;
}
uint32_t BSS_FASTCALL psDirectX11::_usagetobind(uint32_t types)
{
  uint32_t r = 0;
  switch(types&USAGE_BINDMASK)
  {
  case USAGE_VERTEX: r = D3D11_BIND_VERTEX_BUFFER; break;
  case USAGE_INDEX: r = D3D11_BIND_INDEX_BUFFER; break;
  case USAGE_CONSTANT_BUFFER: r = D3D11_BIND_CONSTANT_BUFFER; break;
  }
  if(types&USAGE_RENDERTARGET) r |= D3D11_BIND_RENDER_TARGET;
  if(types&USAGE_SHADER_RESOURCE) r |= D3D11_BIND_SHADER_RESOURCE;
  if(types&USAGE_DEPTH_STENCIL) r |= D3D11_BIND_DEPTH_STENCIL;
  return r;
}
uint32_t BSS_FASTCALL psDirectX11::_reverseusage(uint32_t usage, uint32_t misc, uint32_t bind, bool multisample)
{
  uint32_t r = 0;
  switch(usage)
  {
  case D3D11_USAGE_DEFAULT: r = USAGE_DEFAULT; break;
  case D3D11_USAGE_IMMUTABLE: r = USAGE_IMMUTABLE; break;
  case D3D11_USAGE_DYNAMIC: r = USAGE_DYNAMIC; break;
  case D3D11_USAGE_STAGING: r = USAGE_STAGING; break;
  }
  if(misc&D3D11_RESOURCE_MISC_TEXTURECUBE) r |= USAGE_TEXTURECUBE;
  switch(bind)
  {
  case D3D11_BIND_VERTEX_BUFFER: r |= USAGE_VERTEX; break;
  case D3D11_BIND_INDEX_BUFFER: r |= USAGE_INDEX; break;
  case D3D11_BIND_CONSTANT_BUFFER: r |= USAGE_CONSTANT_BUFFER; break;
  }
  if(bind&D3D11_BIND_RENDER_TARGET) r |= USAGE_RENDERTARGET;
  if(bind&D3D11_BIND_SHADER_RESOURCE) r |= USAGE_SHADER_RESOURCE;
  if(bind&D3D11_BIND_DEPTH_STENCIL) r |= USAGE_DEPTH_STENCIL;
  if(multisample) r |= USAGE_MULTISAMPLE;
  return r;
}
inline uint32_t BSS_FASTCALL psDirectX11::_filtertodx11(FILTERS filter)
{
  switch(filter)
  {
  case FILTER_NEAREST:
    return D3DX11_FILTER_POINT;
  case FILTER_BOX:
    return D3DX11_FILTER_BOX;
  case FILTER_LINEAR:
    return D3DX11_FILTER_LINEAR;
  case FILTER_TRIANGLE:
    return D3DX11_FILTER_TRIANGLE;
  }
  return D3DX11_FILTER_NONE;
}

D3D11_PRIMITIVE_TOPOLOGY BSS_FASTCALL psDirectX11::_getdx11topology(PRIMITIVETYPE type)
{
  switch(type)
  {
  case POINTLIST: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case LINELIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  case LINESTRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
  case TRIANGLELIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case TRIANGLESTRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
  }
  return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
void psDirectX11::_setcambuf(ID3D11Buffer* buf, const float(&cam)[4][4], const float(&world)[4][4])
{
  float* r = (float*)LockBuffer(buf, LOCK_WRITE_DISCARD);
  MEMCPY(r, 16 * sizeof(float), cam, 16 * sizeof(float));
  MEMCPY(r + 16, 16 * sizeof(float), world, 16 * sizeof(float));
  UnlockBuffer(buf);
}
ID3D11View* psDirectX11::_createshaderview(ID3D11Resource* src)
{
  PROFILE_FUNC();
  D3D11_SHADER_RESOURCE_VIEW_DESC desc;
  D3D11_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D11_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
  {
    D3D11_TEXTURE1D_DESC desc1d;
    ((ID3D11Texture1D*)src)->GetDesc(&desc1d);

    desc.Format = desc1d.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
    desc.Texture1D.MostDetailedMip = 0;
    desc.Texture1D.MipLevels = -1;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc2d;
    ((ID3D11Texture2D*)src)->GetDesc(&desc2d);

    desc.Format = desc2d.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = -1;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  {
    D3D11_TEXTURE3D_DESC desc3d;
    ((ID3D11Texture3D*)src)->GetDesc(&desc3d);

    desc.Format = desc3d.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    desc.Texture3D.MostDetailedMip = 0;
    desc.Texture3D.MipLevels = -1;
  }
  break;
  }

  ID3D11ShaderResourceView* r = 0;
  if(FAILED(_device->CreateShaderResourceView(src, &desc, &r)))
  {
    PSLOG(1) << "_createshaderview failed" << std::endl;
    return 0;
  }
  return r;
}
ID3D11View* psDirectX11::_creatertview(ID3D11Resource* src)
{
  PROFILE_FUNC();
  D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
  D3D11_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D11_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc2d;
    ((ID3D11Texture2D*)src)->GetDesc(&desc2d);

    rtDesc.Format = desc2d.Format;
    rtDesc.ViewDimension = (desc2d.SampleDesc.Count > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D.MipSlice = 0;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
    break;
  }

  ID3D11RenderTargetView* r = 0;
  if(FAILED(_device->CreateRenderTargetView(src, &rtDesc, &r)))
  {
    PSLOG(1) << "_creatertview failed" << std::endl;
    return 0;
  }
  return r;
}
ID3D11View* psDirectX11::_createdepthview(ID3D11Resource* src)
{
  PROFILE_FUNC();
  D3D11_DEPTH_STENCIL_VIEW_DESC dDesc;
  D3D11_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D11_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
    break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc2d;
    ((ID3D11Texture2D*)src)->GetDesc(&desc2d);

    dDesc.Format = desc2d.Format;
    dDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dDesc.Texture2D.MipSlice = 0;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
    break;
  }

  ID3D11DepthStencilView* r = 0;
  if(FAILED(_device->CreateDepthStencilView(src, &dDesc, &r)))
  {
    PSLOG(1) << "_createdepthview failed" << std::endl;
    return 0;
  }
  return r;
}
bool psDirectX11::_customfilter(FILTERS filter)
{
  switch(filter)
  {
  case FILTER_PREMULTIPLY_SRGB:
  case FILTER_PREMULTIPLY:
  case FILTER_ALPHABOX:
  case FILTER_DEBUG:
    return true;
  }
  return false;
}

psShader* psDirectX11::_getfiltershader(FILTERS filter)
{
  switch(filter)
  {
  case FILTER_PREMULTIPLY_SRGB:
  case FILTER_PREMULTIPLY: return _premultiplyfilter;
  case FILTER_ALPHABOX: return _alphaboxfilter;
  case FILTER_DEBUG: return library.DEBUG;
  }
  return 0;
}

psVeciu _getrendertargetsize(ID3D11RenderTargetView* view)
{
  D3D11_RENDER_TARGET_VIEW_DESC desc;
  ID3D11Resource* res;
  D3D11_RESOURCE_DIMENSION dim;
  view->GetResource(&res);
  view->GetDesc(&desc);
  res->GetType(&dim);
  switch(dim)
  {
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
  {
    D3D11_TEXTURE1D_DESC desc;
    ((ID3D11Texture1D*)res)->GetDesc(&desc);
    return psVeciu(desc.Width, 1);
  }
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc;
    ((ID3D11Texture2D*)res)->GetDesc(&desc);
    return psVeciu(desc.Width, desc.Height);
  }
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  {
    D3D11_TEXTURE3D_DESC desc;
    ((ID3D11Texture3D*)res)->GetDesc(&desc);
    return psVeciu(desc.Width, desc.Height);
  }
  }
  return psVeciu(0);
}
