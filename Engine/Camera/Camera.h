#pragma once

#include <glm/glm.hpp>

namespace TC
{
    enum class CameraMovement
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    class Camera
    {
    public:
        Camera();

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix(float aspectRatio) const;

        void ProcessKeyboard(CameraMovement direction, float deltaTime);
        void ProcessMouseMovement(float xOffset, float yOffset);
        void ProcessMouseScroll(float yOffset);

        void Reset();

        glm::vec3 GetPosition() const;
        float GetZoom() const;

    private:
        void UpdateCameraVectors();

        glm::vec3 m_Position;
        glm::vec3 m_Front;
        glm::vec3 m_Up;
        glm::vec3 m_Right;
        glm::vec3 m_WorldUp;

        float m_Yaw;
        float m_Pitch;

        float m_MovementSpeed = 4.0f;
        float m_MouseSensitivity = 0.1f;
        float m_Zoom = 45.0f;
    };
}