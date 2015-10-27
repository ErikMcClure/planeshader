// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDirectX11.h"
#include "psDirectX10_fsquadVS.h"
#include "psEngine.h"
#include "psTex.h"
#include "psStateblock.h"
#include "bss-util/cStr.h"
#include "bss-util/cDynArray.h"
#include "bss-util/profiler.h"
#include "psColor.h"

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
psDirectX11::psDirectX11(const psVeciu& dim, unsigned int antialias, bool vsync, bool fullscreen, HWND hwnd) : psDriver(dim), _device(0), _vsync(vsync), _lasterr(0),
_backbuffer(0), _extent(10000, 1), _dpi(BASE_DPI), _infoqueue(0)
{
  PROFILE_FUNC();
  memset(&library, 0, sizeof(SHADER_LIBRARY));

  _cam_def = 0;
  _cam_usr = 0;
  _proj_def = 0;
  _proj_usr = 0;
  _rectvertbuf = 0;
  _batchindexbuf = 0;
  _batchvertbuf = 0;
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
  IDXGIAdapter* adapter = _createfactory(hwnd, _dpi, output);

  LOGFAILURE(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, DEVICEFLAGS, NULL, 0, D3D11_SDK_VERSION, &_device, &_featurelevel, &_context), "D3D11CreateDevice failed with error: ", _lasterr);
 
#ifdef BSS_DEBUG
  _device->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)&_infoqueue);
