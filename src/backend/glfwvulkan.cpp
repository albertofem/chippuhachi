#include "glfwvulkan.h"
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
vkDebugReportFn(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location,
                int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData) {
    (void) flags;
    (void) object;
    (void) location;
    (void) messageCode;
    (void) pUserData;
    (void) pLayerPrefix;

    spdlog::debug("Vulkan debug: Object Type: {} - Message: {}", objectType, pMessage);

    return VK_FALSE;
}

// needs to live outside the class because of
// https://stackoverflow.com/questions/7852101/c-lambda-with-captures-as-a-function-pointer
static void imgUiCheckError(VkResult vkResult) {
    if (vkResult == 0) {
        return;
    }

    spdlog::error("Error returned from imgui: VkResult = %d\n", vkResult);
}

videobackendResult *glfwvulkan::run(class system* emulatedSystem) {
    std::vector<const char *> instanceLayers = {};
    std::vector<const char *> instanceExtensions = {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    };
    std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    glfwInit();

    if (glfwVulkanSupported() == GLFW_FALSE) {
        glfwTerminate();
        return videobackendResult::createWithError("GLFW does not supports Vulkan on this platform");
    }

    uint32_t instanceExtensionCount = 0;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);

    for (uint32_t i = 0; i < instanceExtensionCount; ++i) {
        instanceExtensions.push_back(glfwRequiredExtensions[i]);
    }

    VkResult vkResult;

    VkApplicationInfo vkApplicationInfo{};
    vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkApplicationInfo.pApplicationName = appName;
    vkApplicationInfo.pEngineName = appName;

    VkInstanceCreateInfo vkInstanceCreateInfo{};
    vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
    vkInstanceCreateInfo.enabledLayerCount = instanceLayers.size();
    vkInstanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
    vkInstanceCreateInfo.enabledExtensionCount = instanceExtensions.size();
    vkInstanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    vkResult = vkCreateInstance(&vkInstanceCreateInfo, vkAllocator, &vkInstance);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to create Vulkan instance", vkResult);
    }

    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(
            vkInstance, "vkCreateDebugReportCallbackEXT"
    );

    if (vkCreateDebugReportCallbackEXT == nullptr) {
        return videobackendResult::createWithError(
                "Unable to register debug callback function, maybe extension is not enabled?");
    }

    VkDebugReportCallbackCreateInfoEXT debugReportCi = {};

    debugReportCi.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCi.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                          VK_DEBUG_REPORT_WARNING_BIT_EXT |
                          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debugReportCi.pfnCallback = vkDebugReportFn;
    debugReportCi.pUserData = nullptr;

    vkResult = vkCreateDebugReportCallbackEXT(vkInstance, &debugReportCi, vkAllocator, &vkDebugReport);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to register debug report", vkResult);
    }

    uint32_t gpuCount;
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to enumerate Vulkan physical devices", vkResult);
    }

    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &gpuCount, gpus.data());

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to enumerate Vulkan physical devices", vkResult);
    }

    VkPhysicalDevice physicalDevice = gpus[0];

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queueFamilyCount, familyProperties.data());

    uint32_t graphicsQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
        }
    }

    if (graphicsQueueFamily == UINT32_MAX) {
        glfwTerminate();
        return videobackendResult::createWithError("Non-supported graphics Vulkan family vkDevice found");
    }

    const float priorities[]{1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    deviceQueueCreateInfo.pQueuePriorities = priorities;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();

    vkCreateDevice(gpus[0], &deviceCreateInfo, vkAllocator, &vkDevice);
    vkGetDeviceQueue(vkDevice, graphicsQueueFamily, 0, &vkQueue);

    VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
    };

    VkDescriptorPoolCreateInfo poolInfo = {};

    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t) IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    vkResult = vkCreateDescriptorPool(vkDevice, &poolInfo, vkAllocator, &vkDescriptorPool);

    if (vkResult < 0) {
        glfwTerminate();
        return videobackendResult::createWithError("Unable to create Vulkan descriptor pool", vkResult);
    }

    glfwWindowHint(
            GLFW_CLIENT_API,
            GLFW_NO_API
    );

    auto window = glfwCreateWindow(width, height, appName, nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);

    glfwGetFramebufferSize(window, &width, &height);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int h, int w) {
        auto &self = *static_cast<glfwvulkan *>(glfwGetWindowUserPointer(window));
        self.glfwResizeCallback(window, h, w);
    });

    vkResult = glfwCreateWindowSurface(vkInstance, window, vkAllocator, &vkSurfaceKhr);

    if (vkResult < 0) {
        glfwTerminate();
        return videobackendResult::createWithError("Unable to create GLFW vulkan window vkSurfaceKhr", vkResult);
    }

    imgUiWindowPtr->Surface = vkSurfaceKhr;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamily, imgUiWindowPtr->Surface, &res);

    if (res != VK_TRUE) {
        glfwTerminate();
        return videobackendResult::createWithError("Error no WSI support on physical vkDevice 0");
    }

    const VkFormat requestSurfaceImageFormat[] = {
            VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM
    };

    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    imgUiWindowPtr->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
            physicalDevice,
            imgUiWindowPtr->Surface,
            requestSurfaceImageFormat,
            (size_t) IM_ARRAYSIZE(requestSurfaceImageFormat),
            requestSurfaceColorSpace
    );

