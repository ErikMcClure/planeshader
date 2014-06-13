// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDirectX10.h"
#include "psDirectX10_fsquadVS.h"
#include "psEngine.h"
#include "psTex.h"
#include "psStateblock.h"
#include "bss-util/cStr.h"
#include "bss-util/cDynArray.h"
#include "psColor.h"

using namespace planeshader;
using namespace bss_util;

#ifdef BSS_CPU_x86_64
#pragma comment(lib, "../lib/dxlib/x64/d3d10.lib")
#pragma comment(lib, "../lib/dxlib/x64/dxguid.lib")
#pragma comment(lib, "../lib/dxlib/x64/dxgi.lib")
#pragma comment(lib, "../lib/dxlib/x64/DxErr.lib")
#ifdef BSS_DEBUG
#pragma comment(lib, "../lib/dxlib/x64/d3dx10d.lib")
#else
#pragma comment(lib, "../lib/dxlib/x64/d3dx10.lib")
#endif
#else
#pragma comment(lib, "../lib/dxlib/d3d10.lib")
#pragma comment(lib, "../lib/dxlib/dxguid.lib")
#pragma comment(lib, "../lib/dxlib/DxErr.lib")
#pragma comment(lib, "../lib/dxlib/dxgi.lib")
#ifdef BSS_DEBUG
#pragma comment(lib, "../lib/dxlib/d3dx10d.lib")
#else
#pragma comment(lib, "../lib/dxlib/d3dx10.lib")
#endif
#endif

#define LOGEMPTY  
//#define LOGFAILURERET(fn,rn,errmsg,...) { HRESULT hr = fn; if(FAILED(hr)) { return rn; } }
#define LOGFAILURERET(fn,rn,...) { _lasterr = (fn); if(FAILED(_lasterr)) { PSLOGV(1,__VA_ARGS__); return rn; } }
#define LOGFAILURE(fn,...) LOGFAILURERET(fn,LOGEMPTY,__VA_ARGS__)
#define LOGFAILURERETNULL(fn,...) LOGFAILURERET(fn,0,__VA_ARGS__)