#endif

  DXGI_MODE_DESC target = {
    dim.x,
    dim.y,
    { 0, 0 },
    DXGI_FORMAT_R8G8B8A8_UNORM,
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

  _cam_def = (ID3D11Buffer*)CreateBuffer(sizeof(float) * 4 * 4 * 2, USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC);
  _cam_usr = (ID3D11Buffer*)CreateBuffer(sizeof(float) * 4 * 4 * 2, USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC);
  _proj_def = (ID3D11Buffer*)CreateBuffer(sizeof(float) * 4 * 4 * 2, USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC);
  _proj_usr = (ID3D11Buffer*)CreateBuffer(sizeof(float) * 4 * 4 * 2, USAGE_CONSTANT_BUFFER | USAGE_DYNAMIC);
  _scissorstack.Push(RECT_ZERO); //push the initial scissor rect (it'll get set in ApplyCamera)

  LOGFAILURE(_factory->CreateSwapChain(_device, &swapdesc, &_swapchain), "CreateSwapChain failed with error: ", _geterror(_lasterr));
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
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint)-1 }
    };

    library.IMAGE = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage0.hlsl");

    ELEMENT_DESC desc[3] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 }
    };

    library.IMAGE0 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage2.hlsl");

    ELEMENT_DESC desc[5] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32G32B32A32F, 0, (uint)-1 }
    };

    library.IMAGE2 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsimage3.hlsl");

    ELEMENT_DESC desc[6] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 2, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 3, FMT_R32G32B32A32F, 0, (uint)-1 }
    };

    library.IMAGE3 = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fscircle.hlsl");

    library.CIRCLE = psShader::MergeShaders(2, library.IMAGE, psShader::CreateShader(0, 0, 1,
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0)));
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fsline.hlsl");

    ELEMENT_DESC desc[2] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 }
    };

    library.LINE = psShader::CreateShader(desc, 2,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0));
    library.POLYGON = library.LINE;
  }

  {
    auto a = fnload(cStr(psEngine::Instance()->GetMediaPath()) + "/fspoint.hlsl");

    ELEMENT_DESC desc[4] = {
      { ELEMENT_POSITION, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 0, FMT_R32G32B32A32F, 0, (uint)-1 },
      { ELEMENT_COLOR, 0, FMT_R8G8B8A8, 0, (uint)-1 },
      { ELEMENT_TEXCOORD, 1, FMT_R32G32B32A32F, 0, (uint)-1 }
    };

    library.PARTICLE = psShader::CreateShader(desc, 3,
      &SHADER_INFO::From<void>(a.get(), "mainVS", VERTEX_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainPS", PIXEL_SHADER_4_0, 0),
      &SHADER_INFO::From<void>(a.get(), "mainGS", GEOMETRY_SHADER_4_0, 0));
  }

  _rectvertbuf = CreateBuffer(RECTBUFSIZE, USAGE_VERTEX | USAGE_DYNAMIC, 0);
  _rectobjbuf.indices = 0;
  _rectobjbuf.mode = POINTLIST;
  _rectobjbuf.nindice = 0;
  _rectobjbuf.vsize = sizeof(DX11_rectvert);
  _rectobjbuf.verts = _rectvertbuf;
  _rectobjbuf.nvert = BATCHSIZE;

  unsigned short ind[BATCHSIZE * 3];
  unsigned int k = 0;
  for(unsigned short i = 0; i < BATCHSIZE - 3; ++i)
  {
    ind[k++] = 0;
    ind[k++] = 1 + i;
    ind[k++] = 2 + i;
  }
  _batchindexbuf = CreateBuffer(sizeof(unsigned short)*BATCHSIZE * 3, USAGE_INDEX | USAGE_IMMUTABLE, ind);
  _batchvertbuf = CreateBuffer(sizeof(DX11_simplevert)*BATCHSIZE, USAGE_VERTEX | USAGE_DYNAMIC, 0);
  _batchobjbuf.indices = _batchindexbuf;
  _batchobjbuf.ifmt = FMT_INDEX16;
  _batchobjbuf.nindice = BATCHSIZE * 3;
  _batchobjbuf.mode = TRIANGLELIST;
  _batchobjbuf.vsize = sizeof(DX11_simplevert);
  _batchobjbuf.verts = _batchvertbuf;
  _batchobjbuf.nvert = BATCHSIZE;

  _ptobjbuf.indices = 0;
  _ptobjbuf.mode = POINTLIST;
  _ptobjbuf.nindice = 0;
  _ptobjbuf.vsize = sizeof(DX11_simplevert);
  _ptobjbuf.verts = _batchvertbuf;
  _ptobjbuf.nvert = BATCHSIZE;

  _lineobjbuf.indices = 0;
  _lineobjbuf.mode = LINELIST;
  _lineobjbuf.nindice = 0;
  _lineobjbuf.vsize = sizeof(DX11_simplevert);
  _lineobjbuf.verts = _batchvertbuf;
  _lineobjbuf.nvert = BATCHSIZE;

  // Create default stateblock and sampler state, then activate the default stateblock.
  _defaultSB = (DX11_SB*)CreateStateblock(0);
  _defaultSS = (ID3D11SamplerState*)CreateTexblock(0);
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
  if(_rectvertbuf)
    FreeResource(_rectvertbuf, RES_VERTEXBUF);
  if(_batchindexbuf)
    FreeResource(_batchindexbuf, RES_INDEXBUF);
  if(_batchvertbuf)
    FreeResource(_batchvertbuf, RES_VERTEXBUF);
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
  HRESULT hr = _swapchain->Present(_vsync, DXGI_PRESENT_RESTART); // DXGI_PRESENT_DO_NOT_SEQUENCE doesn't seem to do anything other than break everything.

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
void BSS_FASTCALL psDirectX11::Draw(psVertObj* buf, FLAG_TYPE flags, const float(&transform)[4][4])
{
  PROFILE_FUNC();
  unsigned int offset = 0;
  _context->IASetVertexBuffers(0, 1, (ID3D11Buffer**)&buf->verts, &buf->vsize, &offset);
  _context->IASetPrimitiveTopology(_getdx11topology(buf->mode));

  ID3D11Buffer* cam = (flags&PSFLAG_FIXED) ? _proj_def : _cam_def;
  if(transform != identity)
    _setcambuf(cam = ((flags&PSFLAG_FIXED) ? _proj_usr : _cam_usr), matView.v, transform);

  _context->VSSetConstantBuffers(0, 1, (ID3D11Buffer**)&cam);
  _context->GSSetConstantBuffers(0, 1, (ID3D11Buffer**)&cam);

  if(!buf->indices)
    _context->Draw(buf->nvert, 0);
  else
  {
    _context->IASetIndexBuffer((ID3D11Buffer*)buf->indices, (DXGI_FORMAT)FMTtoDXGI(buf->ifmt), 0);
    _context->DrawIndexed(buf->nindice, 0, 0);
  }
}
void BSS_FASTCALL psDirectX11::DrawRect(const psRectRotateZ rect, const psRect* uv, unsigned char numuv, unsigned int color, const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags, const float(&xform)[4][4])
{ // Because we have to send the rect position in SOMEHOW and DX11 forces us to send matrices through the shaders, we will lock a buffer no matter what we do. 
  PROFILE_FUNC();
  DrawRectBatchBegin(texes, numtex, numuv, flags); // So it's easy and just as fast to "batch render" a single rect instead of try and use an existing vertex buffer
  DrawRectBatch(rect, uv, color, xform);
  DrawRectBatchEnd(xform);
}
void BSS_FASTCALL psDirectX11::DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned char numuv, FLAG_TYPE flags)
{
  PROFILE_FUNC();
  _rectobjbuf.vsize = sizeof(DX11_rectvert) + sizeof(psRect)*numuv;
  _lockedrectbuf = (DX11_rectvert*)LockBuffer(_rectobjbuf.verts, LOCK_WRITE_DISCARD);
  _lockedcount = 0;
  _lockedrectuv = numuv;
  _lockedflag = flags;
  SetTextures(texes, numtex, PIXEL_SHADER_1_1);
}
void BSS_FASTCALL psDirectX11::DrawRectBatch(const psRectRotateZ rect, const psRect* uv, unsigned int color, const float(&xform)[4][4])
{
  DX11_rectvert* buf = (DX11_rectvert*)(((char*)_lockedrectbuf) + (_lockedcount*_rectobjbuf.vsize));
  *buf = { rect.left, rect.top, rect.z, rect.rotation,
    rect.right - rect.left, rect.bottom - rect.top, rect.pivot.x, rect.pivot.y,
    color };
  memcpy(buf + 1, uv, sizeof(psRect)*_lockedrectuv);

  if(((++_lockedcount)*_rectobjbuf.vsize) >= RECTBUFSIZE)
  {
    DrawRectBatchEnd(xform);
    _lockedrectbuf = (DX11_rectvert*)LockBuffer(_rectobjbuf.verts, LOCK_WRITE_DISCARD);
    _lockedcount = 0;
  }
}
void psDirectX11::DrawRectBatchEnd(const float(&xform)[4][4])
{
  UnlockBuffer(_rectobjbuf.verts);
  _rectobjbuf.nvert = _lockedcount;
  Draw(&_rectobjbuf, _lockedflag, xform);
}
void BSS_FASTCALL psDirectX11::DrawPolygon(const psVec* verts, int num, psVec3D offset, unsigned long vertexcolor, FLAG_TYPE flags, const float(&transform)[4][4])
{
  DX11_simplevert* buf = (DX11_simplevert*)LockBuffer(_batchobjbuf.verts, LOCK_WRITE_DISCARD);
  if(num > BATCHSIZE) return;
  for(int i = 0; i < num; ++i)
  {
    buf[i].x = verts[i].x + offset.x;
    buf[i].y = verts[i].y + offset.y;
    buf[i].z = offset.z;
    buf[i].w = 1;
    buf[i].color = vertexcolor;
  }
  UnlockBuffer(_batchobjbuf.verts);
  _batchobjbuf.nvert = num;
  _batchobjbuf.nindice = (num - 2) * 3;
  Draw(&_batchobjbuf, flags, transform);
}
void BSS_FASTCALL psDirectX11::DrawPolygon(const psVertex* verts, int num, FLAG_TYPE flags, const float(&transform)[4][4])
{
  static_assert(sizeof(psVertex) == sizeof(DX11_simplevert), "Error, psVertex is not equal to DX11_simplevert");
  DX11_simplevert* buf = (DX11_simplevert*)LockBuffer(_batchobjbuf.verts, LOCK_WRITE_DISCARD);
  if(num > BATCHSIZE) return;
  memcpy(buf, verts, num*sizeof(psVertex));
  UnlockBuffer(_batchobjbuf.verts);
  _batchobjbuf.nvert = num;
  _batchobjbuf.nindice = (num - 2) * 3;
  Draw(&_batchobjbuf, flags, transform);
}
void BSS_FASTCALL psDirectX11::DrawPointsBegin(const psTex* const* texes, unsigned char numtex, float size, FLAG_TYPE flags)
{
  _lockedptbuf = (DX11_simplevert*)LockBuffer(_ptobjbuf.verts, LOCK_WRITE_DISCARD);
  _lockedcount = 0;
  _lockedflag = flags;
  library.PARTICLE->SetConstants<float, 2>(size);
  SetTextures(texes, numtex, PIXEL_SHADER_1_1);
}
void BSS_FASTCALL psDirectX11::DrawPoints(psVertex* particles, unsigned int num)
{
  static_assert(sizeof(psVertex) == sizeof(DX11_simplevert), "Error, psVertex is not equal to DX11_simplevert");

  for(unsigned int i = num + _lockedcount; i > BATCHSIZE; i -= BATCHSIZE)
  {
    memcpy(_lockedptbuf + _lockedcount, particles, (BATCHSIZE - _lockedcount)*sizeof(DX11_simplevert));
    UnlockBuffer(_lockedptbuf);
    _ptobjbuf.nvert = BATCHSIZE;
    Draw(&_ptobjbuf, _lockedflag);
    _lockedptbuf = (DX11_simplevert*)LockBuffer(_ptobjbuf.verts, LOCK_WRITE_DISCARD);
    _lockedcount = 0;
    num -= BATCHSIZE;
    particles += BATCHSIZE;
  }
  memcpy(_lockedptbuf + _lockedcount, particles, num*sizeof(DX11_simplevert)); // We can do this because we know num+_lockedptcount does not exceed MAX_POINTS
  _lockedcount += num;
}
void psDirectX11::DrawPointsEnd()
{
  PROFILE_FUNC();
  UnlockBuffer(_lockedptbuf);
  _ptobjbuf.nvert = _lockedcount;
  Draw(&_ptobjbuf, _lockedflag);
}
void BSS_FASTCALL psDirectX11::DrawLinesStart(FLAG_TYPE flags)
{
  _lockedlinebuf = (DX11_simplevert*)LockBuffer(_lineobjbuf.verts, LOCK_WRITE_DISCARD);
  _lockedcount = 0;
  _lockedflag = flags;
  SetTextures(0, 0, PIXEL_SHADER_1_1);
}
void BSS_FASTCALL psDirectX11::DrawLines(const psLine& line, float Z1, float Z2, unsigned long vertexcolor, const float(&transform)[4][4])
{
  _lockedlinebuf[_lockedcount++] = { line.x1, line.y1, Z1, 1, vertexcolor };
  _lockedlinebuf[_lockedcount++] = { line.x2, line.y2, Z2, 1, vertexcolor };

  if(_lockedcount >= BATCHSIZE)
  {
    DrawLinesEnd(transform);
    _lockedlinebuf = (DX11_simplevert*)LockBuffer(_lineobjbuf.verts, LOCK_WRITE_DISCARD);
    _lockedcount = 0;
  }
}
void psDirectX11::DrawLinesEnd(const float(&transform)[4][4])
{
  PROFILE_FUNC();
  UnlockBuffer(_lineobjbuf.verts);
  _lineobjbuf.nvert = _lockedcount;
  Draw(&_lineobjbuf, _lockedflag, transform);
}
void BSS_FASTCALL psDirectX11::ApplyCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport)
{
  PROFILE_FUNC();
  D3D11_VIEWPORT vp = { (INT)viewport.left, (INT)viewport.top, viewport.right, viewport.bottom, 0.0f, 1.0f };
  _context->RSSetViewports(1, &vp);
  _scissorstack[0].left = viewport.left;
  _scissorstack[0].top = viewport.top;
  _scissorstack[0].right = viewport.right;
  _scissorstack[0].bottom = viewport.bottom;

  if(_scissorstack.Length() <= 1) // if we are currently using the default scissor rect, re-apply with new dimensions
    PopScissorRect();

  // Build a custom projection matrix that discards the znear scaling. The formula is:
  // 2/w  0     0              0
  // 0    -2/h  0              0
  // 0    0     zf/(zf-zn)     1
  // 0    0     zn*zf/(zn-zf)  0
  // We negate the y axis to get our right handed coordinate system, where +y points down, +x points
  // to the right, and +z points into the screen.
  memset((float*)matProj.v, 0, sizeof(Matrix<float, 4, 4>));
  float znear = _extent.x;
  float zfar = _extent.y;
  matProj.v[0][0] = 2.0f / vp.Width;
  matProj.v[1][1] = -2.0f / vp.Height;
  matProj.v[2][2] = zfar / (zfar - znear);
  matProj.v[3][2] = (znear*zfar) / (znear - zfar);
  matProj.v[2][3] = 1;

  // Cameras have an origin and rotational pivot in their center, so we have to adjust our effective pivot
  psVec adjust = pivot - (psVec(vp.Width, vp.Height) * 0.5f);

  if(_dpi != psVeciu(BASE_DPI)) // Do we need to do DPI scaling?
  {
    Matrix<float, 4, 4> m;
    Matrix<float, 4, 4>::AffineScaling(_dpi.x / (float)BASE_DPI, _dpi.y / (float)BASE_DPI, 1, m.v);
    matProj *= m;
  }

  BSS_ALIGN(16) Matrix<float, 4, 4> cam;
  Matrix<float, 4, 4>::AffineTransform_T(pos.x - adjust.x, pos.y - adjust.y, pos.z, rotation, adjust.x, adjust.y, cam);
  cam.Inverse(matView.v); // inverse cam and store the result in matView

  Matrix<float, 4, 4> defaultCam;
  Matrix<float, 4, 4>::Translation_T(vp.Width*-0.5f, vp.Height*-0.5f, 1, defaultCam);

  matViewProj = matView * matProj;
  _setcambuf(_cam_def, matViewProj.v, identity);
  _setcambuf(_cam_usr, matViewProj.v, identity);
  matProj = defaultCam*matProj;
  _setcambuf(_proj_def, matProj.v, identity);
  _setcambuf(_proj_usr, matProj.v, identity);
}
void BSS_FASTCALL psDirectX11::ApplyCamera3D(const float(&m)[4][4], const psRectiu& viewport)
{
  PROFILE_FUNC();
  D3D11_VIEWPORT vp = { viewport.left, viewport.top, viewport.right, viewport.bottom, 0.0f, 1.0f };
  _context->RSSetViewports(1, &vp);

  memcpy_s(&matProj, sizeof(float) * 16, m, sizeof(float) * 16);
  memcpy_s(&matViewProj, sizeof(float) * 16, m, sizeof(float) * 16);

  Matrix<float, 4, 4>::Identity(matView);

  _setcambuf(_cam_def, matViewProj.v, identity); // matViewProj is identical to matProj here so PSFLAG_FIXED will have no effect
  _setcambuf(_cam_usr, matViewProj.v, identity);
  _setcambuf(_proj_def, matProj.v, identity);
  _setcambuf(_proj_usr, matProj.v, identity);
}
// Applies the camera transform (or it's inverse) according to the flags to a point.
psVec3D BSS_FASTCALL psDirectX11::TransformPoint(const psVec3D& point, FLAG_TYPE flags) const
{
  Vector<float, 4> v = { point.x, point.y, point.z, 1 };
  Vector<float, 4> out = v * ((flags&PSFLAG_FIXED) ? matProj : matViewProj);
  return out.xyz;
}
psVec3D BSS_FASTCALL psDirectX11::ReversePoint(const psVec3D& point, FLAG_TYPE flags) const
{
  Vector<float, 4> v = { point.x, point.y, point.z, 1 };
  Vector<float, 4> out = v * ((flags&PSFLAG_FIXED) ? matProj : matViewProj).Inverse();
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
void BSS_FASTCALL psDirectX11::SetExtent(float znear, float zfar) { _extent.x = znear; _extent.y = zfar; }
void* BSS_FASTCALL psDirectX11::CreateBuffer(size_t bytes, unsigned int usage, const void* initdata)
{
  PROFILE_FUNC();
  D3D11_BUFFER_DESC desc = {
    bytes,
    (D3D11_USAGE)_usagetodxtype(usage),
    _usagetobind(usage),
    _usagetocpuflag(usage),
    _usagetomisc(usage),
  };

  D3D11_SUBRESOURCE_DATA subdata = { initdata, 0, 0 };

  ID3D11Buffer* buf;
  LOGFAILURERETNULL(_device->CreateBuffer(&desc, !initdata ? 0 : &subdata, &buf), "CreateBuffer failed");
  return buf;
}
void* BSS_FASTCALL psDirectX11::LockBuffer(void* target, unsigned int flags)
{
  PROFILE_FUNC();
  D3D11_MAPPED_SUBRESOURCE r;
  LOGFAILURERETNULL(_context->Map((ID3D11Buffer*)target, 0, (D3D11_MAP)(flags&LOCK_TYPEMASK), (flags&LOCK_DONOTWAIT) ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &r), "LockBuffer failed with error ", _geterror(_lasterr))
  return r.pData;
}

ID3D11Resource* psDirectX11::_textotex2D(void* t)
{
  ID3D11Resource* r;
  ((ID3D11ShaderResourceView*)t)->GetResource(&r);
  return r;
}

void* BSS_FASTCALL psDirectX11::LockTexture(void* target, unsigned int flags, unsigned int& pitch, unsigned char miplevel)
{
  PROFILE_FUNC();
  D3D11_MAPPED_SUBRESOURCE tex;

  _context->Map(_textotex2D(target), D3D11CalcSubresource(miplevel, 0, 1), (D3D11_MAP)(flags&LOCK_TYPEMASK), (flags&LOCK_DONOTWAIT) ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &tex);
  pitch = tex.RowPitch;
  PROCESSQUEUE();
  return tex.pData;
}

void BSS_FASTCALL psDirectX11::UnlockBuffer(void* target)
{
  PROFILE_FUNC();
  _context->Unmap((ID3D11Buffer*)target, 0);
  PROCESSQUEUE();
}
void BSS_FASTCALL psDirectX11::UnlockTexture(void* target, unsigned char miplevel)
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
void* BSS_FASTCALL psDirectX11::CreateTexture(psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, const void* initdata, void** additionalview, psTexblock* texblock)
{
  PROFILE_FUNC();
  ID3D11Texture2D* tex = 0;
  if(_samples.Count > 1) usage &= ~USAGE_AUTOGENMIPMAP;
  D3D11_TEXTURE2D_DESC desc = { dim.x, dim.y, (_samples.Count>1) ? 1 : miplevels, 1, (DXGI_FORMAT)FMTtoDXGI(format), (usage & USAGE_STAGING) ? DEFAULT_SAMPLE_DESC : _samples, (D3D11_USAGE)_usagetodxtype(usage), _usagetobind(usage), _usagetocpuflag(usage), _usagetomisc(usage) };
  D3D11_SUBRESOURCE_DATA subdata = { initdata, (((psColor::BitsPerPixel(format)*dim.x) + 7) >> 3), 0 }; // The +7 here is to force the per-line bits to correctly round up the number of bytes.

  LOGFAILURERETNULL(_device->CreateTexture2D(&desc, !initdata ? 0 : &subdata, &tex), "CreateTexture failed, error: ", _geterror(_lasterr))
    if(usage&USAGE_RENDERTARGET)
      *additionalview = _creatertview(tex);
    else if(usage&USAGE_DEPTH_STENCIL)
      *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}

void BSS_FASTCALL psDirectX11::_loadtexture(D3DX11_IMAGE_LOAD_INFO* info, unsigned int usage, FORMATS format, unsigned char miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim)
{
  memset(info, -1, sizeof(D3DX11_IMAGE_LOAD_INFO));
  info->MipLevels = miplevels;
  info->FirstMipLevel = 0;
  info->Filter = _filtertodx11(loadfilter);
  info->MipFilter = _filtertodx11(mipfilter);
  if(dim.x != 0) info->Width = dim.x;
  if(dim.y != 0) info->Height = dim.y;
  info->Usage = (D3D11_USAGE)_usagetodxtype(usage);
  info->BindFlags = _usagetobind(usage);
  info->CpuAccessFlags = _usagetocpuflag(usage);
  info->MiscFlags = _usagetomisc(usage);
  info->pSrcInfo = 0;
  if(format != FMT_UNKNOWN) info->Format = (DXGI_FORMAT)FMTtoDXGI(format);
}

void* BSS_FASTCALL psDirectX11::LoadTexture(const char* path, unsigned int usage, FORMATS format, void** additionalview, unsigned char miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, psTexblock* texblock)
{
  PROFILE_FUNC();
  D3DX11_IMAGE_LOAD_INFO info;
  if(_samples.Count > 1) usage &= ~USAGE_AUTOGENMIPMAP;
  _loadtexture(&info, usage, format, (_samples.Count>1) ? 1 : miplevels, mipfilter, loadfilter, dim);

  ID3D11Resource* tex = 0;
  LOGFAILURERETNULL(D3DX11CreateTextureFromFileW(_device, cStrW(path), &info, 0, &tex, 0), "LoadTexture failed with error ", _geterror(_lasterr), " for ", path);

#ifdef BSS_DEBUG
  tex->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(path), path);
#endif
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}