#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR presentModes[] = {
            VK_PRESENT_MODE_MAILBOX_KHR,
            VK_PRESENT_MODE_IMMEDIATE_KHR,
            VK_PRESENT_MODE_FIFO_KHR
    };
#else
    VkPresentModeKHR presentModes[] = {
            VK_PRESENT_MODE_FIFO_KHR
    };
#endif
    imgUiWindowPtr->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
            physicalDevice,
            imgUiWindowPtr->Surface,
            &presentModes[0],
            IM_ARRAYSIZE(presentModes)
    );

    IM_ASSERT(MIN_IMAGE_COUNT >= 2);

    ImGui_ImplVulkanH_CreateWindow(
            vkInstance,
            physicalDevice,
            vkDevice,
            imgUiWindowPtr,
            graphicsQueueFamily,
            vkAllocator,
            width,
            height,
            MIN_IMAGE_COUNT
    );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::StyleColorsDark();

    VkPipelineCache vkPipelineCache = VK_NULL_HANDLE;

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkInstance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = vkDevice;
    init_info.QueueFamily = graphicsQueueFamily;
    init_info.Queue = vkQueue;
    init_info.PipelineCache = vkPipelineCache;
    init_info.DescriptorPool = vkDescriptorPool;
    init_info.Allocator = vkAllocator;
    init_info.MinImageCount = MIN_IMAGE_COUNT;
    init_info.ImageCount = imgUiWindowPtr->ImageCount;
    init_info.CheckVkResultFn = imgUiCheckError;

    ImGui_ImplVulkan_Init(&init_info, imgUiWindowPtr->RenderPass);

    auto result = io.Fonts->AddFontFromFileTTF("resources/NotoSansCJKjp-Medium.otf", 20.0f, NULL,
                                               io.Fonts->GetGlyphRangesJapanese());

    if (result == nullptr) {
        return videobackendResult::createWithError("Unable to load Japanese font set");
    }

    imgUiUploadFonts();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    spdlog::info("Vulkan/GLFW initialized, starting render loop");

    videobackendResult *vbResult = nullptr;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (swapChainRebuild) {
            swapChainRebuild = false;
            ImGui_ImplVulkan_SetMinImageCount(MIN_IMAGE_COUNT);
            ImGui_ImplVulkanH_CreateWindow(
                    vkInstance,
                    physicalDevice,
                    vkDevice,
                    &imgUiWindow,
                    graphicsQueueFamily,
                    vkAllocator,
                    swapChainResizeWidth,
                    swapChainResizeHeight,
                    MIN_IMAGE_COUNT
            );

            imgUiWindow.FrameIndex = 0;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin(u8"チップ「８」", NULL, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Rom")) {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        ImGui::Begin("Emulation");
        {
            if (emulatedSystem->step())
            {
                // send a frame to draw
            }
        }
        ImGui::End();

        ImGui::Render();
        memcpy(&imgUiWindowPtr->ClearValue.color.float32[0], &clear_color, 4 * sizeof(float));

        vbResult = imgUiFrameRender();

        if (!vbResult->isSuccess) {
            break;
        }

        vbResult = imgUiFramePresent();

        if (!vbResult->isSuccess) {
            break;
        }
    }

    spdlog::info("Shutting down Vulkan/GLFW render context");

    vkDestroySurfaceKHR(vkInstance, vkSurfaceKhr, nullptr);

    glfwDestroyWindow(window);

    vkDestroyDevice(vkDevice, nullptr);
    vkDestroyInstance(vkInstance, nullptr);

    glfwTerminate();

    return vbResult == nullptr ? videobackendResult::createSuccessful() : vbResult;
}

void glfwvulkan::init(int t_width, int t_height, const char *t_appName) {
    width = t_width;
    height = t_height;
    appName = t_appName;
}

