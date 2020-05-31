#ifndef CHIPPUHACHI_GLFWVULKAN_H
#define CHIPPUHACHI_GLFWVULKAN_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "videobackend.h"
#include "imgui_impl_vulkan.h"
#include "../system.h"
#include "vertex.h"

class glfwvulkan : public videobackend {
    const int MIN_IMAGE_COUNT = 2;
    const float VULKAN_QUEUE_PRIORITIES[1]{
            1.0f
    };

    // Vulkan init config
    std::vector<const char *> instanceLayers = {};
    std::vector<const char *> instanceExtensions = {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    };
    std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    uint32_t graphicsQueueFamily = UINT32_MAX;

    // Vulkan structs
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice;
    VkQueue vkQueue = VK_NULL_HANDLE;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT vkDebugReport = VK_NULL_HANDLE;
    VkAllocationCallbacks *vkAllocator = nullptr;
    VkPipelineCache vkPipelineCache = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurfaceKhr{};


    // imgui
    ImGui_ImplVulkanH_Window imgUiWindow;
    ImGui_ImplVulkanH_Window *imgUiWindowPtr = &imgUiWindow;

    int width{};
    int height{};

    const char *appName{};

    bool swapChainRebuild = false;
    int swapChainResizeWidth = 0;
    int swapChainResizeHeight = 0;

    videobackendResult *imgUiFrameRender();

    videobackendResult *imgUiFramePresent();

    videobackendResult *imgUiUploadFonts();

    std::vector<Vertex> toDrawVertices;

    void* emulationPixelBufferData;
    VkCommandBuffer emulationCommandBuffer;
    VkBuffer emulationPixelBuffer;
    VkDeviceMemory emulationPixelMemoryBuffer;
    VkDeviceSize emulationPixelBufferSize;
    VkImage emulationPixelImage;
    VkImageView emulationPixelImageView;
    VkSampler emulationPixelImageSampler;

public:
    videobackendResult *run(class system *system) override;

    void init(int width, int height, const char *t_appName) override;

    void glfwResizeCallback(GLFWwindow *, int width, int height);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool createEmulationPixelBuffer(unsigned short width_t, unsigned short height_t);

    bool createEmulationPixelImage(unsigned short width_t, unsigned short height_t);

    bool registerImageBufferCommands(unsigned short width_t, unsigned short height_t);

    bool createImage(VkImage &image, unsigned short width_t, unsigned short height_t);
};


#endif