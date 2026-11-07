#include "Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

namespace TC
{
    Window::Window() = default;

    Window::~Window()
    {
        if (m_Window)
            glfwDestroyWindow(m_Window);

        glfwTerminate();
    }

    bool Window::Create(int width, int height, const char* title)
    {
        if (!glfwInit())
        {
            std::cerr << "TC: GLFW initialization failed!\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,
                       GLFW_OPENGL_CORE_PROFILE);

        m_Window = glfwCreateWindow(
            width, height, title, nullptr, nullptr
        );

        if (!m_Window)
        {
            std::cerr << "TC: Window creation failed!\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_Window);

        if (!gladLoadGLLoader(
            (GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "TC: GLAD initialization failed!\n";

            glfwDestroyWindow(m_Window);
            m_Window = nullptr;

            glfwTerminate();
            return false;
        }

        glfwSwapInterval(1);

        glViewport(0, 0, width, height);

        glfwSetFramebufferSizeCallback(
            m_Window,
            [](GLFWwindow*, int w, int h)
            {
                glViewport(0, 0, w, h);
            }
        );

        std::cout << "TC: Window initialized!\n";
        std::cout << "OpenGL: "
                  << glGetString(GL_VERSION) << "\n";

        return true;
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
    }

    GLFWwindow* Window::GetNativeWindow() const
    {
        return m_Window;
    }
}