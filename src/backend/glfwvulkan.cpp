#define GLFW_INCLUDE_VULKAN
#define IMGUI_UNLIMITED_FRAME_RATE

#include "glfwvulkan.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <valarray>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "../../vendor/imgui-filebrowser/imfilebrowser.h"
#include <glm/glm.hpp>

// needs to live outside the class because of
// https://stackoverflow.com/questions/7852101/c-lambda-with-captures-as-a-function-pointer
static void imgUiCheckError(VkResult vkResult) {
    if (vkResult == 0) {
        return;
    }

    spdlog::error("Error returned from imgui: VkResult: {}", vkResult);
}

videobackendResult *glfwvulkan::run(class system *emulatedSystem) {
    emulatedSystemPtr = emulatedSystem;

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

    vkResult = initializeVulkan();

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to create Vulkan instance", vkResult);
    }

    uint32_t gpuCount;
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to enumerate Vulkan physical devices", vkResult);
    }

    std::vector <VkPhysicalDevice> gpus(gpuCount);
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &gpuCount, gpus.data());

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to enumerate Vulkan physical devices", vkResult);
    }

    vkPhysicalDevice = gpus[0];

    initializeQueueFamily();

    if (graphicsQueueFamily == UINT32_MAX) {
        glfwTerminate();
        return videobackendResult::createWithError("Non-supported graphics Vulkan family vkDevice found");
    }

    initializeDeviceQueue(gpus);

    vkResult = initializeDescriptorPool();

    if (vkResult < 0) {
        glfwTerminate();
        return videobackendResult::createWithError("Unable to create Vulkan descriptor pool", vkResult);
    }

    glfwWindowHint(
            GLFW_CLIENT_API,
            GLFW_NO_API
    );

    auto window = glfwCreateWindow(windowWidth, windowHeight, appName, nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);

    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int h, int w) {
        auto &self = *static_cast<glfwvulkan *>(glfwGetWindowUserPointer(window));
        self.glfwResizeCallback(window, h, w);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto &self = *static_cast<glfwvulkan *>(glfwGetWindowUserPointer(window));

        if (self.emulationFocus == false)
        {
            return;
        }

        if(self.GLFW_KEYMAP.find(key) != self.GLFW_KEYMAP.end())
        {
            auto mappedKey = self.GLFW_KEYMAP[key];

            int value;

            if (action == GLFW_PRESS || action == GLFW_REPEAT)
            {
                value = 1;
            } else {
                value = 0;
            }

            self.emulatedSystemPtr->keyPressed(mappedKey, value);
        }
    });

    vkResult = glfwCreateWindowSurface(vkInstance, window, vkAllocator, &vkSurfaceKhr);

    if (vkResult < 0) {
        glfwTerminate();
        return videobackendResult::createWithError("Unable to create GLFW vulkan window vkSurfaceKhr", vkResult);
    }

    imgUiWindowPtr->Surface = vkSurfaceKhr;

    VkBool32 vkBoolResult;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, graphicsQueueFamily, imgUiWindowPtr->Surface, &vkBoolResult);

    if (vkBoolResult != VK_TRUE) {
        glfwTerminate();
        return videobackendResult::createWithError("Error no WSI support on physical vkDevice 0");
    }

    glfwSwapInterval(0);

    const VkFormat requestSurfaceImageFormat[] = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8_UNORM,
            VK_FORMAT_R8G8B8_UNORM
    };

    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    imgUiWindowPtr->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
            vkPhysicalDevice,
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
            vkPhysicalDevice,
            imgUiWindowPtr->Surface,
            &presentModes[0],
            IM_ARRAYSIZE(presentModes)
    );

    IM_ASSERT(MIN_IMAGE_COUNT >= 2);

    ImGui_ImplVulkanH_CreateWindow(
            vkInstance,
            vkPhysicalDevice,
            vkDevice,
            imgUiWindowPtr,
            graphicsQueueFamily,
            vkAllocator,
            windowWidth,
            windowHeight,
            MIN_IMAGE_COUNT
    );


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    initializeImGui(window);

    ImFont *result = loadImGuiJapaneseFont(io);

    if (result == nullptr) {
        return videobackendResult::createWithError("Unable to load Japanese font set");
    }

    imgUiUploadFonts();

    spdlog::info("Vulkan/GLFW initialized, starting render loop");

    videobackendResult *vbResult = nullptr;
    ImTextureID imgUiTexture = nullptr;

    auto createPixelBufferResult = createEmulationPixelBuffer(emulatedSystem->renderWidth(),
                                                              emulatedSystem->renderHeight());
    if (!createPixelBufferResult) {
        return videobackendResult::createWithError("Unable to create pixel buffer data");
    }

    auto createPixelImageResult = createEmulationPixelImage(emulatedSystem->renderWidth(),
                                                            emulatedSystem->renderHeight());

    if (!createPixelImageResult) {
        return videobackendResult::createWithError("Unable to create pixel image");
    }

    ImGui::FileBrowser fileDialog;

    fileDialog.SetTitle("Pick a rom");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (swapChainRebuild) {
            swapChainRebuild = false;
            ImGui_ImplVulkan_SetMinImageCount(MIN_IMAGE_COUNT);
            ImGui_ImplVulkanH_CreateWindow(
                    vkInstance,
                    vkPhysicalDevice,
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

        ImGui::Begin(u8"チップ「８」", nullptr, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Rom")) {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) {
                    fileDialog.Open();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        fileDialog.Display();

        if (fileDialog.HasSelected()) {
            spdlog::info("Opening rom");
            emulatedSystem->loadRom(fileDialog.GetSelected().c_str());
            emulatedSystem->start();

            imgUiTexture = ImGui_ImplVulkan_AddTexture(emulationPixelImageSampler,
                                                       emulationPixelImageView,
                                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            fileDialog.ClearSelected();
        }

        ImGui::Begin("Emulation");
        {
            auto currentWidth = (unsigned short) ImGui::GetWindowWidth();
            auto currentHeight = (unsigned short) ImGui::GetWindowHeight();

            if (currentWidth != emulationWindowWidth or currentHeight != emulationWindowHeight) {
                emulationWindowWidth = currentWidth;
                emulationWindowHeight = currentHeight;

                createEmulationPixelScaledImage(currentWidth, currentHeight);
                registerImageBufferCommands(emulatedSystem->renderWidth(), emulatedSystem->renderHeight());
            }

            emulationFocus = ImGui::IsWindowFocused();

            if (emulationFocus && emulatedSystem->step()) {
                auto pixels = emulatedSystem->pixels();

                memset(emulationPixelBufferData, 0, emulationPixelBufferSize);

                for (int y = 0; y < emulatedSystem->renderHeight(); ++y) {
                    for (int x = 0; x < emulatedSystem->renderWidth(); ++x) {
                        if (pixels[(y * emulatedSystem->renderWidth()) + x]) {
                            auto *pixel = static_cast<int *>(emulationPixelBufferData)
                                          + (y * emulatedSystem->renderWidth()) + x;
                            *pixel = 0xFFFFFFFF;
                        }
                    }
                }
            }

            if (imgUiTexture != nullptr) {
                ImGui::Image(
                        imgUiTexture,
                        ImVec2(
                                (float) (currentWidth - EMULATION_WINDOW_PADDING + 10),
                                (float) (currentHeight - EMULATION_WINDOW_PADDING - 12)
                        )
                );
            }
        }
        ImGui::End();
        ImGui::Render();


        memcpy(&imgUiWindowPtr->ClearValue.color.float32[0], &CLEAR_COLOR, 4 * sizeof(float));

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

ImFont *glfwvulkan::loadImGuiJapaneseFont(ImGuiIO &io) {
    auto result = io.Fonts->AddFontFromFileTTF(
            "resources/NotoSansCJKjp-Medium.otf",
            20.0f,
            nullptr,
            io.Fonts->GetGlyphRangesJapanese()
    );
    return result;
}

void glfwvulkan::initializeImGui(GLFWwindow *window) {
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo imgUiInitInfo = {};
    imgUiInitInfo.Instance = vkInstance;
    imgUiInitInfo.PhysicalDevice = vkPhysicalDevice;
    imgUiInitInfo.Device = vkDevice;
    imgUiInitInfo.QueueFamily = graphicsQueueFamily;
    imgUiInitInfo.Queue = vkQueue;
    imgUiInitInfo.PipelineCache = vkPipelineCache;
    imgUiInitInfo.DescriptorPool = vkDescriptorPool;
    imgUiInitInfo.Allocator = vkAllocator;
    imgUiInitInfo.MinImageCount = MIN_IMAGE_COUNT;
    imgUiInitInfo.ImageCount = imgUiWindowPtr->ImageCount;
    imgUiInitInfo.CheckVkResultFn = imgUiCheckError;

    ImGui_ImplVulkan_Init(&imgUiInitInfo, imgUiWindowPtr->RenderPass);
}

VkResult glfwvulkan::initializeDescriptorPool() {
    VkResult vkResult;
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
    return vkResult;
}

void glfwvulkan::initializeQueueFamily() {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

    std::__1::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, familyProperties.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
        }
    }
}

VkResult glfwvulkan::initializeVulkan() {
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
    return vkResult;
}

void glfwvulkan::initializeDeviceQueue(const std::vector<VkPhysicalDevice> &gpus) {
    VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    deviceQueueCreateInfo.pQueuePriorities = VULKAN_QUEUE_PRIORITIES;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();

    vkCreateDevice(gpus[0], &deviceCreateInfo, vkAllocator, &vkDevice);
    vkGetDeviceQueue(vkDevice, graphicsQueueFamily, 0, &vkQueue);
}

void glfwvulkan::createPipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage,
        std::vector<VkMemoryBarrier> memoryBarries,
        std::vector<VkBufferMemoryBarrier> bufferBarries,
        std::vector<VkImageMemoryBarrier> imageBarriers
) {
    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            dstStage,
            0,
            static_cast<unsigned int>(memoryBarries.size()),
            memoryBarries.data(),
            static_cast<unsigned int>(bufferBarries.size()),
            bufferBarries.data(),
            static_cast<unsigned int>(imageBarriers.size()),
            imageBarriers.data()
    );
}

