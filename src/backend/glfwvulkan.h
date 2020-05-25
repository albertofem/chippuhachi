#ifndef CHIPPUHACHI_GLFWVULKAN_H
#define CHIPPUHACHI_GLFWVULKAN_H


#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "videobackend.h"

class glfwvulkan : public videobackend {
    const int MIN_IMAGE_COUNT = 2;

    VkInstance vkInstance = VK_NULL_HANDLE;

    int width;
    int height;

    const char* appName;

    bool swapChainRebuild = false;
    int swapChainResizeWidth = 0;
    int swapChainResizeHeight = 0;
public:
    videobackendResult* run() override;

    void init(int width, int height, const char *t_appName) override;

    void glfwResizeCallback(GLFWwindow *, int width, int height);
};


#endif