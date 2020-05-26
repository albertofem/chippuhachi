#ifndef CHIPPUHACHI_IMGUI_IMPL_VULKAN_H
#define CHIPPUHACHI_IMGUI_IMPL_VULKAN_H

#include "imgui.h"      // IMGUI_IMPL_API
#include <vulkan/vulkan.h>

// Initialization data, for ImGui_ImplVulkan_Init()
// [Please zero-clear before use!]
struct ImGui_ImplVulkan_InitInfo {
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    uint32_t QueueFamily;
    VkQueue Queue;
    VkPipelineCache PipelineCache;
    VkDescriptorPool DescriptorPool;
    uint32_t MinImageCount;          // >= 2
    uint32_t ImageCount;             // >= MinImageCount
    VkSampleCountFlagBits MSAASamples;   // >= VK_SAMPLE_COUNT_1_BIT
    const VkAllocationCallbacks *Allocator;

    void (*CheckVkResultFn)(VkResult err);
};

IMGUI_IMPL_API bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo *info, VkRenderPass render_pass);

IMGUI_IMPL_API void ImGui_ImplVulkan_Shutdown();

IMGUI_IMPL_API void ImGui_ImplVulkan_NewFrame();

IMGUI_IMPL_API void ImGui_ImplVulkan_RenderDrawData(ImDrawData *draw_data, VkCommandBuffer command_buffer);

IMGUI_IMPL_API bool ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);

IMGUI_IMPL_API void ImGui_ImplVulkan_DestroyFontUploadObjects();

IMGUI_IMPL_API void ImGui_ImplVulkan_SetMinImageCount(
        uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain is recreated)

IMGUI_IMPL_API ImTextureID    ImGui_ImplVulkan_AddTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);


struct ImGui_ImplVulkanH_Frame;
struct ImGui_ImplVulkanH_Window;

IMGUI_IMPL_API void
ImGui_ImplVulkanH_CreateWindow(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device,
                               ImGui_ImplVulkanH_Window *wnd, uint32_t queue_family,
                               const VkAllocationCallbacks *allocator, int w, int h, uint32_t min_image_count);

IMGUI_IMPL_API void ImGui_ImplVulkanH_DestroyWindow(VkInstance instance, VkDevice device, ImGui_ImplVulkanH_Window *wnd,
                                                    const VkAllocationCallbacks *allocator);

IMGUI_IMPL_API VkSurfaceFormatKHR
ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                      const VkFormat *request_formats, int request_formats_count,
                                      VkColorSpaceKHR request_color_space);

IMGUI_IMPL_API VkPresentModeKHR
ImGui_ImplVulkanH_SelectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                    const VkPresentModeKHR *request_modes, int request_modes_count);

IMGUI_IMPL_API int ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(VkPresentModeKHR present_mode);

struct ImGui_ImplVulkanH_Frame {
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;
    VkFence Fence;
    VkImage Backbuffer;
    VkImageView BackbufferView;
    VkFramebuffer Framebuffer;
};

struct ImGui_ImplVulkanH_FrameSemaphores {
    VkSemaphore ImageAcquiredSemaphore;
    VkSemaphore RenderCompleteSemaphore;
};

struct ImGui_ImplVulkanH_Window {
    int Width;
    int Height;
    VkSwapchainKHR Swapchain;
    VkSurfaceKHR Surface;
    VkSurfaceFormatKHR SurfaceFormat;
    VkPresentModeKHR PresentMode;
    VkRenderPass RenderPass;
    bool ClearEnable;
    VkClearValue ClearValue;
    uint32_t FrameIndex;             // Current frame being rendered to (0 <= FrameIndex < FrameInFlightCount)
    uint32_t ImageCount;             // Number of simultaneous in-flight frames (returned by vkGetSwapchainImagesKHR, usually derived from min_image_count)
    uint32_t SemaphoreIndex;         // Current set of swapchain wait semaphores we're using (needs to be distinct from per frame data)
    ImGui_ImplVulkanH_Frame *Frames;
    ImGui_ImplVulkanH_FrameSemaphores *FrameSemaphores;

    ImGui_ImplVulkanH_Window() {
        memset(this, 0, sizeof(*this));
        PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        ClearEnable = true;
    }
};


#endif //CHIPPUHACHI_IMGUI_IMPL_VULKAN_H
