#pragma once

#include "SceneObject.h"

#include <vector>

namespace TC
{
    class Scene
    {
    public:
        Scene();

        void AddObject(const SceneObject& object);

        const std::vector<SceneObject>& GetObjects() const;

    private:
        std::vector<SceneObject> m_Objects;
    };
}