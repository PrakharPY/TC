#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main()
{
    if (!glfwInit())
    {
        std::cerr << "TC: Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "TC - Lightweight 3D Development Platform",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "TC: Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "TC: Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "TC initialized successfully!\n";
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}