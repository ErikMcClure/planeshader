// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDirectX10.h"
#include "psDirectX10_fsquadVS.h"
#include "psEngine.h"
#include "psTex.h"

using namespace planeshader;

#ifdef BSS_CPU_x86_64
#pragma comment(lib, "../lib/dxlib/x64/d3d10.lib")
#pragma comment(lib, "../lib/dxlib/x64/dxguid.lib")
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
#ifdef BSS_DEBUG
#pragma comment(lib, "../lib/dxlib/d3dx10d.lib")
#else
#pragma comment(lib, "../lib/dxlib/d3dx10.lib")
#endif
#endif

// Constructors
psDirectX10::psDirectX10(const psVeciu& dim) : psDriver(dim), _device(0)
{
#ifdef BSS_DEBUG
  const UINT DEVICEFLAGS = D3D10_CREATE_DEVICE_SINGLETHREADED|D3D10_CREATE_DEVICE_DEBUG;
#else
  const UINT DEVICEFLAGS = D3D10_CREATE_DEVICE_SINGLETHREADED;
#endif
  IDXGIAdapter* adapter = NULL;

  HRESULT hr = D3D10CreateDevice(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, DEVICEFLAGS, D3D10_SDK_VERSION, &_device);
  if(FAILED(hr)) {
    return; //immediately give up
  }

  _fsquadVS = (ID3D10VertexShader*)CreateShader(fsquadVS_main, sizeof(fsquadVS_main), VERTEX_SHADER_4_0);
}
psDirectX10::~psDirectX10()
{
  if(_device)
    _device->Release();
}
bool psDirectX10::Begin() { return true; }
void psDirectX10::End() {}
void BSS_FASTCALL psDirectX10::Draw(psVertObj* buf, float(&transform)[4][4], FLAG_TYPE flags)
{ 
  unsigned int offset=0;
  _device->IASetVertexBuffers(0, 1, (ID3D10Buffer**)&buf->verts, &buf->vsize, &offset);
  _device->IASetInputLayout((ID3D10InputLayout*)buf->layout);
  _device->IASetPrimitiveTopology(_getdx10topology(buf->mode));
  
  if(!buf->indices)
    _device->Draw(buf->nvert, 0);
  else
  {
    _device->IASetIndexBuffer((ID3D10Buffer*)buf->indices, DXGI_FORMAT_R16_UINT, 0);
    _device->DrawIndexed(buf->nindice, 0, 0);
  }
}
void BSS_FASTCALL psDirectX10::DrawRect(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], const psTex* const* texes, unsigned char numtex, FLAG_TYPE flags) { }
void BSS_FASTCALL psDirectX10::DrawRectBatchBegin(const psTex* const* texes, unsigned char numtex, unsigned int numrects, FLAG_TYPE flags, const float xform[4][4]) { }
void BSS_FASTCALL psDirectX10::DrawRectBatch(const psRectRotateZ rect, const psRect& uv, const unsigned int(&vertexcolor)[4], FLAG_TYPE flags) { }
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
void* BSS_FASTCALL psDirectX10::CreateBuffer(unsigned short bytes, USAGETYPES usage, BUFFERTYPES type, void* initdata)
{
  D3D10_BUFFER_DESC desc ={
    bytes,
    (D3D10_USAGE)_usagetodxtype(usage),
    _buftypetodxtype(type),
    _usagetocpuflag(usage),
    _usagetomisc(usage),
  };

  D3D10_SUBRESOURCE_DATA subdata ={ initdata, 0, 0 };
  
  ID3D10Buffer* buf;
  if(FAILED(_device->CreateBuffer(&desc, !initdata?0:&subdata, &buf)))
  {
    PSLOG(1) << "CreateBuffer failed" << std::endl;
    buf=0;
  }
  return buf;
}
void* BSS_FASTCALL psDirectX10::CreateTexture(psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels, void* initdata, void** additionalview=0)
{
  ID3D10Texture2D* tex = 0;
  D3D10_TEXTURE2D_DESC desc ={ dim.x, dim.y, miplevels, 1, (DXGI_FORMAT)_fmttodx10(format), { 0, 0 }, (D3D10_USAGE)_usagetodxtype(usage), _usagetobind(usage), _usagetocpuflag(usage), _usagetomisc(usage) };
  D3D10_SUBRESOURCE_DATA subdata ={ initdata, (((_getbitsperpixel(format)*dim.x)+7)<<3), 0 }; // The +7 here is to force the per-line bits to correctly round up the number of bytes.

  if(FAILED(_device->CreateTexture2D(&desc, !initdata?0:&subdata, &tex)))
  {
    PSLOG(1) << "CreateTexture failed" << std::endl;
    return 0;
  }
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE)!=0)?_createshaderview(tex):tex;
}
void* BSS_FASTCALL psDirectX10::LoadTexture(const char* path, USAGETYPES usage, FORMATS format, unsigned char miplevels, void** additionalview=0)
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
  if(FAILED(D3DX10CreateTextureFromFileW(_device, cStrW(path), &info, 0, &tex, 0)))
  {
    PSLOG(1) << "LoadTexture failed for " << path << std::endl;
    return 0;
  }
  if(usage&USAGE_RENDERTARGET)
    *additionalview = _creatertview(tex);
  else if(usage&USAGE_DEPTH_STENCIL)
    *additionalview = _createdepthview(tex);
  return ((usage&USAGE_SHADER_RESOURCE)!=0)?_createshaderview(tex):tex;
}
void* BSS_FASTCALL psDirectX10::LoadTextureInMemory(const void* data, size_t datasize, USAGETYPES usage, FORMATS format, unsigned char miplevels, void** additionalview=0)
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
  if(FAILED(D3DX10CreateTextureFromMemory(_device, data, datasize, &info, 0, &tex, 0)))
  {
    PSLOG(1) << "LoadTexture failed for " << data << std::endl;
    tex=0;
  }
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
    _device->VSSetConstantBuffers(0, 1, (ID3D10Buffer**)&constbuf);
  else if(shader<=PIXEL_SHADER_5_0)
    _device->PSSetConstantBuffers(0, 1, (ID3D10Buffer**)&constbuf);
  else if(shader<=GEOMETRY_SHADER_5_0)
    _device->GSSetConstantBuffers(0, 1, (ID3D10Buffer**)&constbuf);
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
    ((ID3D10Buffer*)p)->Release();
    break;
  case RES_SURFACE:
    ((ID3D10RenderTargetView*)p)->Release();
    break;
  }
}

