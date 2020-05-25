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
debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location,
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

static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

static void FrameRender(ImGui_ImplVulkanH_Window *wd, VkDevice device, VkQueue queue) {
    VkResult err;

    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE,
                                &wd->FrameIndex);
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(device, 1, &fd->Fence, VK_TRUE,
                              UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd, VkQueue queue) {
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(queue, &info);
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

videobackendResult *glfwvulkan::run() {
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
    VkAllocationCallbacks *vkAllocator = NULL;

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
    debugReportCi.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debugReportCi.pfnCallback = debug_report;
    debugReportCi.pUserData = nullptr;

    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;

    vkResult = vkCreateDebugReportCallbackEXT(vkInstance, &debugReportCi, vkAllocator, &debugReport);

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
        return videobackendResult::createWithError("Non-supported graphics Vulkan family device found");
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

    VkDevice device = VK_NULL_HANDLE;
    vkCreateDevice(gpus[0], &deviceCreateInfo, vkAllocator, &device);

    VkQueue vkQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &vkQueue);

    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;

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

    vkResult = vkCreateDescriptorPool(device, &poolInfo, vkAllocator, &vkDescriptorPool);

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

    ImGui_ImplVulkanH_Window imgUiWindow;
    ImGui_ImplVulkanH_Window *imgUiWindowPtr = &imgUiWindow;

    VkSurfaceKHR surface;
    vkResult = glfwCreateWindowSurface(vkInstance, window, vkAllocator, &surface);

    if (vkResult < 0) {
        glfwTerminate();
        return videobackendResult::createWithError("Unable to create GLFW vulkan window surface", vkResult);
    }

    imgUiWindowPtr->Surface = surface;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamily, imgUiWindowPtr->Surface, &res);

    if (res != VK_TRUE) {
        glfwTerminate();
        return videobackendResult::createWithError("Error no WSI support on physical device 0");
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
    VkPresentModeKHR presentModes[] = {VK_PRESENT_MODE_FIFO_KHR};
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
            device,
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
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    VkPipelineCache vkPipelineCache = VK_NULL_HANDLE;

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkInstance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = graphicsQueueFamily;
    init_info.Queue = vkQueue;
    init_info.PipelineCache = vkPipelineCache;
    init_info.DescriptorPool = vkDescriptorPool;
    init_info.Allocator = vkAllocator;
    init_info.MinImageCount = MIN_IMAGE_COUNT;
    init_info.ImageCount = imgUiWindowPtr->ImageCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, imgUiWindowPtr->RenderPass);

    auto result = io.Fonts->AddFontFromFileTTF("resources/NotoSansCJKjp-Medium.otf", 20.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    if (result == nullptr)
    {
        return videobackendResult::createWithError("Unable to load Japanese font set");
    }

    // Upload Fonts
    {
        // Use any command queue
        VkCommandPool command_pool = imgUiWindowPtr->Frames[imgUiWindowPtr->FrameIndex].CommandPool;
        VkCommandBuffer command_buffer = imgUiWindowPtr->Frames[imgUiWindowPtr->FrameIndex].CommandBuffer;

        vkResult = vkResetCommandPool(device, command_pool, 0);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Unable to reset command pool", vkResult);
        }

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkResult = vkBeginCommandBuffer(command_buffer, &begin_info);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Unable to begin command buffer", vkResult);
        }

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;

        vkResult = vkEndCommandBuffer(command_buffer);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Unable to end command buffer", vkResult);
        }

        vkResult = vkQueueSubmit(vkQueue, 1, &end_info, VK_NULL_HANDLE);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Unable to submit queue", vkResult);
        }

        vkResult = vkDeviceWaitIdle(device);

        if (vkResult < 0) {
            return videobackendResult::createWithError("Unable to wait for device", vkResult);
        }

        ImGui_ImplVulkan_DestroyFontUploadObjects();

    }

    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    spdlog::info("Vulkan/GLFW initialized, starting render loop");


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (swapChainRebuild) {
            swapChainRebuild = false;
            ImGui_ImplVulkan_SetMinImageCount(MIN_IMAGE_COUNT);
            ImGui_ImplVulkanH_CreateWindow(
                    vkInstance,
                    physicalDevice,
                    device,
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
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Rom"))
            {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        ImGui::Begin("Emulation");
        {

        }
        ImGui::End();

        ImGui::Render();
        memcpy(&imgUiWindowPtr->ClearValue.color.float32[0], &clear_color, 4 * sizeof(float));

        FrameRender(imgUiWindowPtr, device, vkQueue);
        FramePresent(imgUiWindowPtr, vkQueue);
    }

    spdlog::info("Shutting down Vulkan/GLFW render context");

    vkDestroySurfaceKHR(vkInstance, surface, nullptr);

    glfwDestroyWindow(window);

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vkInstance, nullptr);

    glfwTerminate();

    return videobackendResult::createSuccessful();
}

void glfwvulkan::init(int t_width, int t_height, const char *t_appName) {
    width = t_width;
    height = t_height;
    appName = t_appName;
}

void glfwvulkan::glfwResizeCallback(GLFWwindow *, int width, int height) {
    swapChainRebuild = true;
    swapChainResizeWidth = width;
    swapChainResizeHeight = height;
}