void* BSS_FASTCALL psDirectX11::LoadTextureInMemory(const void* data, size_t datasize, unsigned int usage, FORMATS format, void** additionalview, unsigned char miplevels, FILTERS mipfilter, FILTERS loadfilter, psVeciu dim, psTexblock* texblock)
{
  PROFILE_FUNC();
  D3DX11_IMAGE_LOAD_INFO info;
  if(_samples.Count > 1) usage &= ~USAGE_AUTOGENMIPMAP;
  _loadtexture(&info, usage, format, (_samples.Count>1) ? 1 : miplevels, mipfilter, loadfilter, dim);

  ID3D11Resource* tex = 0;
  LOGFAILURERETNULL(D3DX11CreateTextureFromMemory(_device, data, datasize, &info, 0, &tex, 0), "LoadTexture failed for ", data);
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE) != 0) ? _createshaderview(tex) : new DX11_EmptyView(tex);
}
void BSS_FASTCALL psDirectX11::CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, unsigned char miplevel)
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

void BSS_FASTCALL psDirectX11::PushScissorRect(const psRect& rect)
{
  PROFILE_FUNC();
  psVec scale = GetDPIScale();
  psRectl r(fFastTruncate(floor(rect.left*scale.x)), fFastTruncate(floor(rect.top*scale.y)), fFastTruncate(ceilf(rect.right*scale.x)), fFastTruncate(ceil(rect.bottom*scale.y)));
  _scissorstack.Push(r);
  _context->RSSetScissorRects(1, (D3D11_RECT*)&r);
}