videobackendResult *glfwvulkan::initializeCommandBuffer(VkCommandBuffer &vkCommandBuffer) {
    VkResult vkResult;
    VkCommandPool vkCommandPool;

    VkCommandPoolCreateInfo vkCommandPoolCreateInfo;
    vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkCommandPoolCreateInfo.pNext = nullptr;
    vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCommandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    vkResult = vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, nullptr, &vkCommandPool);

    if (vkResult != VK_SUCCESS) {
        return videobackendResult::createWithError("Unable to create command pool", vkResult);
    }

    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo;
    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkCommandBufferAllocateInfo.pNext = nullptr;
    vkCommandBufferAllocateInfo.commandPool = vkCommandPool;
    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkCommandBufferAllocateInfo.commandBufferCount = 1;

    vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer);

    if (vkResult != VK_SUCCESS) {
        return videobackendResult::createWithError("Unable to allocate command buffer", vkResult);
    }

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    vkResult = vkBeginCommandBuffer(vkCommandBuffer, &begin_info);

    if (vkResult != VK_SUCCESS) {
        return videobackendResult::createWithError("Unable to being command buffer", vkResult);
    }

    return videobackendResult::createSuccessful();
}

videobackendResult *glfwvulkan::registerImageBufferCommands(unsigned short width_t, unsigned short height_t) {
    VkResult vkResult;

    initializeCommandBuffer(emulationCommandBuffer);

    VkImageSubresourceRange vkImageSubresourceRange;
    vkImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkImageSubresourceRange.baseMipLevel = 0;
    vkImageSubresourceRange.levelCount = 1;
    vkImageSubresourceRange.baseArrayLayer = 0;
    vkImageSubresourceRange.layerCount = 1;

    VkImageSubresourceLayers vkImageSubresourceLayers;
    vkImageSubresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkImageSubresourceLayers.mipLevel = 0;
    vkImageSubresourceLayers.baseArrayLayer = 0;
    vkImageSubresourceLayers.layerCount = 1;

    VkBufferMemoryBarrier vkBufferMemoryBarrier;
    vkBufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vkBufferMemoryBarrier.pNext = nullptr;
    vkBufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    vkBufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkBufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBufferMemoryBarrier.buffer = emulationPixelBuffer;
    vkBufferMemoryBarrier.offset = 0;
    vkBufferMemoryBarrier.size = emulationPixelBufferSize;

    VkImageMemoryBarrier vkImageMemoryBarrierScaledPixelImage;
    vkImageMemoryBarrierScaledPixelImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkImageMemoryBarrierScaledPixelImage.pNext = nullptr;
    vkImageMemoryBarrierScaledPixelImage.srcAccessMask = 0;
    vkImageMemoryBarrierScaledPixelImage.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkImageMemoryBarrierScaledPixelImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkImageMemoryBarrierScaledPixelImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkImageMemoryBarrierScaledPixelImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierScaledPixelImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierScaledPixelImage.image = emulationScaledPixelImage;
    vkImageMemoryBarrierScaledPixelImage.subresourceRange = vkImageSubresourceRange;

    VkImageMemoryBarrier vkImageMemoryBarrierPixelImage;
    vkImageMemoryBarrierPixelImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkImageMemoryBarrierPixelImage.pNext = nullptr;
    vkImageMemoryBarrierPixelImage.srcAccessMask = 0;
    vkImageMemoryBarrierPixelImage.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkImageMemoryBarrierPixelImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkImageMemoryBarrierPixelImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkImageMemoryBarrierPixelImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierPixelImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierPixelImage.image = emulationPixelImage;
    vkImageMemoryBarrierPixelImage.subresourceRange = vkImageSubresourceRange;

    createPipelineBarrier(
            emulationCommandBuffer,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {},
            {vkBufferMemoryBarrier},
            {
                    vkImageMemoryBarrierScaledPixelImage,
                    vkImageMemoryBarrierPixelImage
            }
    );

    VkBufferImageCopy vkBufferImageCopy;
    vkBufferImageCopy.bufferOffset = 0;
    vkBufferImageCopy.bufferRowLength = 0;
    vkBufferImageCopy.bufferImageHeight = 0;
    vkBufferImageCopy.imageSubresource = vkImageSubresourceLayers;
    vkBufferImageCopy.imageOffset = {};
    vkBufferImageCopy.imageExtent = {width_t, height_t, 1};

    vkCmdCopyBufferToImage(
            emulationCommandBuffer,
            emulationPixelBuffer,
            emulationPixelImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &vkBufferImageCopy
    );

    VkImageMemoryBarrier vkImageMemoryBarrierBlitPixelImage;
    vkImageMemoryBarrierBlitPixelImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkImageMemoryBarrierBlitPixelImage.pNext = nullptr;
    vkImageMemoryBarrierBlitPixelImage.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkImageMemoryBarrierBlitPixelImage.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkImageMemoryBarrierBlitPixelImage.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkImageMemoryBarrierBlitPixelImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    vkImageMemoryBarrierBlitPixelImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierBlitPixelImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkImageMemoryBarrierBlitPixelImage.image = emulationPixelImage;
    vkImageMemoryBarrierBlitPixelImage.subresourceRange = vkImageSubresourceRange;

    createPipelineBarrier(
            emulationCommandBuffer,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {},
            {},
            {vkImageMemoryBarrierBlitPixelImage}
    );

    VkImageBlit vkImageBlit;
    vkImageBlit.srcSubresource = vkImageSubresourceLayers;
    vkImageBlit.srcOffsets[0] = {};
    vkImageBlit.srcOffsets[1] = {width_t, height_t, 1};
    vkImageBlit.dstSubresource = vkImageSubresourceLayers;
    vkImageBlit.dstOffsets[0] = {};
    vkImageBlit.dstOffsets[1] = {emulationWindowWidth, emulationWindowHeight, 1};

    vkCmdBlitImage(
            emulationCommandBuffer,
            emulationPixelImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            emulationScaledPixelImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &vkImageBlit,
            VK_FILTER_NEAREST
    );

    vkResult = vkEndCommandBuffer(emulationCommandBuffer);

    if (vkResult != VK_SUCCESS) {
        return videobackendResult::createWithError("Unable to finish command buffer", vkResult);
    }

    return videobackendResult::createSuccessful();
}

