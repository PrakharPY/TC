#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cmath>
#include <string>

namespace TC
{
    namespace
    {
        constexpr int GROUND_PLANE_INDEX = 0;
        constexpr float MIN_PICK_DIRECTION = 0.000001f;
    }

    Scene::Scene()
    {
        m_Objects.emplace_back(PrimitiveType::Plane, glm::vec3(0.0f),
            glm::vec3(0.0f), glm::vec3(20.0f, 1.0f, 20.0f),
            glm::vec3(0.32f, 0.34f, 0.38f), "Ground");

        m_Objects.emplace_back(PrimitiveType::Cube, glm::vec3(-1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f), glm::vec3(1.0f),
            glm::vec3(0.95f, 0.25f, 0.20f), "Red Cube");

        m_Objects.emplace_back(PrimitiveType::Cube, glm::vec3(1.5f, 0.5f, 0.0f),
            glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(1.0f),
            glm::vec3(0.20f, 0.55f, 0.95f), "Blue Cube");

        m_Objects.emplace_back(PrimitiveType::Cube, glm::vec3(0.0f, 0.5f, -2.0f),
            glm::vec3(0.0f), glm::vec3(0.65f),
            glm::vec3(0.25f, 0.85f, 0.40f), "Green Cube");
    }

    std::string Scene::MakeUniqueName(const std::string& baseName) const
    {
        auto exists = [this](const std::string& candidate)
        {
            for (const auto& object : m_Objects)
                if (object.GetName() == candidate)
                    return true;
            return false;
        };

        if (!exists(baseName))
            return baseName;

        for (int suffix = 2; suffix < 100000; ++suffix)
        {
            const std::string candidate = baseName + " " + std::to_string(suffix);
            if (!exists(candidate))
                return candidate;
        }
        return baseName + " New";
    }

    void Scene::AddObject(const SceneObject& object)
    {
        SceneObject copy = object;
        copy.SetName(MakeUniqueName(copy.GetName()));
        m_Objects.push_back(copy);
    }

    int Scene::CreateObject(PrimitiveType type, const glm::vec3& position)
    {
        const std::string base = type == PrimitiveType::Cube ? "Cube" : "Plane";
        if (type == PrimitiveType::Cube)
            m_Objects.emplace_back(type, position, glm::vec3(0.0f), glm::vec3(1.0f),
                glm::vec3(0.85f, 0.65f, 0.25f), MakeUniqueName(base));
        else
            m_Objects.emplace_back(type, position, glm::vec3(0.0f), glm::vec3(2.0f, 1.0f, 2.0f),
                glm::vec3(0.55f, 0.38f, 0.78f), MakeUniqueName(base));
        return static_cast<int>(m_Objects.size()) - 1;
    }

    int Scene::DuplicateObject(int index)
    {
        SceneObject* source = GetObject(index);
        if (!source || index == GROUND_PLANE_INDEX)
            return -1;

        SceneObject duplicate = *source;
        duplicate.SetPosition(source->GetPosition() + glm::vec3(1.0f, 0.0f, 1.0f));
        duplicate.SetName(MakeUniqueName(source->GetName() + " Copy"));
        m_Objects.push_back(duplicate);
        return static_cast<int>(m_Objects.size()) - 1;
    }

    void Scene::SetObjects(const std::vector<SceneObject>& objects)
    {
        if (!objects.empty())
            m_Objects = objects;
    }

    bool Scene::RemoveObject(int index)
    {
        if (index <= GROUND_PLANE_INDEX || index >= static_cast<int>(m_Objects.size()))
            return false;
        m_Objects.erase(m_Objects.begin() + index);
        return true;
    }

    const std::vector<SceneObject>& Scene::GetObjects() const { return m_Objects; }

    SceneObject* Scene::GetObject(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_Objects.size()))
            return nullptr;
        return &m_Objects[index];
    }

    int Scene::PickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const
    {
        float closestDistance = std::numeric_limits<float>::max();
        int selectedIndex = -1;

        for (int i = 0; i < static_cast<int>(m_Objects.size()); ++i)
        {
            if (i == GROUND_PLANE_INDEX)
                continue;

            const SceneObject& object = m_Objects[i];
            glm::mat4 inverseModel = glm::inverse(object.GetModelMatrix());
            glm::vec3 localOrigin = glm::vec3(inverseModel * glm::vec4(rayOrigin, 1.0f));
            glm::vec3 localDirection = glm::vec3(inverseModel * glm::vec4(rayDirection, 0.0f));
            float distance = std::numeric_limits<float>::max();
            bool hit = false;

            if (object.GetType() == PrimitiveType::Cube)
            {
                float tMin = 0.0f;
                float tMax = std::numeric_limits<float>::max();
                hit = true;
                for (int axis = 0; axis < 3; ++axis)
                {
                    float origin = localOrigin[axis];
                    float direction = localDirection[axis];
                    if (std::abs(direction) < MIN_PICK_DIRECTION)
                    {
                        if (origin < -0.5f || origin > 0.5f) { hit = false; break; }
                        continue;
                    }
                    float t1 = (-0.5f - origin) / direction;
                    float t2 = (0.5f - origin) / direction;
                    if (t1 > t2) std::swap(t1, t2);
                    tMin = glm::max(tMin, t1);
                    tMax = glm::min(tMax, t2);
                    if (tMin > tMax) { hit = false; break; }
                }
                if (hit && tMax >= 0.0f)
                    distance = tMin >= 0.0f ? tMin : tMax;
                else
                    hit = false;
            }
            else if (object.GetType() == PrimitiveType::Plane &&
                std::abs(localDirection.y) >= MIN_PICK_DIRECTION)
            {
                float t = -localOrigin.y / localDirection.y;
                if (t >= 0.0f)
                {
                    glm::vec3 localHit = localOrigin + localDirection * t;
                    if (std::abs(localHit.x) <= 0.5f && std::abs(localHit.z) <= 0.5f)
                    {
                        hit = true;
                        distance = t;
                    }
                }
            }

            if (hit && distance < closestDistance)
            {
                closestDistance = distance;
                selectedIndex = i;
            }
        }
        return selectedIndex;
    }
}
