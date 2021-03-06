#ifndef CHIPPUHACHI_GLFWVULKAN_H
#define CHIPPUHACHI_GLFWVULKAN_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <map>
#include "videobackend.h"
#include "imgui_impl_vulkan.h"
#include "../system.h"

class glfwvulkan : public videobackend {
    std::map<int, char> GLFW_KEYMAP = {
            {GLFW_KEY_1, 0x1},
            {GLFW_KEY_2, 0x2},
            {GLFW_KEY_3, 0x3},
            {GLFW_KEY_4, 0xC},
            {GLFW_KEY_Q, 0x4},
            {GLFW_KEY_W, 0x5},
            {GLFW_KEY_E, 0x6},
            {GLFW_KEY_R, 0xD},
            {GLFW_KEY_A, 0x7},
            {GLFW_KEY_S, 0x8},
            {GLFW_KEY_D, 0x9},
            {GLFW_KEY_F, 0xE},
            {GLFW_KEY_Y, 0xA},
            {GLFW_KEY_X, 0x0},
            {GLFW_KEY_C, 0xB},
            {GLFW_KEY_V, 0xF}
    };

    const int EMULATION_WINDOW_PADDING = 30;
    const int MIN_IMAGE_COUNT = 2;
    const float VULKAN_QUEUE_PRIORITIES[1]{
            1.0f
    };

    const ImVec4 CLEAR_COLOR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

    int windowWidth{};
    int windowHeight{};

    unsigned short emulationWindowWidth{};
    unsigned short emulationWindowHeight{};

    const char *appName{};

    bool swapChainRebuild = false;
    int swapChainResizeWidth = 0;
    int swapChainResizeHeight = 0;

    videobackendResult *imgUiFrameRender();

    videobackendResult *imgUiFramePresent();

    videobackendResult *imgUiUploadFonts();

    // emulation render data
    void *emulationPixelBufferData;
    VkCommandBuffer emulationCommandBuffer;

    VkDeviceSize emulationPixelBufferSize;
    VkBuffer emulationPixelBuffer;

    VkDeviceMemory emulationPixelMemoryBuffer;
    VkDeviceMemory emulationPixelScaledMemoryBuffer;

    VkImage emulationPixelImage;
    VkImage emulationScaledPixelImage;

    // for imgui
    VkImageView emulationPixelImageView;
    VkSampler emulationPixelImageSampler;

    // emulated system
    class system* emulatedSystemPtr;
    bool emulationFocus = false;

public:
    videobackendResult *run(class system *system) override;

    void init(int width, int height, const char *appName_t) override;

    void glfwResizeCallback(GLFWwindow *, int width, int height);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool createEmulationPixelBuffer(unsigned short width_t, unsigned short height_t);

    bool createEmulationPixelImage(unsigned short width_t, unsigned short height_t);

    bool createEmulationPixelScaledImage(unsigned short width_t, unsigned short height_t);

    videobackendResult *registerImageBufferCommands(unsigned short width_t, unsigned short height_t);

    bool createImage(unsigned short width_t, unsigned short height_t);

    void initializeDeviceQueue(const std::vector<VkPhysicalDevice> &gpus);

    VkResult initializeVulkan();

    void initializeQueueFamily();

    VkResult initializeDescriptorPool();

    void initializeImGui(GLFWwindow *window);

    static ImFont *loadImGuiJapaneseFont(ImGuiIO &io);

    static void createPipelineBarrier(VkCommandBuffer
                                      commandBuffer,
                                      VkPipelineStageFlags srcStage, VkPipelineStageFlags
                                      dstStage,
                                      std::vector<VkMemoryBarrier> memoryBarries, std::vector<VkBufferMemoryBarrier>
                                      bufferBarries,
                                      std::vector<VkImageMemoryBarrier> imageBarriers
    );

    videobackendResult *initializeCommandBuffer(VkCommandBuffer &vkCommandBuffer);
};


#endif