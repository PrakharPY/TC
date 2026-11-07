#pragma once

struct GLFWwindow;

namespace TC
{
    class Window
    {
    public:
        Window();
        ~Window();

        bool Create(int width, int height, const char* title);

        bool ShouldClose() const;
        void PollEvents();
        void SwapBuffers();

        GLFWwindow* GetNativeWindow() const;

    private:
        GLFWwindow* m_Window = nullptr;
    };
}