const char* shader_profiles[] ={ "vs_1_1", "vs_2_0", "vs_2_a", "vs_3_0", "vs_4_0", "vs_4_1", "vs_5_0", "ps_1_1", "ps_1_2", "ps_1_3", "ps_1_4", "ps_2_0",
"ps_2_a", "ps_2_b", "ps_3_0", "ps_4_0", "ps_4_1", "ps_5_0", "gs_4_0", "gs_4_1", "gs_5_0", "cs_4_0", "cs_4_1",
"cs_5_0", "ds_5_0", "hs_5_0" };

void* BSS_FASTCALL psDirectX10::CompileShader(const char* source, SHADER_VER profile, const char* entrypoint)
{
  ID3D10Blob* ret=0;
  ID3D10Blob* err=0;
  HRESULT res=0;
  if(FAILED(res=D3D10CompileShader(source, strlen(source), 0, 0, 0, entrypoint, shader_profiles[profile], 0, &ret, &err)))
  {
    if(!err) PSLOG(2) << "The effect could not be compiled! ERR: " << res << " Source: \n" << source << std::endl;
    else if(res==0x8007007e) PSLOG(2) << "The effect cannot be loaded because a module cannot be found (?) Source: \n" << source << std::endl;
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
    _device->CreateVertexShader(data, datasize, (ID3D10VertexShader**)&p);
  else if(profile<=PIXEL_SHADER_5_0)
    _device->CreatePixelShader(data, datasize, (ID3D10PixelShader**)&p);
  else if(profile<=GEOMETRY_SHADER_5_0)
    _device->CreateGeometryShader(data, datasize, (ID3D10GeometryShader**)&p);
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

unsigned int BSS_FASTCALL psDirectX10::_usagetodxtype(USAGETYPES types)
{
  switch(types)
  {
  case USAGE_DEFAULT: return D3D10_USAGE_DEFAULT;
  case USAGE_IMMUTABLE: return D3D10_USAGE_IMMUTABLE;
  case USAGE_DYNAMIC: return D3D10_USAGE_DYNAMIC;
  case USAGE_STAGING: return D3D10_USAGE_STAGING;
  }
  return -1;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetocpuflag(USAGETYPES types)
{
  switch(types)
  {
  case USAGE_DEFAULT: return 0;
  case USAGE_IMMUTABLE: return 0;
  case USAGE_DYNAMIC: return D3D10_CPU_ACCESS_WRITE;
  case USAGE_STAGING: return D3D10_CPU_ACCESS_WRITE|D3D10_CPU_ACCESS_READ;
  }
  return 0;
}
unsigned int BSS_FASTCALL psDirectX10::_buftypetodxtype(BUFFERTYPES types)
{
  switch(types)
  {
  case BUFFER_INDEX: return D3D10_BIND_INDEX_BUFFER;
  case BUFFER_VERTEX: return D3D10_BIND_VERTEX_BUFFER;
  }
  return -1;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetomisc(USAGETYPES types)
{
  unsigned int r=0;
  if(types&USAGE_AUTOGENMIPMAP) r |= D3D10_RESOURCE_MISC_GENERATE_MIPS;
  if(types&USAGE_TEXTURECUBE) r |= D3D10_RESOURCE_MISC_TEXTURECUBE;
  return r;
}
unsigned int BSS_FASTCALL psDirectX10::_usagetobind(USAGETYPES types)
{
  unsigned int r=0;
  if(types&USAGE_RENDERTARGET) r |= D3D10_BIND_RENDER_TARGET;
  if(types&USAGE_SHADER_RESOURCE) r |= D3D10_BIND_SHADER_RESOURCE;
  if(types&USAGE_DEPTH_STENCIL) r |= D3D10_BIND_DEPTH_STENCIL;
  if(types&USAGE_CONSTANT_BUFFER) r |= D3D10_BIND_CONSTANT_BUFFER;
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
  D3D10_RENDER_TARGET_VIEW_DESC rtDesc;
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
  }

  ID3D10RenderTargetView* r = 0;
  if(FAILED(_device->CreateRenderTargetView(src, &rtDesc, &r)))
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