void psDirectX11::PopScissorRect()
{
  PROFILE_FUNC();
  if(_scissorstack.Length()>1)
    _scissorstack.Discard();
  _context->RSSetScissorRects(1, (D3D11_RECT*)&_scissorstack.Peek());
}

void BSS_FASTCALL psDirectX11::SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil)
{
  PROFILE_FUNC();
  unsigned char realnum = (num<1) ? 1 : num;
  DYNARRAY(ID3D11RenderTargetView*, views, realnum);
  for(unsigned char i = 0; i < num; ++i) views[i] = (ID3D11RenderTargetView*)texes[i]->GetView();
  if(num<1 || !views[0]) views[0] = (ID3D11RenderTargetView*)_defaultrt->GetView();
  _context->OMSetRenderTargets(realnum, views, (ID3D11DepthStencilView*)(!depthstencil ? 0 : depthstencil->GetView()));
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

void BSS_FASTCALL psDirectX11::SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader)
{
  PROFILE_FUNC();
  DYNARRAY(ID3D11ShaderResourceView*, views, num);
  DYNARRAY(ID3D11SamplerState*, states, num);
  for(unsigned char i = 0; i < num; ++i)
  {
    views[i] = (ID3D11ShaderResourceView*)texes[i]->GetRes();
    const psTexblock* texblock = texes[i]->GetTexblock();
    states[i] = !texblock ? _defaultSS : (ID3D11SamplerState*)texblock->GetSB();
  }

  if(shader <= VERTEX_SHADER_5_0)
  {
    _context->VSSetShaderResources(0, num, views);
    _context->VSSetSamplers(0, num, states);
  }
  else if(shader <= PIXEL_SHADER_5_0)
  {
    _context->PSSetShaderResources(0, num, views);
    _context->PSSetSamplers(0, num, states);
  }
  else if(shader <= GEOMETRY_SHADER_5_0)
  {
    _context->GSSetShaderResources(0, num, views);
    _context->GSSetSamplers(0, num, states);
  }
}
// Builds a stateblock from the given set of state changes
void* BSS_FASTCALL psDirectX11::CreateStateblock(const STATEINFO* states)
{
  PROFILE_FUNC();
  // Build each type of stateblock and independently check for duplicates.
  DX11_SB* sb = new DX11_SB { 0, 0, 0, 0, { 1.0f, 1.0f, 1.0f, 1.0f }, 0xffffffff };
  D3D11_BLEND_DESC bs = { 0, 0, { 1, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL } };
  D3D11_DEPTH_STENCIL_DESC ds = { 1, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, 0, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
  { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
  { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS } };
  D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, 0, 0, 0.0f, 0.0f, 1, 1, 0, 1 };
  
  const STATEINFO* cur = states;
  if(cur != 0 && cur->type == TYPE_BLEND_ALPHATOCOVERAGEENABLE) bs.AlphaToCoverageEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_BLEND_INDEPENDENTBLENDENABLE) bs.IndependentBlendEnable = (cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_BLENDENABLE && cur->index <= 7) bs.RenderTarget[cur->index].BlendEnable = (cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_SRCBLEND && cur->index <= 7) bs.RenderTarget[cur->index].SrcBlend = (D3D11_BLEND)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_DESTBLEND && cur->index <= 7) bs.RenderTarget[cur->index].DestBlend = (D3D11_BLEND)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_BLENDOP && cur->index <= 7) bs.RenderTarget[cur->index].BlendOp = (D3D11_BLEND_OP)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_SRCBLENDALPHA && cur->index <= 7) bs.RenderTarget[cur->index].SrcBlendAlpha = (D3D11_BLEND)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_DESTBLENDALPHA && cur->index <= 7) bs.RenderTarget[cur->index].DestBlendAlpha = (D3D11_BLEND)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_BLENDOPALPHA && cur->index <= 7) bs.RenderTarget[cur->index].BlendOpAlpha = (D3D11_BLEND_OP)(cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_RENDERTARGETWRITEMASK && cur->index <= 7) bs.RenderTarget[cur->index].RenderTargetWriteMask = (cur++)->value;
  while(cur != 0 && cur->type == TYPE_BLEND_BLENDFACTOR && cur->index <= 3) sb->blendfactor[cur->index] = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_BLEND_SAMPLEMASK) sb->sampleMask = (cur++)->value;

  if(cur != 0 && cur->type == TYPE_DEPTH_DEPTHENABLE) ds.DepthEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_DEPTHWRITEMASK) ds.DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_DEPTHFUNC) ds.DepthFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_STENCILENABLE) ds.StencilEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_STENCILREADMASK) ds.StencilReadMask = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_STENCILWRITEMASK) ds.StencilWriteMask = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_FRONT_STENCILFAILOP) ds.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_FRONT_STENCILDEPTHFAILOP) ds.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_FRONT_STENCILPASSOP) ds.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_FRONT_STENCILFUNC) ds.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_BACK_STENCILFAILOP) ds.BackFace.StencilFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_BACK_STENCILDEPTHFAILOP) ds.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_BACK_STENCILPASSOP) ds.BackFace.StencilPassOp = (D3D11_STENCIL_OP)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_BACK_STENCILFUNC) ds.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_DEPTH_STENCILVALUE) sb->stencilref = (cur++)->value;

  if(cur != 0 && cur->type == TYPE_RASTER_FILLMODE) rs.FillMode = (D3D11_FILL_MODE)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_CULLMODE) rs.CullMode = (D3D11_CULL_MODE)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_FRONTCOUNTERCLOCKWISE) rs.FrontCounterClockwise = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_DEPTHBIAS) rs.DepthBias = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_DEPTHBIASCLAMP) rs.DepthBiasClamp = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_RASTER_SLOPESCALEDDEPTHBIAS) rs.SlopeScaledDepthBias = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_RASTER_DEPTHCLIPENABLE) rs.DepthClipEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_SCISSORENABLE) rs.ScissorEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_MULTISAMPLEENABLE) rs.MultisampleEnable = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_RASTER_ANTIALIASEDLINEENABLE) rs.AntialiasedLineEnable = (cur++)->value;

  LOGFAILURERETNULL(_device->CreateBlendState(&bs, &sb->bs), "CreateBlendState failed")
    LOGFAILURERETNULL(_device->CreateDepthStencilState(&ds, &sb->ds), "CreateDepthStencilState failed")
    LOGFAILURERETNULL(_device->CreateRasterizerState(&rs, &sb->rs), "CreateRasterizerState failed")

    return sb;
}

