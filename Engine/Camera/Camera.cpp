#include "Camera.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace TC
{
    Camera::Camera()
    {
        Reset();
    }

    void Camera::Reset()
    {
        // Initial camera position
        m_Position = glm::vec3(2.0f, 2.0f, 3.0f);

        // World up direction
        m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        // Initial camera orientation
        m_Yaw = -123.7f;
        m_Pitch = -29.0f;

        // Initial field of view
        m_Zoom = 45.0f;

        UpdateCameraVectors();
    }

    glm::mat4 Camera::GetViewMatrix() const
    {
        return glm::lookAt(
            m_Position,
            m_Position + m_Front,
            m_Up
        );
    }

    glm::mat4 Camera::GetProjectionMatrix(
        float aspectRatio) const
    {
        return glm::perspective(
            glm::radians(m_Zoom),
            aspectRatio,
            0.1f,
            100.0f
        );
    }

    void Camera::ProcessKeyboard(
        CameraMovement direction,
        float deltaTime)
    {
        float velocity = m_MovementSpeed * deltaTime;

        switch (direction)
        {
        case CameraMovement::Forward:
            m_Position += m_Front * velocity;
            break;

        case CameraMovement::Backward:
            m_Position -= m_Front * velocity;
            break;

        case CameraMovement::Left:
            m_Position -= m_Right * velocity;
            break;

        case CameraMovement::Right:
            m_Position += m_Right * velocity;
            break;

        case CameraMovement::Up:
            m_Position += m_WorldUp * velocity;
            break;

        case CameraMovement::Down:
            m_Position -= m_WorldUp * velocity;
            break;
        }
    }

    void Camera::ProcessMouseMovement(
        float xOffset,
        float yOffset)
    {
        m_Yaw += xOffset * m_MouseSensitivity;
        m_Pitch += yOffset * m_MouseSensitivity;

        // Prevent camera flipping
        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;

        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;

        UpdateCameraVectors();
    }

    void Camera::ProcessMouseScroll(float yOffset)
    {
        m_Zoom -= yOffset;

        if (m_Zoom < 15.0f)
            m_Zoom = 15.0f;

        if (m_Zoom > 90.0f)
            m_Zoom = 90.0f;
    }

    glm::vec3 Camera::GetPosition() const
    {
        return m_Position;
    }

    void Camera::UpdateCameraVectors()
    {
        glm::vec3 front;

        front.x =
            cos(glm::radians(m_Yaw)) *
            cos(glm::radians(m_Pitch));

        front.y =
            sin(glm::radians(m_Pitch));

        front.z =
            sin(glm::radians(m_Yaw)) *
            cos(glm::radians(m_Pitch));

        m_Front = glm::normalize(front);

        m_Right = glm::normalize(
            glm::cross(m_Front, m_WorldUp)
        );

        m_Up = glm::normalize(
            glm::cross(m_Right, m_Front)
        );
    }
}