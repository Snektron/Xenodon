#include <iostream>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

template <typename F>
struct Defer {
    F f;

    Defer(F&& f):
        f(std::forward<F>(f)) {
    }

    ~Defer() {
        this->f();
    }
};

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    Defer _finalize_glfw([] {
        glfwTerminate();
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto* window = glfwCreateWindow(800, 600, "Vulkan test", nullptr, nullptr);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}
