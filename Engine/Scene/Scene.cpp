#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <limits>
#include <cmath>

namespace TC
{
    Scene::Scene()
    {
        // Ground plane
        m_Objects.emplace_back(
            PrimitiveType::Plane,
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            glm::vec3(20.0f, 1.0f, 20.0f),
            glm::vec3(0.32f, 0.34f, 0.38f)
        );

        // Red cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(-1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f),
            glm::vec3(0.95f, 0.25f, 0.20f)
        );

        // Blue cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f, 30.0f, 0.0f),
            glm::vec3(1.0f),
            glm::vec3(0.20f, 0.55f, 0.95f)
        );

        // Green cube
        m_Objects.emplace_back(
            PrimitiveType::Cube,
            glm::vec3(0.0f, 0.5f, -2.0f),
            glm::vec3(0.0f),
            glm::vec3(0.65f),
            glm::vec3(0.25f, 0.85f, 0.40f)
        );
    }

    void Scene::AddObject(const SceneObject& object)
    {
        m_Objects.push_back(object);
    }

    const std::vector<SceneObject>& Scene::GetObjects() const
    {
        return m_Objects;
    }

    SceneObject* Scene::GetObject(int index)
    {
        if (index < 0 ||
            index >= static_cast<int>(m_Objects.size()))
        {
            return nullptr;
        }

        return &m_Objects[index];
    }

    int Scene::PickObject(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection
    ) const
    {
        float closestDistance =
            std::numeric_limits<float>::max();

        int selectedIndex = -1;

        for (int i = 0; i < static_cast<int>(m_Objects.size()); ++i)
        {
            const SceneObject& object = m_Objects[i];

            if (object.GetType() != PrimitiveType::Cube)
                continue;

            glm::mat4 inverseModel =
                glm::inverse(object.GetModelMatrix());

            glm::vec3 localOrigin =
                glm::vec3(inverseModel * glm::vec4(rayOrigin, 1.0f));

            glm::vec3 localDirection =
                glm::vec3(inverseModel * glm::vec4(rayDirection, 0.0f));

            float tMin = 0.0f;
            float tMax = std::numeric_limits<float>::max();

            bool hit = true;

            for (int axis = 0; axis < 3; ++axis)
            {
                float origin = localOrigin[axis];
                float direction = localDirection[axis];

                if (std::abs(direction) < 0.000001f)
                {
                    if (origin < -0.5f || origin > 0.5f)
                    {
                        hit = false;
                        break;
                    }

                    continue;
                }

                float t1 = (-0.5f - origin) / direction;
                float t2 = (0.5f - origin) / direction;

                if (t1 > t2)
                {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                tMin = glm::max(tMin, t1);
                tMax = glm::min(tMax, t2);

                if (tMin > tMax)
                {
                    hit = false;
                    break;
                }
            }

            if (hit && tMax >= 0.0f)
            {
                float distance = tMin >= 0.0f ? tMin : tMax;

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    selectedIndex = i;
                }
            }
        }

        return selectedIndex;
    }
}