// Constructors
psDirectX10::psDirectX10(const psVeciu& dim, unsigned antialias, bool vsync, bool fullscreen, bool destalpha, HWND hwnd) : psDriver(dim), _device(0), _vsync(vsync), _lasterr(0), _backbuffer(0)
{
#ifdef BSS_DEBUG
  const UINT DEVICEFLAGS = D3D10_CREATE_DEVICE_SINGLETHREADED|D3D10_CREATE_DEVICE_DEBUG;
#else
  const UINT DEVICEFLAGS = D3D10_CREATE_DEVICE_SINGLETHREADED;
#endif

  LOGFAILURE(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&_factory)), "CreateDXGIFactory failed with error: ", _lasterr)

  cDynArray<cArraySimple<IDXGIAdapter*, unsigned char>> _adapters;
  IDXGIAdapter* adapter = NULL;
  for(unsigned char i=0; _factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    _adapters.Add(adapter);

  if(!_adapters.Length())
  {
    PSLOGV(0, "No adapters to attach to!");
    return;
  }
  adapter=_adapters[0];

  cDynArray<cArraySimple<IDXGIOutput*, unsigned char>> _outputs;
  IDXGIOutput* output = NULL;
  for(unsigned char i=0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
    _outputs.Add(output);

  if(!_adapters.Length())
  {
    PSLOGV(0, "No outputs to attach to!");
    return;
  }
  output=_outputs[0];

  LOGFAILURE(D3D10CreateDevice(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, DEVICEFLAGS, D3D10_SDK_VERSION, &_device), "D3D10CreateDevice failed with error: ", _lasterr);
  DXGI_MODE_DESC target ={
    dim.x,
    dim.y,
    { 1, 60 },
    destalpha?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_B8G8R8X8_UNORM,
    //DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
    DXGI_MODE_SCALING_UNSPECIFIED
  };
  DXGI_MODE_DESC match={ 0 };
  if(FAILED(output->FindClosestMatchingMode(&target, &match, _device)))
  {
    PSLOGV(2, "FindClosestMatchingMode failed with error ", _lasterr, ", using target instead");
    memcpy(&match, &target, sizeof(DXGI_MODE_DESC));
  }
  DXGI_SWAP_CHAIN_DESC swapdesc ={
    match,
    { antialias+1, 0 },
    DXGI_USAGE_RENDER_TARGET_OUTPUT,
    2,
    hwnd,
    !fullscreen,
    DXGI_SWAP_EFFECT_DISCARD,
    0
  };
  LOGFAILURE(_factory->CreateSwapChain(_device, &swapdesc, &_swapchain), "CreateSwapChain failed with error: ", _lasterr);

  ID3D10Texture2D* backbuffer = 0;
  LOGFAILURE(_swapchain->GetBuffer(0, __uuidof(*backbuffer), (LPVOID*)&backbuffer));

  D3D10_TEXTURE2D_DESC backbuffer_desc;
  backbuffer->GetDesc(&backbuffer_desc);
  _backbuffer = new psTex(_createshaderview(backbuffer),
    _creatertview(backbuffer),
    psVeciu(backbuffer_desc.Width, backbuffer_desc.Height),
    _reversefmt(backbuffer_desc.Format),
    (USAGETYPES)_reverseusage(backbuffer_desc.Usage, backbuffer_desc.MiscFlags, backbuffer_desc.BindFlags),
    backbuffer_desc.MipLevels);
  backbuffer->Release();

  SetRenderTargets(&_backbuffer, 1);
  screendim = _backbuffer->GetDim();
  ApplyCamera(VEC3D_ZERO, VEC_ZERO, 0, psRectiu(0, 0, screendim.x, screendim.y));

  _fsquadVS = (ID3D10VertexShader*)CreateShader(fsquadVS_main, sizeof(fsquadVS_main), VERTEX_SHADER_4_0);
  
  
  
  Clear(0xFF000000);
  _swapchain->Present(0, 0); // Do this so DXGI_PRESENT_DO_NOT_SEQUENCE doesn't blow things up
}
psDirectX10::~psDirectX10()
{
  if(_backbuffer)
    delete _backbuffer;
  int r;
  if(_device)
    r=_device->Release();
  if(_factory)
    r=_factory->Release();
}
bool psDirectX10::Begin() { return true; }
char psDirectX10::End()
{
  HRESULT hr = _swapchain->Present(_vsync, 0); // DXGI_PRESENT_DO_NOT_SEQUENCE doesn't seem to do anything other than break everything.
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
void BSS_FASTCALL psDirectX10::Draw(psVertObj* buf, FLAG_TYPE flags, const float(&transform)[4][4])
{ 
  unsigned int offset=0;
  _device->IASetVertexBuffers(0, 1, (ID3D10Buffer**)&buf->verts, &buf->vsize, &offset);
  //_device->IASetInputLayout((ID3D10InputLayout*)buf->layout);
  _device->IASetPrimitiveTopology(_getdx10topology(buf->mode));
  
  if(!buf->indices)
    _device->Draw(buf->nvert, 0);
  else
  {
    _device->IASetIndexBuffer((ID3D10Buffer*)buf->indices, DXGI_FORMAT_R16_UINT, 0);
    _device->DrawIndexed(buf->nindice, 0, 0);
  }
}
void BSS_FASTCALL psDirectX10::DrawRect(const psRectRotateZ rect, const psRect& uv, unsigned int color, const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags)
{ // Because we have to send the rect position in SOMEHOW and DX10 forces us to send matrices through the shaders, we will lock a buffer no matter what we do. 
  DrawRectBatchBegin(texes, numtex, 1, flags); // So it's easy and just as fast to "batch render" a single rect instead of try and use an existing vertex buffer
  DrawRectBatch(rect, uv, color, flags);
  DrawRectBatchEnd();
}
void BSS_FASTCALL psDirectX10::DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned int numrects, FLAG_TYPE flags, const float (&xform)[4][4])
{ 

  
}
void BSS_FASTCALL psDirectX10::DrawRectBatch(const psRectRotateZ rect, const psRect& uv, unsigned int color, FLAG_TYPE flags) { }
void psDirectX10::DrawRectBatchEnd() { }
void BSS_FASTCALL psDirectX10::DrawCircle() { }
void BSS_FASTCALL psDirectX10::DrawPolygon(const psVec* verts, FNUM Z, int num, unsigned long vertexcolor, FLAG_TYPE flags) { }
void BSS_FASTCALL psDirectX10::DrawPointsBegin(const psTex* texture, float size, FLAG_TYPE flags) { }
void BSS_FASTCALL psDirectX10::DrawPoints(psVertex* particles, unsigned int num) { }
void psDirectX10::DrawPointsEnd() { }
void BSS_FASTCALL psDirectX10::DrawLinesStart(int num, FLAG_TYPE flags)
{

}
void BSS_FASTCALL psDirectX10::DrawLines(const cLineT<float>& line, float Z1, float Z2, unsigned long vertexcolor, FLAG_TYPE flags) { }
void psDirectX10::DrawLinesEnd() { }
void BSS_FASTCALL psDirectX10::ApplyCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport) 
{ 
  D3D10_VIEWPORT vp ={ viewport.left, viewport.top, viewport.right, viewport.bottom, 0.0f, 1.0f };
  _device->RSSetViewports(1, &vp);

  D3DXMatrixPerspectiveLH(&matProj, (FLOAT)vp.Width, (FLOAT)vp.Height, 1.0f/_extent.y, _extent.x);
  matProj._22=-matProj._22;

  D3DXMATRIX m;
  D3DXMatrixTranslation(&m, vp.Width*-0.5f*_extent.y, vp.Height*-0.5f*_extent.y, 0);
  D3DXMatrixMultiply(&matProj, &m, &matProj);
  D3DXMatrixScaling(&matScaleZ, _extent.y, _extent.y, 1.0f);

  matCamPos_NoScale._41=-pos.x-pivot.x; //This is faster then rebuilding the entire matrix every time for no reason
  matCamPos_NoScale._42=-pos.y-pivot.y;

  D3DXMATRIX matMid;
  D3DXMATRIX matCamRotate_NoScale;
  D3DXMatrixTranslation(&matMid, -pos.x, -pos.y, 0.0f);
  D3DXMatrixRotationZ(&matCamRotate, -rotation); // This has to be reversed because of coordinate system weirdness
  D3DXMatrixMultiply(&matCamRotate_NoScale, &matMid, &matCamRotate); // Mc * Mr
  _inversetransform(matMid.m);
  D3DXMatrixMultiply(&matCamRotate_NoScale, &matCamRotate_NoScale, &matMid); // * Mc inverse
  _zerocamrot=bss_util::fsmall(rotation);

  D3DXMatrixMultiply(&matCamRotPos, &matCamRotate_NoScale, &matCamPos_NoScale); //This builds the double
  //D3DXMatrixMultiply(&matCamRotPos, &matCamPos_NoScale, &matCamRotate_NoScale); //This builds the double
  D3DXMatrixMultiply(&matCamRotPos, &matCamRotPos, &matScaleZ);
  D3DXMatrixMultiply(&matCamRotate, &matCamRotate_NoScale, &matScaleZ); // and apply the universal scaling (We have to do this after the double matrix is calculated)

  D3DXMatrixMultiply(&matCamPos, &matCamPos_NoScale, &matScaleZ); // and apply the universal scaling (We have to do this after the double matrix is calculated)
}
void BSS_FASTCALL psDirectX10::ApplyCamera3D(float(&m)[4][4], const psRectiu& viewport) 
{
  D3D10_VIEWPORT vp ={ viewport.left, viewport.top, viewport.right, viewport.bottom, 0.0f, 1.0f };
  _device->RSSetViewports(1, &vp);

  memcpy_s(&matProj, sizeof(float)*16, m, sizeof(float)*16);

  D3DXMatrixIdentity(&matCamPos_NoScale);
  D3DXMatrixIdentity(&matCamRotate);
  D3DXMatrixIdentity(&matCamRotPos);
  D3DXMatrixIdentity(&matCamPos);
}
void psDirectX10::DrawFullScreenQuad()
{ 
  _device->VSSetShader(_fsquadVS);
  _device->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _device->Draw(3, 0);
}
void BSS_FASTCALL psDirectX10::SetExtent(float znear, float zfar) { _extent.x=zfar; _extent.y=znear+1.0f; }
void* BSS_FASTCALL psDirectX10::CreateBuffer(unsigned short bytes, unsigned int usage, const void* initdata)
{
  D3D10_BUFFER_DESC desc ={
    bytes,
    (D3D10_USAGE)_usagetodxtype(usage),
    _usagetobind(usage),
    _usagetocpuflag(usage),
    _usagetomisc(usage),
  };

  D3D10_SUBRESOURCE_DATA subdata ={ initdata, 0, 0 };
  
  ID3D10Buffer* buf;
  LOGFAILURERETNULL(_device->CreateBuffer(&desc, !initdata?0:&subdata, &buf), "CreateBuffer failed");
  return buf;
}
void* BSS_FASTCALL psDirectX10::LockBuffer(void* target, unsigned int flags)
{
  void* r;
  LOGFAILURERETNULL(((ID3D10Buffer*)target)->Map((D3D10_MAP)(flags&LOCK_TYPEMASK), (flags&LOCK_DONOTWAIT)?D3D10_MAP_FLAG_DO_NOT_WAIT:0, &r),"LockBuffer failed")
  return r;
}
void BSS_FASTCALL psDirectX10::UnlockBuffer(void* target)
{
  ((ID3D10Buffer*)target)->Unmap();
}
void* BSS_FASTCALL psDirectX10::CreateTexture(psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, const void* initdata, void** additionalview)
{
  ID3D10Texture2D* tex = 0;
  D3D10_TEXTURE2D_DESC desc ={ dim.x, dim.y, miplevels, 1, (DXGI_FORMAT)_fmttodx10(format), { 0, 0 }, (D3D10_USAGE)_usagetodxtype(usage), _usagetobind(usage), _usagetocpuflag(usage), _usagetomisc(usage) };
  D3D10_SUBRESOURCE_DATA subdata ={ initdata, (((_getbitsperpixel(format)*dim.x)+7)<<3), 0 }; // The +7 here is to force the per-line bits to correctly round up the number of bytes.

  LOGFAILURERETNULL(_device->CreateTexture2D(&desc, !initdata?0:&subdata, &tex), "CreateTexture failed")
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE)!=0)?_createshaderview(tex):tex;
}
void* BSS_FASTCALL psDirectX10::LoadTexture(const char* path, unsigned int usage, FORMATS format, unsigned char miplevels, void** additionalview)
{ 
  D3DX10_IMAGE_INFO img;
  D3DX10_IMAGE_LOAD_INFO info;
  info.MipLevels=miplevels;
  info.Usage=(D3D10_USAGE)_usagetodxtype(usage);
  info.BindFlags=_usagetobind(usage);
  info.CpuAccessFlags=_usagetocpuflag(usage);
  info.MiscFlags=_usagetomisc(usage);
  info.Format=(DXGI_FORMAT)_fmttodx10(format);
  info.pSrcInfo=&img;

  ID3D10Resource* tex=0;
  LOGFAILURERETNULL(D3DX10CreateTextureFromFileW(_device, cStrW(path), &info, 0, &tex, 0), "LoadTexture failed for ", path);
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE)!=0)?_createshaderview(tex):tex;
}
void* BSS_FASTCALL psDirectX10::LoadTextureInMemory(const void* data, size_t datasize, unsigned int usage, FORMATS format, unsigned char miplevels, void** additionalview)
{
  D3DX10_IMAGE_INFO img ={};
  D3DX10_IMAGE_LOAD_INFO info;
  info.MipLevels=miplevels;
  info.Usage=(D3D10_USAGE)_usagetodxtype(usage);
  info.BindFlags=_usagetobind(usage);
  info.CpuAccessFlags=_usagetocpuflag(usage);
  info.MiscFlags=_usagetomisc(usage);
  info.Format=(DXGI_FORMAT)_fmttodx10(format);
  info.pSrcInfo=&img;

  ID3D10Resource* tex=0;
  LOGFAILURERETNULL(D3DX10CreateTextureFromMemory(_device, data, datasize, &info, 0, &tex, 0), "LoadTexture failed for ", data);
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE)!=0)?_createshaderview(tex):tex;
}
void BSS_FASTCALL psDirectX10::PushScissorRect(const psRectl& rect) { _scissorstack.Push(rect); _device->RSSetScissorRects(1, (D3D10_RECT*)&rect); }
void psDirectX10::PopScissorRect() { _scissorstack.Discard(); _device->RSSetScissorRects(1, (D3D10_RECT*)&_scissorstack.Peek()); }
void BSS_FASTCALL psDirectX10::SetRenderTargets(const psTex* const* texes, unsigned char num, const psTex* depthstencil)
{ 
  DYNARRAY(ID3D10RenderTargetView*, views, num);
  for(unsigned char i = 0; i < num; ++i) views[i] = (ID3D10RenderTargetView*)texes[i]->GetView();
  _device->OMSetRenderTargets(num, views, (ID3D10DepthStencilView*)(!depthstencil?0:depthstencil->GetView()));
}