void glfwvulkan::glfwResizeCallback(GLFWwindow *, int width_t, int height_t) {
    swapChainRebuild = true;
    swapChainResizeWidth = width_t;
    swapChainResizeHeight = height_t;
}

videobackendResult *glfwvulkan::imgUiFrameRender() {
    VkResult vkResult;

    VkSemaphore vkImageSemaphore = imgUiWindowPtr
            ->FrameSemaphores[imgUiWindowPtr->SemaphoreIndex]
            .ImageAcquiredSemaphore;

    VkSemaphore vkRenderCompleteSemaphore = imgUiWindowPtr
            ->FrameSemaphores[imgUiWindowPtr->SemaphoreIndex]
            .RenderCompleteSemaphore;

    vkResult = vkAcquireNextImageKHR(
            vkDevice,
            imgUiWindowPtr->Swapchain,
            UINT64_MAX,
            vkImageSemaphore,
            VK_NULL_HANDLE,
            &imgUiWindowPtr->FrameIndex
    );

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to acquire next image", vkResult);
    }

    ImGui_ImplVulkanH_Frame *fd = &imgUiWindowPtr->Frames[imgUiWindowPtr->FrameIndex];
    {
        vkResult = vkWaitForFences(
                vkDevice,
                1, // fence count
                &fd->Fence,
                VK_TRUE,
                UINT64_MAX
        );

        if (vkResult < 0) {
            return videobackendResult::createWithError("Error waiting for fences", vkResult);
        }

        vkResult = vkResetFences(
                vkDevice,
                1,
                &fd->Fence
        );

        if (vkResult < 0) {
            return videobackendResult::createWithError("Error resetting fencd", vkResult);
        }
    }

    {
        vkResult = vkResetCommandPool(vkDevice, fd->CommandPool, 0);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Error resetting command pool", vkResult);
        }

        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkResult = vkBeginCommandBuffer(fd->CommandBuffer, &info);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Error beginning command buffer", vkResult);
        }
    }

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = imgUiWindowPtr->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = imgUiWindowPtr->Width;
        info.renderArea.extent.height = imgUiWindowPtr->Height;
        info.clearValueCount = 1;
        info.pClearValues = &imgUiWindowPtr->ClearValue;

        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fd->CommandBuffer);

    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &vkImageSemaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &vkRenderCompleteSemaphore;

        vkResult = vkEndCommandBuffer(fd->CommandBuffer);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Error ending command buffer", vkResult);
        }

        vkResult = vkQueueSubmit(vkQueue, 1, &info, fd->Fence);


        if (vkResult < 0) {
            return videobackendResult::createWithError("Failed to submit queue", vkResult);
        }
    }

    return videobackendResult::createSuccessful();
}

videobackendResult *glfwvulkan::imgUiFramePresent() {
    VkSemaphore vkRenderCompleteSemaphore = imgUiWindowPtr
            ->FrameSemaphores[imgUiWindowPtr->SemaphoreIndex]
            .RenderCompleteSemaphore;

    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkRenderCompleteSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &imgUiWindowPtr->Swapchain;
    info.pImageIndices = &imgUiWindowPtr->FrameIndex;

    VkResult vkResult = vkQueuePresentKHR(vkQueue, &info);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Error presenting queue", vkResult);
    }

    imgUiWindowPtr->SemaphoreIndex = (imgUiWindowPtr->SemaphoreIndex + 1) % imgUiWindowPtr->ImageCount;

    return videobackendResult::createSuccessful();
}

videobackendResult *glfwvulkan::imgUiUploadFonts() {
    VkResult vkResult;

    VkCommandPool commandPool = imgUiWindowPtr->Frames[imgUiWindowPtr->FrameIndex].CommandPool;
    VkCommandBuffer commandBuffer = imgUiWindowPtr->Frames[imgUiWindowPtr->FrameIndex].CommandBuffer;

    vkResult = vkResetCommandPool(vkDevice, commandPool, 0);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to reset command pool", vkResult);
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkResult = vkBeginCommandBuffer(commandBuffer, &begin_info);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to begin command buffer", vkResult);
    }

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &commandBuffer;

    vkResult = vkEndCommandBuffer(commandBuffer);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to end command buffer", vkResult);
    }

    vkResult = vkQueueSubmit(vkQueue, 1, &end_info, VK_NULL_HANDLE);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to submit queue", vkResult);
    }

    vkResult = vkDeviceWaitIdle(vkDevice);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to wait for device", vkResult);
    }

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return videobackendResult::createSuccessful();
}