void glfwvulkan::init(int width, int height, const char *appName_t) {
    windowWidth = width;
    windowHeight = height;
    appName = appName_t;
}

void glfwvulkan::glfwResizeCallback(GLFWwindow *, int width, int height) {
    swapChainRebuild = true;
    swapChainResizeWidth = width;
    swapChainResizeHeight = height;
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
            return videobackendResult::createWithError("Error resetting fence", vkResult);
        }
    }

    VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vkImageSemaphore;
    submit_info.pWaitDstStageMask = &stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &emulationCommandBuffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    vkResult = vkQueueSubmit(vkQueue, 1, &submit_info, fd->Fence);

    if (vkResult < 0) {
        return videobackendResult::createWithError("Unable to acquire next image", vkResult);
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

bool glfwvulkan::createEmulationPixelBuffer(unsigned short width_t, unsigned short height_t) {
    emulationPixelBufferSize = width_t * height_t * 4;

    VkBufferCreateInfo vkBufferCreateInfo;
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.pNext = nullptr;
    vkBufferCreateInfo.flags = 0;
    vkBufferCreateInfo.size = emulationPixelBufferSize;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.queueFamilyIndexCount = 0;
    vkBufferCreateInfo.pQueueFamilyIndices = nullptr;

    if (vkCreateBuffer(vkDevice, &vkBufferCreateInfo, nullptr, &emulationPixelBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, emulationPixelBuffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo vkMemoryAllocateInfo;
    vkMemoryAllocateInfo.pNext = nullptr;
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = findMemoryType(vkMemoryRequirements.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

    if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &emulationPixelMemoryBuffer) != VK_SUCCESS) {
        return false;
    }

    if (vkBindBufferMemory(vkDevice, emulationPixelBuffer, emulationPixelMemoryBuffer, 0) != VK_SUCCESS) {
        return false;
    }

    return !(vkMapMemory(vkDevice, emulationPixelMemoryBuffer, 0, vkMemoryRequirements.size, 0,
                         &emulationPixelBufferData) != VK_SUCCESS);

}

bool glfwvulkan::createImage(unsigned short width_t, unsigned short height_t) {
    VkImageCreateInfo vkImageCreateInfo;
    vkImageCreateInfo.pNext = nullptr;
    vkImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkImageCreateInfo.flags = 0;
    vkImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    vkImageCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    vkImageCreateInfo.extent = {width_t, height_t, 1};
    vkImageCreateInfo.mipLevels = 1;
    vkImageCreateInfo.arrayLayers = 1;
    vkImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkImageCreateInfo.queueFamilyIndexCount = 0;
    vkImageCreateInfo.pQueueFamilyIndices = nullptr;
    vkImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(vkDevice, &vkImageCreateInfo, nullptr, &emulationPixelImage);

    if (result < 0) {
        spdlog::error("failed to create image: {}", result);
        throw std::runtime_error("ERROR!");
        return false;
    }

    return true;
}

bool glfwvulkan::createEmulationPixelScaledImage(unsigned short width_t, unsigned short height_t) {
    spdlog::info("Creating new emulated scaled image: {}x{}", width_t, height_t);

    VkImageCreateInfo vkImageCreateInfo;
    vkImageCreateInfo.pNext = nullptr;
    vkImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkImageCreateInfo.flags = 0;
    vkImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    vkImageCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    vkImageCreateInfo.extent = {width_t, height_t, 1};
    vkImageCreateInfo.mipLevels = 1;
    vkImageCreateInfo.arrayLayers = 1;
    vkImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkImageCreateInfo.queueFamilyIndexCount = 0;
    vkImageCreateInfo.pQueueFamilyIndices = nullptr;
    vkImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(vkDevice, &vkImageCreateInfo, nullptr, &emulationScaledPixelImage);

    if (result < 0) {
        spdlog::error("failed to create image: {}", result);
        throw std::runtime_error("ERROR!");
        return false;
    }

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, emulationPixelBuffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo vkMemoryAllocateInfo;
    vkMemoryAllocateInfo.pNext = nullptr;
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = findMemoryType(vkMemoryRequirements.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &emulationPixelScaledMemoryBuffer) != VK_SUCCESS) {
        return false;
    }

    if (vkBindImageMemory(vkDevice, emulationScaledPixelImage, emulationPixelScaledMemoryBuffer, 0) != VK_SUCCESS) {
        return false;
    }

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = emulationScaledPixelImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vkDevice, &createInfo, nullptr, &emulationPixelImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
    }

    return true;
}

bool glfwvulkan::createEmulationPixelImage(unsigned short width_t, unsigned short height_t) {
    createImage(width_t, height_t);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, emulationPixelBuffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo vkMemoryAllocateInfo;
    vkMemoryAllocateInfo.pNext = nullptr;
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = findMemoryType(vkMemoryRequirements.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &emulationPixelMemoryBuffer) != VK_SUCCESS) {
        return false;
    }

    if (vkBindImageMemory(vkDevice, emulationPixelImage, emulationPixelMemoryBuffer, 0) != VK_SUCCESS) {
        return false;
    }

    VkSamplerCreateInfo vkSamplerCreateInfo{};
    vkSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    vkSamplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    vkSamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    vkSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkSamplerCreateInfo.anisotropyEnable = VK_FALSE;
    vkSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    vkSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    vkSamplerCreateInfo.compareEnable = VK_FALSE;
    vkSamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    vkSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    if (vkCreateSampler(vkDevice, &vkSamplerCreateInfo, nullptr, &emulationPixelImageSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    return true;
}

uint32_t
glfwvulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
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

