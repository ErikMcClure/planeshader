// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __VULKAN_H__PS__
#define __VULKAN_H__PS__

#include "psDriver.h"
#include "ps_feather.h"
#include "psTex.h"
#include "bss-util/win32_includes.h" // Vulkan will try to include the windows header itself, so first include it on our terms

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

namespace planeshader {
  struct SwapChainBuffer {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
  };

  class psVulkan : public psDriver, public psDriverHold
  {
  public:
    psVulkan(const psVeciu& dim, uint32_t antialias, bool vsync, bool fullscreen, bool sRGB, psMonitor* monitor);
    ~psVulkan();

    // Begins a scene
    virtual bool Begin() { return true; }
    // Ends a scene
    virtual char End() { return 0; }
    // Flush draw buffer
    virtual void Flush() {}
    virtual psBatchObj* FlushPreserve() { return 0; }
    // Draws a vertex object
    virtual void Draw(psVertObj* buf, psFlag flags, const float(&transform)[4][4] = identity) {}
    // Draws a rectangle
    virtual psBatchObj* DrawRect(psShader* shader, const psStateblock* stateblock, const psRectRotateZ& rect, const psRect* uv, uint8_t numuv, uint32_t color, psFlag flags) { return 0; }
    virtual psBatchObj* DrawRectBatchBegin(psShader* shader, const psStateblock* stateblock, uint8_t numuv, psFlag flags) { return 0; }
    virtual void DrawRectBatch(psBatchObj*& o, const psRectRotateZ& rect, const psRect* uv, uint32_t color) { }
    // Draws a polygon
    virtual psBatchObj* DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVec* verts, uint32_t num, psVec3D offset, unsigned long vertexcolor, psFlag flags) { return 0; }
    virtual psBatchObj* DrawPolygon(psShader* shader, const psStateblock* stateblock, const psVertex* verts, uint32_t num, psFlag flags) { return 0; }
    // Draws points
    virtual psBatchObj* DrawPoints(psShader* shader, const psStateblock* stateblock, psVertex* particles, uint32_t num, psFlag flags) { return 0; }
    // Draws lines
    virtual psBatchObj* DrawLinesStart(psShader* shader, const psStateblock* stateblock, psFlag flags) { return 0; }
    virtual void DrawLines(psBatchObj*& obj, const psLine& line, float Z1, float Z2, unsigned long vertexcolor) {}
    virtual psBatchObj* DrawCurveStart(psShader* shader, const psStateblock* stateblock, psFlag flags) { return 0; }
    virtual psBatchObj* DrawCurve(psBatchObj*& o, const psVertex* curve, uint32_t num) override { return 0; }
    // Applies a camera (if you need the current camera, look at the pass you belong to, not the driver)
    virtual void PushCamera(const psVec3D& pos, const psVec& pivot, FNUM rotation, const psRectiu& viewport, const psVec& extent) {}
    virtual void PushCamera3D(const float(&m)[4][4], const psRectiu& viewport) {}
    virtual void PopCamera() {}
    // Applies the camera transform (or it's inverse) according to the flags to a point.
    virtual psVec3D TransformPoint(const psVec3D& point) const { return VEC3D_ZERO; }
    virtual psVec3D ReversePoint(const psVec3D& point) const { return VEC3D_ZERO; }
    // Draws a fullscreen quad
    virtual void DrawFullScreenQuad() {}
    // Creates a vertex or index buffer
    virtual void* CreateBuffer(uint32_t capacity, uint32_t element, uint32_t usage, const void* initdata = 0) { return 0; }
    virtual void* LockBuffer(void* target, uint32_t flags) { return 0; }
    virtual void UnlockBuffer(void* target) {}
    virtual void* LockTexture(void* target, uint32_t flags, uint32_t& pitch, uint8_t miplevel = 0) { return 0; }
    virtual void UnlockTexture(void* target, uint8_t miplevel = 0) {}
    // Creates a texture
    virtual void* CreateTexture(psVeciu dim, FORMATS format, uint32_t usage = USAGE_SHADER_RESOURCE, uint8_t miplevels = 0, const void* initdata = 0, void** additionalview = 0, psTexblock* texblock = 0) { return (void*)~0; }
    virtual void* LoadTexture(const char* path, uint32_t usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, uint8_t miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false) { return (void*)~0; }
    virtual void* LoadTextureInMemory(const void* data, size_t datasize, uint32_t usage = USAGE_SHADER_RESOURCE, FORMATS format = FMT_UNKNOWN, void** additionalview = 0, uint8_t miplevels = 0, FILTERS mipfilter = FILTER_BOX, FILTERS loadfilter = FILTER_NONE, psVeciu dim = VEC_ZERO, psTexblock* texblock = 0, bool sRGB = false) { return (void*)~0; }
    virtual void CopyTextureRect(const psRectiu* srcrect, psVeciu destpos, void* src, void* dest, uint8_t miplevel = 0) {}
    // Pushes or pops a clip rect on to the stack
    virtual void PushClipRect(const psRect& rect) {}
    virtual psRect PeekClipRect() { return psRect(0, 0, 0, 0); }
    virtual void PopClipRect() {}
    // Sets the current rendertargets, setting all the rest to null.
    virtual void SetRenderTargets(const psTex* const* texes, uint8_t num, const psTex* depthstencil = 0) {}
    // Sets shader constants
    virtual void SetShaderConstants(void* constbuf, SHADER_VER shader) {}
    // Sets textures for a given type of shader (in DX9 this is completely ignored)
    virtual void SetTextures(const psTex* const* texes, uint8_t num, SHADER_VER shader = PIXEL_SHADER_1_1) {}
    // Builds a stateblock from the given set of state changes
    virtual void* CreateStateblock(const STATEINFO* states, uint32_t count) { return 0; }
    // Sets a stateblock
    virtual void SetStateblock(void* stateblock) { }
    // Builds a texblock from the given set of sampler states
    virtual void* CreateTexblock(const STATEINFO* states, uint32_t count) { return 0; }
    // Create a vertex layout from several element descriptions
    virtual void* CreateLayout(void* shader, const ELEMENT_DESC* elements, uint8_t num) { return 0; }
    virtual void SetLayout(void* layout) {}
    // Frees a created resource of the specified type
    virtual TEXTURE_DESC GetTextureDesc(void* t) { TEXTURE_DESC r = { psVec3Diu(0,0,0), USAGE_DEFAULT, FMT_UNKNOWN, 0 }; return r; }
    virtual void FreeResource(void* p, RESOURCE_TYPE t) {}
    virtual void GrabResource(void* p, RESOURCE_TYPE t) {}
    virtual void CopyResource(void* dest, void* src, RESOURCE_TYPE t) {}
    virtual void Resize(psVeciu dim, FORMATS format, char fullscreen) {}
    // Clears everything to a specified color
    virtual void Clear(uint32_t color) {}
    // Gets the backbuffer texture
    virtual psTex* GetBackBuffer() const { return _backbuffer; }
    // Gets a pointer to the driver implementation
    inline virtual RealDriver GetRealDriver() override { RealDriver d; d.vk = this; d.type = RealDriver::DRIVERTYPE_VULKAN; return d; }
    // Gets/Sets the effective DPI
    virtual void SetDPIScale(psVec dpiscale = psVec(1.0f)) {}
    virtual psVec GetDPIScale() const { return VEC_ZERO; }
    // Compile a shader from a string
    virtual void* CompileShader(const char* source, SHADER_VER profile, const char* entrypoint = "") { return 0; }
    // Create an actual shader object from compiled shader source (either precompiled or from CompileShader())
    virtual void* CreateShader(const void* data, SHADER_VER profile) { return 0; }
    // Sets current shader
    virtual char SetShader(void* shader, SHADER_VER profile) { return -1; }
    // Returns true if shader version is supported
    virtual bool ShaderSupported(SHADER_VER profile) { return false; }
    // Returns an index to an internal state snapshot
    virtual uint32_t GetSnapshot() { return 0; }

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;

    static const char* VkResultToString(const VkResult result);

  protected:
    void _prepare(bool fullscreen, bool sRGB, psMonitor* monitor);
    void _loadswapchain(const psVeciu& dim, bool vsync);
    void _createdevice();
    void _createcommands();
    void _createvertices();
    void _createlayout();
    void _createpass();
    void _createpipeline();
    void _createpools();
    void _createframebuffers();

    VkInstance _instance;
    VkPhysicalDevice _gpu;
    VkDevice _device;
    VkQueue _queue;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkCommandPool _cmdpool;
    VkCommandBuffer _cmdbuffer;
    VkPhysicalDeviceProperties _device_props;
    VkPhysicalDeviceFeatures _device_features;
    VkFormat _format;
    VkColorSpaceKHR _colorspace;
    VkPhysicalDeviceMemoryProperties _memory_props;
    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    uint32_t graphics_queue_node_index;
    uint32_t _swapchainImageCount;
    char* extension_names[64];
    uint32_t queue_count;
    std::unique_ptr<VkQueueFamilyProperties[]> queue_props;
    std::unique_ptr<SwapChainBuffer[]> _buffers;
    psTex* _backbuffer;
  };
}

#endif