void* BSS_FASTCALL psDirectX11::CreateTexblock(const STATEINFO* states)
{
  D3D11_SAMPLER_DESC ss = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 16, D3D11_COMPARISON_NEVER, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, FLT_MAX };

  ID3D11SamplerState* ret;
  const STATEINFO* cur = states;
  if(cur != 0 && cur->type == TYPE_SAMPLER_FILTER) ss.Filter = (D3D11_FILTER)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_ADDRESSU) ss.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_ADDRESSV) ss.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_ADDRESSW) ss.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_MIPLODBIAS) ss.MipLODBias = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_MAXANISOTROPY) ss.MaxAnisotropy = (cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_COMPARISONFUNC) ss.ComparisonFunc = (D3D11_COMPARISON_FUNC)(cur++)->value;
  if(cur != 0 && cur->type == TYPE_SAMPLER_BORDERCOLOR1) ss.BorderColor[0] = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_BORDERCOLOR2) ss.BorderColor[1] = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_BORDERCOLOR3) ss.BorderColor[2] = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_BORDERCOLOR4) ss.BorderColor[3] = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_MINLOD) ss.MinLOD = (cur++)->valuef;
  if(cur != 0 && cur->type == TYPE_SAMPLER_MAXLOD) ss.MaxLOD = (cur++)->valuef;
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
void* BSS_FASTCALL psDirectX11::CreateLayout(void* shader, const ELEMENT_DESC* elements, unsigned char num)
{
  PROFILE_FUNC();
  ID3D10Blob* blob = (ID3D10Blob*)shader;
  return CreateLayout(blob->GetBufferPointer(), blob->GetBufferSize(), elements, num);
}
void* BSS_FASTCALL psDirectX11::CreateLayout(void* shader, size_t sz, const ELEMENT_DESC* elements, unsigned char num)
{
  PROFILE_FUNC();
  DYNARRAY(D3D11_INPUT_ELEMENT_DESC, descs, num);

  for(unsigned char i = 0; i < num; ++i)
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
    descs[i].Format = (DXGI_FORMAT)FMTtoDXGI(elements[i].format);
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
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags);
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
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags);
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
    ret.usage = (USAGETYPES)_reverseusage(desc.Usage, desc.MiscFlags, desc.BindFlags);
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

