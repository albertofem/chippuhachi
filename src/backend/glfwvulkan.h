#ifndef CHIPPUHACHI_GLFWVULKAN_H
#define CHIPPUHACHI_GLFWVULKAN_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "videobackend.h"
#include "imgui_impl_vulkan.h"
#include "../system.h"

class glfwvulkan : public videobackend {
    const int MIN_IMAGE_COUNT = 2;

    // Vulkan structs
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkQueue vkQueue = VK_NULL_HANDLE;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT vkDebugReport = VK_NULL_HANDLE;
    VkAllocationCallbacks *vkAllocator = nullptr;
    VkSurfaceKHR vkSurfaceKhr{};

    // imgui
    ImGui_ImplVulkanH_Window imgUiWindow;
    ImGui_ImplVulkanH_Window *imgUiWindowPtr = &imgUiWindow;

    int width{};
    int height{};

    const char* appName{};

    bool swapChainRebuild = false;
    int swapChainResizeWidth = 0;
    int swapChainResizeHeight = 0;

    videobackendResult* imgUiFrameRender();
    videobackendResult* imgUiFramePresent();
    videobackendResult* imgUiUploadFonts();
public:
    videobackendResult* run(class system* system) override;

    void init(int width, int height, const char *t_appName) override;

    void glfwResizeCallback(GLFWwindow *, int width, int height);
};


#endif