void BSS_FASTCALL psDirectX10::SetShaderConstants(void* constbuf, SHADER_VER shader)
{
  if(shader<=VERTEX_SHADER_5_0)
    _device->VSSetConstantBuffers(1, 1, (ID3D10Buffer**)&constbuf);
  else if(shader<=PIXEL_SHADER_5_0)
    _device->PSSetConstantBuffers(0, 1, (ID3D10Buffer**)&constbuf);
  else if(shader<=GEOMETRY_SHADER_5_0)
    _device->GSSetConstantBuffers(1, 1, (ID3D10Buffer**)&constbuf);
}

void BSS_FASTCALL psDirectX10::SetTextures(const psTex* const* texes, unsigned char num, SHADER_VER shader)
{
  DYNARRAY(ID3D10ShaderResourceView*, views, num);
  for(unsigned char i = 0; i < num; ++i) views[i] = (ID3D10ShaderResourceView*)texes[i]->GetRes();
  if(shader<=VERTEX_SHADER_5_0)
    _device->VSSetShaderResources(0, num, views);
  else if(shader<=PIXEL_SHADER_5_0)
    _device->PSSetShaderResources(0, num, views);
  else if(shader<=GEOMETRY_SHADER_5_0)
    _device->GSSetShaderResources(0, num, views);
}
// Builds a stateblock from the given set of state changes
void* BSS_FASTCALL psDirectX10::CreateStateblock(const STATEINFO* states)
{
  // Build each type of stateblock and independently check for duplicates. Allocate array of sampler pointers at end
  DX10_SB* sb = 0;
  D3D10_BLEND_DESC bs ={ 0, { 0 }, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, { D3D10_COLOR_WRITE_ENABLE_ALL } };
  D3D10_DEPTH_STENCIL_DESC ds ={ 1, D3D10_DEPTH_WRITE_MASK_ALL, D3D10_COMPARISON_LESS, 0, D3D10_DEFAULT_STENCIL_READ_MASK, D3D10_DEFAULT_STENCIL_WRITE_MASK,
    { D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_COMPARISON_ALWAYS },
    { D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_COMPARISON_ALWAYS } };
  D3D10_RASTERIZER_DESC rs ={ D3D10_FILL_SOLID, D3D10_CULL_BACK, 0, 0, 0.0f, 0.0f, 1, 1, 0, 1 };
  D3D10_SAMPLER_DESC ss ={ D3D10_FILTER_MIN_MAG_MIP_POINT, D3D10_TEXTURE_ADDRESS_CLAMP, D3D10_TEXTURE_ADDRESS_CLAMP, D3D10_TEXTURE_ADDRESS_CLAMP, 0.0f, 16, D3D10_COMPARISON_NEVER, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, FLT_MAX };
  
  const STATEINFO* cur=states;
  if(cur->type == TYPE_BLEND_ALPHATOCOVERAGEENABLE) bs.AlphaToCoverageEnable = (cur++)->value;
  while(cur->type == TYPE_BLEND_BLENDENABLE) bs.BlendEnable[cur->index] = (cur++)->value;
  if(cur->type == TYPE_BLEND_SRCBLEND) bs.SrcBlend = (D3D10_BLEND)(cur++)->value;
  if(cur->type == TYPE_BLEND_DESTBLEND) bs.DestBlend = (D3D10_BLEND)(cur++)->value;
  if(cur->type == TYPE_BLEND_BLENDOP) bs.BlendOp = (D3D10_BLEND_OP)(cur++)->value;
  if(cur->type == TYPE_BLEND_SRCBLENDALPHA) bs.SrcBlendAlpha = (D3D10_BLEND)(cur++)->value;
  if(cur->type == TYPE_BLEND_DESTBLENDALPHA) bs.DestBlendAlpha = (D3D10_BLEND)(cur++)->value;
  if(cur->type == TYPE_BLEND_BLENDOPALPHA) bs.BlendOpAlpha = (D3D10_BLEND_OP)(cur++)->value;
  while(cur->type == TYPE_BLEND_RENDERTARGETWRITEMASK) bs.RenderTargetWriteMask[cur->index] = (cur++)->value;

  if(cur->type == TYPE_DEPTH_DEPTHENABLE) ds.DepthEnable = (cur++)->value;
  if(cur->type == TYPE_DEPTH_DEPTHWRITEMASK) ds.DepthWriteMask = (D3D10_DEPTH_WRITE_MASK)(cur++)->value;
  if(cur->type == TYPE_DEPTH_DEPTHFUNC) ds.DepthFunc = (D3D10_COMPARISON_FUNC)(cur++)->value;
  if(cur->type == TYPE_DEPTH_STENCILENABLE) ds.StencilEnable = (cur++)->value;
  if(cur->type == TYPE_DEPTH_STENCILREADMASK) ds.StencilReadMask = (cur++)->value;
  if(cur->type == TYPE_DEPTH_STENCILWRITEMASK) ds.StencilWriteMask = (cur++)->value;
  if(cur->type == TYPE_DEPTH_FRONT_STENCILFAILOP) ds.FrontFace.StencilFailOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_FRONT_STENCILDEPTHFAILOP) ds.FrontFace.StencilDepthFailOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_FRONT_STENCILPASSOP) ds.FrontFace.StencilPassOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_FRONT_STENCILFUNC) ds.FrontFace.StencilFunc = (D3D10_COMPARISON_FUNC)(cur++)->value;
  if(cur->type == TYPE_DEPTH_BACK_STENCILFAILOP) ds.BackFace.StencilFailOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_BACK_STENCILDEPTHFAILOP) ds.BackFace.StencilDepthFailOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_BACK_STENCILPASSOP) ds.BackFace.StencilPassOp = (D3D10_STENCIL_OP)(cur++)->value;
  if(cur->type == TYPE_DEPTH_BACK_STENCILFUNC) ds.BackFace.StencilFunc = (D3D10_COMPARISON_FUNC)(cur++)->value;

  if(cur->type == TYPE_RASTER_FILLMODE) rs.FillMode = (D3D10_FILL_MODE)(cur++)->value;
  if(cur->type == TYPE_RASTER_CULLMODE) rs.CullMode = (D3D10_CULL_MODE)(cur++)->value;
  if(cur->type == TYPE_RASTER_FRONTCOUNTERCLOCKWISE) rs.FrontCounterClockwise = (cur++)->value;
  if(cur->type == TYPE_RASTER_DEPTHBIAS) rs.DepthBias = (cur++)->value;
  if(cur->type == TYPE_RASTER_DEPTHBIASCLAMP) rs.DepthBiasClamp = (cur++)->valuef;
  if(cur->type == TYPE_RASTER_SLOPESCALEDDEPTHBIAS) rs.SlopeScaledDepthBias = (cur++)->valuef;
  if(cur->type == TYPE_RASTER_DEPTHCLIPENABLE) rs.DepthClipEnable = (cur++)->value;
  if(cur->type == TYPE_RASTER_SCISSORENABLE) rs.ScissorEnable = (cur++)->value;
  if(cur->type == TYPE_RASTER_MULTISAMPLEENABLE) rs.MultisampleEnable = (cur++)->value;
  if(cur->type == TYPE_RASTER_ANTIALIASEDLINEENABLE) rs.AntialiasedLineEnable = (cur++)->value;

  states=cur;
  unsigned char numvs=0;
  unsigned char numps=0;
  unsigned char numgs=0;
  while(cur)
  {
    if(cur->index<psStateblock::MAXSAMPLERS && cur->index>numps) numps=cur->index;
    if(cur->index<(psStateblock::MAXSAMPLERS*2) && cur->index>numvs) numvs=cur->index;
    if(cur->index<(psStateblock::MAXSAMPLERS*3) && cur->index>numgs) numgs=cur->index;
    ++cur;
  }
  cur=states;
  sb = (DX10_SB*)malloc(sizeof(DX10_SB)+(numvs+numps+numgs)*sizeof(D3D10_SAMPLER_DESC));
  sb->numsamplervs=numvs;
  sb->numsamplerps=numps;
  sb->numsamplergs=numgs;
  unsigned char total = numvs+numps+numgs;
  D3D10_SAMPLER_DESC curss;
  for(unsigned char i = 0; i < total; ++i)
  {
    MEMCPY(&curss, sizeof(D3D10_SAMPLER_DESC), &ss, sizeof(D3D10_SAMPLER_DESC));
    unsigned char index = i + (psStateblock::MAXSAMPLERS*((i>numps)+(i>numvs)+(i>numgs)));
    if(cur->type == TYPE_SAMPLER_FILTER && cur->index==index) ss.Filter = (D3D10_FILTER)(cur++)->value;
    if(cur->type == TYPE_SAMPLER_ADDRESSU && cur->index==index) ss.AddressU = (D3D10_TEXTURE_ADDRESS_MODE)(cur++)->value;
    if(cur->type == TYPE_SAMPLER_ADDRESSV && cur->index==index) ss.AddressV = (D3D10_TEXTURE_ADDRESS_MODE)(cur++)->value;
    if(cur->type == TYPE_SAMPLER_ADDRESSW && cur->index==index) ss.AddressW = (D3D10_TEXTURE_ADDRESS_MODE)(cur++)->value;
    if(cur->type == TYPE_SAMPLER_MIPLODBIAS && cur->index==index) ss.MipLODBias = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_MAXANISOTROPY && cur->index==index) ss.MaxAnisotropy = (cur++)->value;
    if(cur->type == TYPE_SAMPLER_COMPARISONFUNC && cur->index==index) ss.ComparisonFunc = (D3D10_COMPARISON_FUNC)(cur++)->value;
    if(cur->type == TYPE_SAMPLER_BORDERCOLOR1 && cur->index==index) ss.BorderColor[0] = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_BORDERCOLOR2 && cur->index==index) ss.BorderColor[1] = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_BORDERCOLOR3 && cur->index==index) ss.BorderColor[2] = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_BORDERCOLOR4 && cur->index==index) ss.BorderColor[3] = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_MINLOD && cur->index==index) ss.MinLOD = (cur++)->valuef;
    if(cur->type == TYPE_SAMPLER_MAXLOD && cur->index==index) ss.MaxLOD = (cur++)->valuef;
    LOGFAILURERETNULL(_device->CreateSamplerState(&curss, ((ID3D10SamplerState**)sb->GetPS(0))+i));
  }

  LOGFAILURERETNULL(_device->CreateBlendState(&bs, &sb->bs), "CreateBlendState failed")
  LOGFAILURERETNULL(_device->CreateDepthStencilState(&ds, &sb->ds), "CreateDepthStencilState failed")
  LOGFAILURERETNULL(_device->CreateRasterizerState(&rs, &sb->rs), "CreateRasterizerState failed")

  return sb;
}
// Sets a given stateblock
void BSS_FASTCALL psDirectX10::SetStateblock(void* stateblock)
{
  DX10_SB* sb = (DX10_SB*)stateblock;
  _device->RSSetState(sb->rs);
  _device->OMSetDepthStencilState(sb->ds, sb->stencilref);
  _device->OMSetBlendState(sb->bs, sb->blendfactor, sb->sampleMask);
  _device->PSSetSamplers(0, sb->numsamplerps, DX10_SB::GetPS(sb));
  _device->VSSetSamplers(0, sb->numsamplervs, DX10_SB::GetVS(sb));
  _device->GSSetSamplers(0, sb->numsamplergs, DX10_SB::GetGS(sb));
}
void* BSS_FASTCALL psDirectX10::CreateLayout(void* shader, const ELEMENT_DESC* elements, unsigned char num)
{
  ID3D10Blob* blob = (ID3D10Blob*)shader;
  return CreateLayout(blob->GetBufferPointer(), blob->GetBufferSize(), elements, num);
}
void* BSS_FASTCALL psDirectX10::CreateLayout(void* shader, size_t sz, const ELEMENT_DESC* elements, unsigned char num)
{
  DYNARRAY(D3D10_INPUT_ELEMENT_DESC, descs, num);

  for(unsigned char i = 0; i < num; ++i)
  {
    switch(elements[i].semantic)
    {
    case ELEMENT_BINORMAL: descs[i].SemanticName="BINORMAL"; break;
    case ELEMENT_BLENDINDICES: descs[i].SemanticName="BLENDINDICES"; break;
    case ELEMENT_BLENDWEIGHT: descs[i].SemanticName="BLENDWEIGHT"; break;
    case ELEMENT_COLOR: descs[i].SemanticName="COLOR"; break;
    case ELEMENT_NORMAL: descs[i].SemanticName="NORMAL"; break;
    case ELEMENT_POSITION: descs[i].SemanticName="POSITION"; break;
    case ELEMENT_POSITIONT: descs[i].SemanticName="POSITIONT"; break;
    case ELEMENT_PSIZE: descs[i].SemanticName="PSIZE"; break;
    case ELEMENT_TANGENT: descs[i].SemanticName="TANGENT"; break;
    case ELEMENT_TEXCOORD: descs[i].SemanticName="TEXCOORD"; break;
    }
    descs[i].SemanticIndex = elements[i].semanticIndex;
    descs[i].Format = (DXGI_FORMAT)_fmttodx10(elements[i].format);
    descs[i].AlignedByteOffset = elements[i].byteOffset;
    descs[i].InputSlot = elements[i].IAslot;
    descs[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
    descs[i].InstanceDataStepRate = 0;
  }

  ID3D10InputLayout* r;
  LOGFAILURERETNULL(_device->CreateInputLayout(descs, num, shader, sz, &r), "CreateInputLayout failed")
  return r;
}
void BSS_FASTCALL psDirectX10::SetLayout(void* layout)
{
  _device->IASetInputLayout((ID3D10InputLayout*)layout);
}

void BSS_FASTCALL psDirectX10::FreeResource(void* p, RESOURCE_TYPE t)
{ 
  switch(t)
  {
  case RES_SHADERPS:
    ((ID3D10PixelShader*)p)->Release();
    break;
  case RES_SHADERVS:
    ((ID3D10VertexShader*)p)->Release();
    break;
  case RES_SHADERGS:
    ((ID3D10GeometryShader*)p)->Release();
    break;
  case RES_TEXTURE:
    ((ID3D10ShaderResourceView*)p)->Release();
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    ((ID3D10Buffer*)p)->Release();
    break;
  case RES_SURFACE:
    ((ID3D10RenderTargetView*)p)->Release();
    break;
  case RES_DEPTHVIEW:
    ((ID3D10DepthStencilView*)p)->Release();
    break;
  case RES_STATEBLOCK:
  {
    DX10_SB* sb = (DX10_SB*)p;
    sb->rs->Release();
    sb->ds->Release();
    sb->bs->Release();
    ID3D10SamplerState* const* cur=DX10_SB::GetPS(sb);
    for(unsigned char i = 0; i < sb->numsamplerps; ++i) cur[i]->Release();
    cur=DX10_SB::GetVS(sb);
    for(unsigned char i = 0; i < sb->numsamplervs; ++i) cur[i]->Release();
    cur=DX10_SB::GetGS(sb);
    for(unsigned char i = 0; i < sb->numsamplergs; ++i) cur[i]->Release();
  }
    break;
  case RES_LAYOUT:
    ((ID3D10InputLayout*)p)->Release();
    break;
  }
}
void BSS_FASTCALL psDirectX10::GrabResource(void* p, RESOURCE_TYPE t)
{
  switch(t)
  {
  case RES_SHADERPS:
    ((ID3D10PixelShader*)p)->AddRef();
    break;
  case RES_SHADERVS:
    ((ID3D10VertexShader*)p)->AddRef();
    break;
  case RES_SHADERGS:
    ((ID3D10GeometryShader*)p)->AddRef();
    break;
  case RES_TEXTURE:
    ((ID3D10ShaderResourceView*)p)->AddRef();
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    ((ID3D10Buffer*)p)->AddRef();
    break;
  case RES_SURFACE:
    ((ID3D10RenderTargetView*)p)->AddRef();
    break;
  case RES_DEPTHVIEW:
    ((ID3D10DepthStencilView*)p)->AddRef();
    break;
  case RES_LAYOUT:
    ((ID3D10InputLayout*)p)->AddRef();
    break;
  }
}
void BSS_FASTCALL psDirectX10::CopyResource(void* dest, void* src, RESOURCE_TYPE t)
{
  ID3D10Resource* rsrc=0;
  ID3D10Resource* rdest=0;
  switch(t)
  {
  case RES_TEXTURE:
    ((ID3D10ShaderResourceView*)src)->GetResource(&rsrc);
    ((ID3D10ShaderResourceView*)dest)->GetResource(&rdest);
    break;
  case RES_SURFACE:
    ((ID3D10RenderTargetView*)src)->GetResource(&rsrc);
    ((ID3D10RenderTargetView*)dest)->GetResource(&rdest);
    break;
  case RES_DEPTHVIEW:
    ((ID3D10DepthStencilView*)src)->GetResource(&rsrc);
    ((ID3D10DepthStencilView*)dest)->GetResource(&rdest);
    break;
  case RES_INDEXBUF:
  case RES_VERTEXBUF:
  case RES_CONSTBUF:
    rsrc = ((ID3D10Buffer*)src);
    rdest = ((ID3D10Buffer*)dest);
    break;
  }
  assert(rsrc!=0 && rdest!=0);
  _device->CopyResource(rdest, rsrc);
}

void BSS_FASTCALL psDirectX10::Resize(psVeciu dim, FORMATS format, bool fullscreen)
{
  _device->OMSetRenderTargets(0, 0, 0);
  _lasterr = _swapchain->ResizeBuffers(2, dim.x, dim.y, (DXGI_FORMAT)_fmttodx10(format), DXGI_SWAP_CHAIN_FLAG_NONPREROTATED|DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
}

void BSS_FASTCALL psDirectX10::Clear(unsigned int color)
{
  psColor colors = psColor(color).rgba();
  ID3D10RenderTargetView* views[8];
  ID3D10DepthStencilView* dview;
  _device->OMGetRenderTargets(8, views, &dview);
  if(dview)
  {
    _device->ClearDepthStencilView(dview, D3D10_CLEAR_DEPTH|D3D10_CLEAR_STENCIL, 1.0f, 0);
    dview->Release(); // GetRenderTargets calls Get on everything
  }
  for(unsigned char i = 0; i < 8; ++i)
    if(views[i])
    {
      _device->ClearRenderTargetView(views[i], colors);
      views[i]->Release(); // GetRenderTargets calls Get on everything
    }
}

const char* shader_profiles[] ={ "vs_1_1", "vs_2_0", "vs_2_a", "vs_3_0", "vs_4_0", "vs_4_1", "vs_5_0", "ps_1_1", "ps_1_2", "ps_1_3", "ps_1_4", "ps_2_0",
"ps_2_a", "ps_2_b", "ps_3_0", "ps_4_0", "ps_4_1", "ps_5_0", "gs_4_0", "gs_4_1", "gs_5_0", "cs_4_0", "cs_4_1",
"cs_5_0", "ds_5_0", "hs_5_0" };

void* BSS_FASTCALL psDirectX10::CompileShader(const char* source, SHADER_VER profile, const char* entrypoint)
{
  ID3D10Blob* ret=0;
  ID3D10Blob* err=0;
  if(FAILED(_lasterr=D3D10CompileShader(source, strlen(source), 0, 0, 0, entrypoint, shader_profiles[profile], 0, &ret, &err)))
  {
    if(!err) PSLOG(2) << "The effect could not be compiled! ERR: " << _lasterr << " Source: \n" << source << std::endl;
    else if(_lasterr==0x8007007e) PSLOG(2) << "The effect cannot be loaded because a module cannot be found (?) Source: \n" << source << std::endl;
    else PSLOG(2) << "The effect failed to compile (Errors: " << (const char*)err->GetBufferPointer() << ") Source: \n" << source << std::endl;
    return 0;
  }
  return ret;
}
void* BSS_FASTCALL psDirectX10::CreateShader(const void* data, SHADER_VER profile)
{
  ID3D10Blob* blob = (ID3D10Blob*)data;
  return CreateShader(blob->GetBufferPointer(), blob->GetBufferSize(), profile);
}
void* BSS_FASTCALL psDirectX10::CreateShader(const void* data, size_t datasize, SHADER_VER profile)
{
  void* p;

  if(profile<=VERTEX_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreateVertexShader(data, datasize, (ID3D10VertexShader**)&p), "CreateVertexShader failed")
  else if(profile<=PIXEL_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreatePixelShader(data, datasize, (ID3D10PixelShader**)&p), "CreatePixelShader failed")
  else if(profile<=GEOMETRY_SHADER_5_0)
    LOGFAILURERETNULL(_device->CreateGeometryShader(data, datasize, (ID3D10GeometryShader**)&p), "CreateGeometryShader failed")
  //else if(profile<=COMPUTE_SHADER_5_0)
  //  _device->Create((DWORD*)blob->GetBufferPointer(), blob->GetBufferSize(), (ID3D10PixelShader**)&p);
  else
    return 0;
  return p;
}
char BSS_FASTCALL psDirectX10::SetShader(void* shader, SHADER_VER profile)
{
  if(profile<=VERTEX_SHADER_5_0)
    _device->VSSetShader((ID3D10VertexShader*)shader);
  else if(profile<=PIXEL_SHADER_5_0)
    _device->PSSetShader((ID3D10PixelShader*)shader);
  else if(profile<=GEOMETRY_SHADER_5_0)
    _device->GSSetShader((ID3D10GeometryShader*)shader);
  else
    return -1;
  return 0;
}
bool BSS_FASTCALL psDirectX10::ShaderSupported(SHADER_VER profile) //With DX10 shader support is known at compile time.
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

unsigned int BSS_FASTCALL psDirectX10::_usagetodxtype(unsigned int types)
{
  switch(types&USAGE_USAGEMASK)
  {
  case USAGE_DEFAULT: return D3D10_USAGE_DEFAULT;
  case USAGE_IMMUTABLE: return D3D10_USAGE_IMMUTABLE;
  case USAGE_DYNAMIC: return D3D10_USAGE_DYNAMIC;
  case USAGE_STAGING: return D3D10_USAGE_STAGING;
  }
  return -1;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetocpuflag(unsigned int types)
{
  switch(types&USAGE_USAGEMASK)
  {
  case USAGE_DEFAULT: return 0;
  case USAGE_IMMUTABLE: return 0;
  case USAGE_DYNAMIC: return D3D10_CPU_ACCESS_WRITE;
  case USAGE_STAGING: return D3D10_CPU_ACCESS_WRITE|D3D10_CPU_ACCESS_READ;
  }
  return 0;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetomisc(unsigned int types)
{
  unsigned int r=0;
  if(types&USAGE_AUTOGENMIPMAP) r |= D3D10_RESOURCE_MISC_GENERATE_MIPS;
  if(types&USAGE_TEXTURECUBE) r |= D3D10_RESOURCE_MISC_TEXTURECUBE;
  return r;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetobind(unsigned int types)
{
  unsigned int r=0;
  switch(types&USAGE_BINDMASK)
  {
  case USAGE_VERTEX: r = D3D10_BIND_VERTEX_BUFFER; break;
  case USAGE_INDEX: r = D3D10_BIND_INDEX_BUFFER; break;
  case USAGE_CONSTANT_BUFFER: r = D3D10_BIND_CONSTANT_BUFFER; break;
  }
  if(types&USAGE_RENDERTARGET) r |= D3D10_BIND_RENDER_TARGET;
  if(types&USAGE_SHADER_RESOURCE) r |= D3D10_BIND_SHADER_RESOURCE;
  if(types&USAGE_DEPTH_STENCIL) r |= D3D10_BIND_DEPTH_STENCIL;
  return r;
}
unsigned int BSS_FASTCALL psDirectX10::_reverseusage(unsigned int usage, unsigned int misc, unsigned int bind)
{
  unsigned int r = 0;
  switch(usage)
  {
  case D3D10_USAGE_DEFAULT: r = USAGE_DEFAULT;
  case D3D10_USAGE_IMMUTABLE: r = USAGE_IMMUTABLE;
  case D3D10_USAGE_DYNAMIC: r = USAGE_DYNAMIC;
  case D3D10_USAGE_STAGING: r = USAGE_STAGING;
  }
  if(misc&D3D10_RESOURCE_MISC_GENERATE_MIPS) r |= USAGE_AUTOGENMIPMAP;
  if(misc&D3D10_RESOURCE_MISC_TEXTURECUBE) r |= USAGE_TEXTURECUBE;
  switch(bind)
  {
  case D3D10_BIND_VERTEX_BUFFER: r |= USAGE_VERTEX; break;
  case D3D10_BIND_INDEX_BUFFER: r |= USAGE_INDEX; break;
  case D3D10_BIND_CONSTANT_BUFFER: r |= USAGE_CONSTANT_BUFFER; break;
  }
  if(bind&D3D10_BIND_RENDER_TARGET) r |= USAGE_RENDERTARGET;
  if(bind&D3D10_BIND_SHADER_RESOURCE) r |= USAGE_SHADER_RESOURCE;
  if(bind&D3D10_BIND_DEPTH_STENCIL) r |= USAGE_DEPTH_STENCIL;
  return r;
}

unsigned int BSS_FASTCALL psDirectX10::_fmttodx10(FORMATS format)
{
  switch(format)
  {
  case FMT_UNKNOWN: return DXGI_FORMAT_UNKNOWN;
  case FMT_A8R8G8B8: return DXGI_FORMAT_B8G8R8A8_UNORM;
  case FMT_X8R8G8B8: return DXGI_FORMAT_B8G8R8X8_UNORM;
  case FMT_R5G6B5: return DXGI_FORMAT_B5G6R5_UNORM;
  case FMT_A1R5G5B5: return DXGI_FORMAT_B5G5R5A1_UNORM;
  case FMT_A8: return DXGI_FORMAT_A8_UNORM;
  case FMT_A2B10G10R10: return DXGI_FORMAT_R10G10B10A2_UNORM;
  case FMT_A8B8G8R8: return DXGI_FORMAT_R8G8B8A8_UNORM;
  case FMT_G16R16: return DXGI_FORMAT_R16G16_UNORM;
  case FMT_A16B16G16R16: return DXGI_FORMAT_R16G16B16A16_UNORM;
  case FMT_V8U8: return DXGI_FORMAT_R8G8_SNORM;
  case FMT_Q8W8V8U8: return DXGI_FORMAT_R8G8B8A8_SNORM;
  case FMT_V16U16: return DXGI_FORMAT_R16G16_SNORM;
  case FMT_R8G8_B8G8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
  case FMT_G8R8_G8B8: return DXGI_FORMAT_R8G8_B8G8_UNORM;
  case FMT_DXT1: return DXGI_FORMAT_BC1_UNORM;
  case FMT_DXT2: return DXGI_FORMAT_BC1_UNORM;
  case FMT_DXT3: return DXGI_FORMAT_BC2_UNORM;
  case FMT_DXT4: return DXGI_FORMAT_BC2_UNORM;
  case FMT_DXT5: return DXGI_FORMAT_BC3_UNORM;
  case FMT_D16: return DXGI_FORMAT_D16_UNORM;
  case FMT_D32F: return DXGI_FORMAT_D32_FLOAT;
  case FMT_S8D24: return DXGI_FORMAT_D24_UNORM_S8_UINT;
  case FMT_INDEX16: return DXGI_FORMAT_R16_UINT;
  case FMT_INDEX32: return DXGI_FORMAT_R32_UINT;
  case FMT_Q16W16V16U16: return DXGI_FORMAT_R16G16B16A16_SNORM;
  case FMT_R16F: return DXGI_FORMAT_R16_FLOAT;
  case FMT_G16R16F: return DXGI_FORMAT_R16G16_FLOAT;
  case FMT_A16B16G16R16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case FMT_R32F: return DXGI_FORMAT_R32_FLOAT;
  case FMT_G32R32F: return DXGI_FORMAT_R32G32_FLOAT;
  case FMT_A32B32G32R32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case FMT_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
  case FMT_R16G16_SINT: return DXGI_FORMAT_R16G16_SINT;
  case FMT_FORMAT_R16G16B16A16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
  case FMT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
  case FMT_R16G16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
  case FMT_R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
  case FMT_R16G16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
  case FMT_R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
  case FMT_R16G16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
  case FMT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
  }
  return -1;
}
inline FORMATS BSS_FASTCALL psDirectX10::_reversefmt(unsigned int format)
{
  switch(format)
  {
  case DXGI_FORMAT_UNKNOWN: return FMT_UNKNOWN;
  case DXGI_FORMAT_B8G8R8A8_UNORM: return FMT_A8R8G8B8;
  case DXGI_FORMAT_B8G8R8X8_UNORM: return FMT_X8R8G8B8;
  case DXGI_FORMAT_B5G6R5_UNORM: return FMT_R5G6B5;
  case DXGI_FORMAT_B5G5R5A1_UNORM: return FMT_A1R5G5B5;
  case DXGI_FORMAT_A8_UNORM: return FMT_A8;
  case DXGI_FORMAT_R10G10B10A2_UNORM: return FMT_A2B10G10R10;
  //case DXGI_FORMAT_R8G8B8A8_UNORM: return FMT_A8B8G8R8;
  //case DXGI_FORMAT_R16G16_UNORM: return FMT_G16R16;
  //case DXGI_FORMAT_R16G16B16A16_UNORM: return FMT_A16B16G16R16;
  case DXGI_FORMAT_R8G8_SNORM: return FMT_V8U8;
  case DXGI_FORMAT_R8G8B8A8_SNORM: return FMT_Q8W8V8U8;
  //case DXGI_FORMAT_R16G16_SNORM: return FMT_V16U16;
  case DXGI_FORMAT_G8R8_G8B8_UNORM: return FMT_R8G8_B8G8;
  case DXGI_FORMAT_R8G8_B8G8_UNORM: return FMT_G8R8_G8B8;
  case DXGI_FORMAT_BC1_UNORM: return FMT_DXT1;
  case DXGI_FORMAT_BC2_UNORM: return FMT_DXT3;
  case DXGI_FORMAT_BC3_UNORM: return FMT_DXT5;
  case DXGI_FORMAT_D16_UNORM: return FMT_D16;
  case DXGI_FORMAT_D32_FLOAT: return FMT_D32F;
  case DXGI_FORMAT_D24_UNORM_S8_UINT: return FMT_S8D24;
  case DXGI_FORMAT_R16_UINT: return FMT_INDEX16;
  case DXGI_FORMAT_R32_UINT: return FMT_INDEX32;
  //case DXGI_FORMAT_R16G16B16A16_SNORM: return FMT_Q16W16V16U16;
  case DXGI_FORMAT_R16_FLOAT: return FMT_R16F;
  //case DXGI_FORMAT_R16G16_FLOAT: return FMT_G16R16F;
  //case DXGI_FORMAT_R16G16B16A16_FLOAT: return FMT_A16B16G16R16F;
  case DXGI_FORMAT_R32_FLOAT: return FMT_R32F;
  case DXGI_FORMAT_R32G32_FLOAT: return FMT_G32R32F;
  case DXGI_FORMAT_R32G32B32A32_FLOAT: return FMT_A32B32G32R32F;
  case DXGI_FORMAT_R8G8B8A8_UINT: return FMT_R8G8B8A8_UINT;
  case DXGI_FORMAT_R16G16_SINT: return FMT_R16G16_SINT;
  case DXGI_FORMAT_R16G16B16A16_SINT: return FMT_FORMAT_R16G16B16A16_SINT;
  case DXGI_FORMAT_R8G8B8A8_UNORM: return FMT_R8G8B8A8_UNORM;
  case DXGI_FORMAT_R16G16_SNORM: return FMT_R16G16_SNORM;
  case DXGI_FORMAT_R16G16B16A16_SNORM: return FMT_R16G16B16A16_SNORM;
  case DXGI_FORMAT_R16G16_UNORM: return FMT_R16G16_UNORM;
  case DXGI_FORMAT_R16G16B16A16_UNORM: return FMT_R16G16B16A16_UNORM;
  case DXGI_FORMAT_R16G16_FLOAT: return FMT_R16G16_FLOAT;
  case DXGI_FORMAT_R16G16B16A16_FLOAT: return FMT_R16G16B16A16_FLOAT;
  }
  return FMT_UNKNOWN;
}
unsigned int BSS_FASTCALL psDirectX10::_getbitsperpixel(FORMATS format)
{
  switch(_fmttodx10(format))
  {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_UINT:
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return 128;
  case DXGI_FORMAT_R32G32B32_TYPELESS:
  case DXGI_FORMAT_R32G32B32_FLOAT:
  case DXGI_FORMAT_R32G32B32_UINT:
  case DXGI_FORMAT_R32G32B32_SINT:
    return 96;
  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
  case DXGI_FORMAT_R16G16B16A16_UNORM:
  case DXGI_FORMAT_R16G16B16A16_UINT:
  case DXGI_FORMAT_R16G16B16A16_SNORM:
  case DXGI_FORMAT_R16G16B16A16_SINT:
  case DXGI_FORMAT_R32G32_TYPELESS:
  case DXGI_FORMAT_R32G32_FLOAT:
  case DXGI_FORMAT_R32G32_UINT:
  case DXGI_FORMAT_R32G32_SINT:
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return 64;
  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
  case DXGI_FORMAT_R10G10B10A2_UNORM:
  case DXGI_FORMAT_R10G10B10A2_UINT:
  case DXGI_FORMAT_R11G11B10_FLOAT:
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UINT:
  case DXGI_FORMAT_R8G8B8A8_SNORM:
  case DXGI_FORMAT_R8G8B8A8_SINT:
  case DXGI_FORMAT_R16G16_TYPELESS:
  case DXGI_FORMAT_R16G16_FLOAT:
  case DXGI_FORMAT_R16G16_UNORM:
  case DXGI_FORMAT_R16G16_UINT:
  case DXGI_FORMAT_R16G16_SNORM:
  case DXGI_FORMAT_R16G16_SINT:
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
  case DXGI_FORMAT_R32_UINT:
  case DXGI_FORMAT_R32_SINT:
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
  case DXGI_FORMAT_R8G8_B8G8_UNORM:
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return 32;
  case DXGI_FORMAT_R8G8_TYPELESS:
  case DXGI_FORMAT_R8G8_UNORM:
  case DXGI_FORMAT_R8G8_UINT:
  case DXGI_FORMAT_R8G8_SNORM:
  case DXGI_FORMAT_R8G8_SINT:
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_R16_FLOAT:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
  case DXGI_FORMAT_R16_UINT:
  case DXGI_FORMAT_R16_SNORM:
  case DXGI_FORMAT_R16_SINT:
  case DXGI_FORMAT_B5G6R5_UNORM:
  case DXGI_FORMAT_B5G5R5A1_UNORM:
#ifdef DXGI_1_2_FORMATS
  case DXGI_FORMAT_B4G4R4A4_UNORM:
#endif
    return 16;
  case DXGI_FORMAT_R8_TYPELESS:
  case DXGI_FORMAT_R8_UNORM:
  case DXGI_FORMAT_R8_UINT:
  case DXGI_FORMAT_R8_SNORM:
  case DXGI_FORMAT_R8_SINT:
  case DXGI_FORMAT_A8_UNORM:
    return 8;
  case DXGI_FORMAT_R1_UNORM:
    return 1;
  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
    return 4;
  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return 8;
  }
  return 4;
}
D3D10_PRIMITIVE_TOPOLOGY BSS_FASTCALL psDirectX10::_getdx10topology(PRIMITIVETYPE type)
{
  switch(type)
  {
  case POINTLIST: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case LINELIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  case LINESTRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
  case TRIANGLELIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case TRIANGLESTRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
  }
  return D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
void* psDirectX10::_createshaderview(ID3D10Resource* src)
{
  D3D10_SHADER_RESOURCE_VIEW_DESC desc;
  D3D10_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D10_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D10_TEXTURE2D_DESC desc2d;
    ((ID3D10Texture2D*)src)->GetDesc(&desc2d);

    desc.Format = desc2d.Format;
    desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = desc2d.MipLevels;
    desc.Texture2D.MostDetailedMip = desc2d.MipLevels -1;
  }
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
    break;
  }

  ID3D10ShaderResourceView* r = 0;
  if(FAILED(_device->CreateShaderResourceView(src, &desc, &r)))
  {
    PSLOG(1) << "_createshaderview failed" << std::endl;
    return 0;
  }
  return r;
}
void* psDirectX10::_creatertview(ID3D10Resource* src)
{
  /*D3D10_RENDER_TARGET_VIEW_DESC rtDesc;
  D3D10_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D10_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D10_TEXTURE2D_DESC desc2d;
    ((ID3D10Texture2D*)src)->GetDesc(&desc2d);

    rtDesc.Format = desc2d.Format;
    rtDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D.MipSlice = 0;
  }
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
    break;
  }*/

  ID3D10RenderTargetView* r = 0;
  if(FAILED(_device->CreateRenderTargetView(src, 0, &r)))
  {
    PSLOG(1) << "_createshaderview failed" << std::endl;
    return 0;
  }
  return r;
}
void* psDirectX10::_createdepthview(ID3D10Resource* src)
{
  D3D10_DEPTH_STENCIL_VIEW_DESC dDesc;
  D3D10_RESOURCE_DIMENSION type;
  src->GetType(&type);

  switch(type)
  {
  case D3D10_RESOURCE_DIMENSION_BUFFER:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D10_TEXTURE2D_DESC desc2d;
    ((ID3D10Texture2D*)src)->GetDesc(&desc2d);

    dDesc.Format = desc2d.Format;
    dDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    dDesc.Texture2D.MipSlice = 0;
  }
    break;
  case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
    break;
  }

  ID3D10DepthStencilView* r = 0;
  if(FAILED(_device->CreateDepthStencilView(src, &dDesc, &r)))
  {
    PSLOG(1) << "_createshaderview failed" << std::endl;
    return 0;
  }
  return r;
}