void BSS_FASTCALL psDirectX11::Resize(psVeciu dim, FORMATS format, bool fullscreen)
{
  PROFILE_FUNC();
  if(_backbuffer)
    _backbuffer->~psTex();
  _context->OMSetRenderTargets(0, 0, 0);
  _context->ClearState();
  LOGFAILURE(_swapchain->ResizeBuffers(1, dim.x, dim.y, (DXGI_FORMAT)FMTtoDXGI(format), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH), "ResizeBuffers failed with error code: ", _lasterr);
  _getbackbufferref();
  _resetscreendim();
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
      (USAGETYPES)_reverseusage(backbuffer_desc.Usage, backbuffer_desc.MiscFlags, backbuffer_desc.BindFlags),
      backbuffer_desc.MipLevels);
  else
    new(_backbuffer) psTex(0,
      _creatertview(backbuffer),
      psVeciu(backbuffer_desc.Width, backbuffer_desc.Height),
      DXGItoFMT(backbuffer_desc.Format),
      (USAGETYPES)_reverseusage(backbuffer_desc.Usage, backbuffer_desc.MiscFlags, backbuffer_desc.BindFlags),
      backbuffer_desc.MipLevels);

  backbuffer->Release();
}
void psDirectX11::_resetscreendim()
{
  SetDefaultRenderTarget();
  SetRenderTargets(0, 0, 0);
  rawscreendim = _backbuffer->GetDim();
  screendim = ((_dpi == psVeciu(BASE_DPI)) ? psVec(rawscreendim) : (psVec(rawscreendim) * (psVec(BASE_DPI) / psVec(_dpi))));
  ApplyCamera(psVec3D(0, 0, -1.0f), VEC_ZERO, 0, psRectiu(0, 0, rawscreendim.x, rawscreendim.y));
}
void BSS_FASTCALL psDirectX11::Clear(unsigned int color)
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
  for(unsigned char i = 0; i < 8; ++i)
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
  if(profile <= VERTEX_SHADER_5_0 && _lastVS != shader)
    _context->VSSetShader(_lastVS = (ID3D11VertexShader*)shader, 0, 0);
  else if(profile <= PIXEL_SHADER_5_0 && _lastPS != shader)
    _context->PSSetShader(_lastPS = (ID3D11PixelShader*)shader, 0, 0);
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
void psDirectX11::SetDPI(psVeciu dpi)
{
  _dpi = dpi;
}
psVeciu psDirectX11::GetDPI()
{
  return _dpi;
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

unsigned int BSS_FASTCALL psDirectX11::_usagetodxtype(unsigned int types)
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
unsigned int BSS_FASTCALL psDirectX11::_usagetocpuflag(unsigned int types)
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
unsigned int BSS_FASTCALL psDirectX11::_usagetomisc(unsigned int types)
{
  unsigned int r = 0;
  if(types&USAGE_AUTOGENMIPMAP) r |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
  if(types&USAGE_TEXTURECUBE) r |= D3D11_RESOURCE_MISC_TEXTURECUBE;
  return r;
}
unsigned int BSS_FASTCALL psDirectX11::_usagetobind(unsigned int types)
{
  unsigned int r = 0;
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
unsigned int BSS_FASTCALL psDirectX11::_reverseusage(unsigned int usage, unsigned int misc, unsigned int bind)
{
  unsigned int r = 0;
  switch(usage)
  {
  case D3D11_USAGE_DEFAULT: r = USAGE_DEFAULT;
  case D3D11_USAGE_IMMUTABLE: r = USAGE_IMMUTABLE;
  case D3D11_USAGE_DYNAMIC: r = USAGE_DYNAMIC;
  case D3D11_USAGE_STAGING: r = USAGE_STAGING;
  }
  if(misc&D3D11_RESOURCE_MISC_GENERATE_MIPS) r |= USAGE_AUTOGENMIPMAP;
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
  return r;
}
inline unsigned int BSS_FASTCALL psDirectX11::_filtertodx11(FILTERS filter)
{
  switch(filter)
  {
  case FILTER_NEAREST:
    return D3DX11_FILTER_POINT;
  case FILTER_LINEAR:
    return D3DX11_FILTER_LINEAR;
  case FILTER_BOX:
    return D3DX11_FILTER_BOX;
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
    desc.Texture1D.MipLevels = desc1d.MipLevels;
    desc.Texture1D.MostDetailedMip = desc1d.MipLevels - 1;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc2d;
    ((ID3D11Texture2D*)src)->GetDesc(&desc2d);

    desc.Format = desc2d.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = desc2d.MipLevels;
    desc.Texture2D.MostDetailedMip = desc2d.MipLevels - 1;
  }
  break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  {
    D3D11_TEXTURE3D_DESC desc3d;
    ((ID3D11Texture3D*)src)->GetDesc(&desc3d);

    desc.Format = desc3d.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    desc.Texture3D.MipLevels = desc3d.MipLevels;
    desc.Texture3D.MostDetailedMip = desc3d.MipLevels - 1;
  }
  break;
  }

  ID3D11ShaderResourceView* r = 0;
  if(FAILED(_device->CreateShaderResourceView(src, 0, &r)))
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
    PSLOG(1) << "_createshaderview failed" << std::endl;
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
    PSLOG(1) << "_createshaderview failed" << std::endl;
    return 0;
  }
  return r;
}