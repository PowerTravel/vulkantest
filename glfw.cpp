#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
GLFWwindow* window;

void initWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void cleanup()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void mainLoop
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

int main(int argc, char* argv[])
{
    run();
    return 0;
}
