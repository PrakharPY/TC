#include "TransformGizmo.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <vector>

namespace TC
{
namespace
{
constexpr float GIZMO_LENGTH = 1.3f;
constexpr float GRID_SNAP_SIZE = 0.5f;
constexpr float ROTATION_SNAP_DEGREES = 15.0f;
constexpr float SCALE_SNAP_SIZE = 0.1f;
constexpr int RING_SEGMENTS = 96;

bool ProjectToScreen(const glm::vec3& p, const glm::mat4& view,
    const glm::mat4& projection, int width, int height, glm::vec2& screen)
{
    glm::vec4 clip = projection * view * glm::vec4(p, 1.0f);
    if (clip.w <= 0.0001f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    screen = glm::vec2((ndc.x * 0.5f + 0.5f) * width,
        (1.0f - (ndc.y * 0.5f + 0.5f)) * height);
    return true;
}

float DistanceToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b)
{
    glm::vec2 ab = b - a;
    float len2 = glm::dot(ab, ab);
    if (len2 < 0.0001f) return glm::length(p - a);
    float t = glm::clamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f);
    return glm::length(p - (a + t * ab));
}

glm::vec3 AxisDirection(GizmoAxis axis)
{
    if (axis == GizmoAxis::X) return {1, 0, 0};
    if (axis == GizmoAxis::Y) return {0, 1, 0};
    if (axis == GizmoAxis::Z) return {0, 0, 1};
    return {0, 0, 0};
}

glm::vec3 ScreenPointToRay(const Camera& camera, float x, float y, int width, int height)
{
    float nx = 2.0f * x / width - 1.0f;
    float ny = 1.0f - 2.0f * y / height;
    glm::vec4 clip(nx, ny, -1.0f, 1.0f);
    glm::mat4 projection = camera.GetProjectionMatrix(float(width) / float(height));
    glm::vec4 eye = glm::inverse(projection) * clip;
    eye.z = -1.0f; eye.w = 0.0f;
    return glm::normalize(glm::vec3(glm::inverse(camera.GetViewMatrix()) * eye));
}

bool RayPlaneHit(const Camera& camera, const glm::vec3& origin, const glm::vec3& normal,
    float x, float y, int width, int height, glm::vec3& hit)
{
    glm::vec3 ray = ScreenPointToRay(camera, x, y, width, height);
    float denom = glm::dot(ray, normal);
    if (std::abs(denom) < 0.0001f) return false;
    float t = glm::dot(origin - camera.GetPosition(), normal) / denom;
    if (t <= 0.0f) return false;
    hit = camera.GetPosition() + t * ray;
    return true;
}
}

float TransformGizmo::CalculateGizmoScale(const Camera& camera, const glm::vec3& position) const
{
    float distance = glm::length(camera.GetPosition() - position);
    float halfFov = glm::radians(camera.GetZoom()) * 0.5f;
    float desired = 2.0f * distance * std::tan(halfFov) * 0.35f;
    return glm::clamp(desired / GIZMO_LENGTH, 0.05f, 100.0f);
}

bool TransformGizmo::Initialize()
{
    if (!m_Shader.LoadFromFiles("Assets/Shaders/basic.vert", "Assets/Shaders/basic.frag"))
    {
        std::cerr << "Failed to load gizmo shaders!\n";
        return false;
    }
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    std::cout << "TC: Transform gizmo initialized!\n";
    return true;
}

bool TransformGizmo::GetAxisParameter(const Camera& camera, const glm::vec3& origin,
    GizmoAxis axis, float mouseX, float mouseY, int width, int height, float& parameter) const
{
    if (width <= 0 || height <= 0 || axis == GizmoAxis::None || axis == GizmoAxis::Uniform) return false;
    glm::vec3 u = AxisDirection(axis), v = ScreenPointToRay(camera, mouseX, mouseY, width, height);
    glm::vec3 w = origin - camera.GetPosition();
    float a = glm::dot(u, u), b = glm::dot(u, v), c = glm::dot(v, v);
    float d = glm::dot(u, w), e = glm::dot(v, w);
    float denominator = a * c - b * b;
    if (std::abs(denominator) < 0.0001f) return false;
    parameter = (b * e - c * d) / denominator;
    return true;
}

bool TransformGizmo::GetRotationVector(const Camera& camera, const glm::vec3& origin,
    GizmoAxis axis, float mouseX, float mouseY, int width, int height, glm::vec3& vector) const
{
    glm::vec3 hit;
    if (width <= 0 || height <= 0 || !RayPlaneHit(camera, origin, AxisDirection(axis),
        mouseX, mouseY, width, height, hit)) return false;
    glm::vec3 offset = hit - origin;
    if (glm::length(offset) < 0.0001f) return false;
    vector = glm::normalize(offset);
    return true;
}

void TransformGizmo::UpdateHover(const Camera& camera, const glm::vec3& position,
    float mouseX, float mouseY, int width, int height)
{
    if (m_IsDragging) return;
    m_HoveredAxis = GizmoAxis::None;
    if (width <= 0 || height <= 0) return;
    float scale = CalculateGizmoScale(camera, position);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix(float(width) / float(height));
    glm::vec2 mouse(mouseX, mouseY);
    const GizmoAxis axes[] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
    float closest = 12.0f;

    if (m_Mode == GizmoMode::Translate || m_Mode == GizmoMode::Scale)
    {
        glm::vec2 center;
        if (!ProjectToScreen(position, view, projection, width, height, center)) return;
        if (m_Mode == GizmoMode::Scale && glm::length(mouse - center) <= 11.0f)
        {
            m_HoveredAxis = GizmoAxis::Uniform;
            return;
        }
        for (GizmoAxis axis : axes)
        {
            glm::vec2 a, b;
            if (!ProjectToScreen(position, view, projection, width, height, a) ||
                !ProjectToScreen(position + AxisDirection(axis) * GIZMO_LENGTH * scale,
                    view, projection, width, height, b)) continue;
            float d = DistanceToSegment(mouse, a, b);
            if (d < closest) { closest = d; m_HoveredAxis = axis; }
        }
    }
    else
    {
        for (GizmoAxis axis : axes)
        {
            glm::vec3 previous3 = axis == GizmoAxis::X
                ? position + glm::vec3(0, GIZMO_LENGTH * scale, 0)
                : position + glm::vec3(GIZMO_LENGTH * scale, 0, 0);
            glm::vec2 previous;
            if (!ProjectToScreen(previous3, view, projection, width, height, previous)) continue;
            for (int i = 1; i <= RING_SEGMENTS; ++i)
            {
                float angle = 6.28318530718f * float(i) / float(RING_SEGMENTS);
                float r = GIZMO_LENGTH * scale;
                glm::vec3 point;
                if (axis == GizmoAxis::X) point = position + glm::vec3(0, std::cos(angle) * r, std::sin(angle) * r);
                else if (axis == GizmoAxis::Y) point = position + glm::vec3(std::cos(angle) * r, 0, std::sin(angle) * r);
                else point = position + glm::vec3(std::cos(angle) * r, std::sin(angle) * r, 0);
                glm::vec2 current;
                if (ProjectToScreen(point, view, projection, width, height, current))
                {
                    float d = DistanceToSegment(mouse, previous, current);
                    if (d < closest) { closest = d; m_HoveredAxis = axis; }
                    previous = current;
                }
            }
        }
    }
}

bool TransformGizmo::BeginDrag(const Camera& camera, const glm::vec3& position,
    float mouseX, float mouseY, int width, int height)
{
    if (m_HoveredAxis == GizmoAxis::None) return false;
    m_DragOrigin = position;
    if (m_Mode == GizmoMode::Translate || (m_Mode == GizmoMode::Scale && m_HoveredAxis != GizmoAxis::Uniform))
    {
        float parameter = 0.0f;
        if (!GetAxisParameter(camera, position, m_HoveredAxis, mouseX, mouseY, width, height, parameter)) return false;
        m_LastDragParameter = parameter;
        m_LastAppliedMovement = 0.0f;
        m_LastAppliedScale = 0.0f;
    }
    else if (m_Mode == GizmoMode::Rotate)
    {
        if (!GetRotationVector(camera, position, m_HoveredAxis, mouseX, mouseY, width, height, m_InitialRotationVector)) return false;
        m_LastAppliedRotation = 0.0f;
    }
    else
    {
        m_InitialMouseY = mouseY;
        m_LastAppliedScale = 0.0f;
    }
    m_ActiveAxis = m_HoveredAxis;
    m_IsDragging = true;
    std::cout << "TC: Started " << (m_Mode == GizmoMode::Translate ? "translation" : m_Mode == GizmoMode::Rotate ? "rotation" : "scaling") << " drag\n";
    return true;
}

glm::vec3 TransformGizmo::UpdateDrag(const Camera& camera, float mouseX, float mouseY,
    int width, int height, bool snapping)
{
    if (!m_IsDragging || m_Mode != GizmoMode::Translate) return glm::vec3(0.0f);
    float current = 0.0f;
    if (!GetAxisParameter(camera, m_DragOrigin, m_ActiveAxis, mouseX, mouseY, width, height, current)) return glm::vec3(0.0f);
    float total = current - m_LastDragParameter;
    float target = snapping ? std::round(total / GRID_SNAP_SIZE) * GRID_SNAP_SIZE : total;
    float delta = target - m_LastAppliedMovement;
    m_LastAppliedMovement = target;
    return AxisDirection(m_ActiveAxis) * delta;
}

float TransformGizmo::UpdateRotationDrag(const Camera& camera, float mouseX, float mouseY,
    int width, int height, bool snapping)
{
    if (!m_IsDragging || m_Mode != GizmoMode::Rotate) return 0.0f;
    glm::vec3 current;
    if (!GetRotationVector(camera, m_DragOrigin, m_ActiveAxis, mouseX, mouseY, width, height, current)) return 0.0f;
    glm::vec3 axis = AxisDirection(m_ActiveAxis);
    float radians = std::atan2(glm::dot(axis, glm::cross(m_InitialRotationVector, current)),
        glm::dot(m_InitialRotationVector, current));
    float totalDegrees = glm::degrees(radians);
    if (snapping) totalDegrees = std::round(totalDegrees / ROTATION_SNAP_DEGREES) * ROTATION_SNAP_DEGREES;
    float delta = totalDegrees - m_LastAppliedRotation;
    m_LastAppliedRotation = totalDegrees;
    return delta;
}

float TransformGizmo::UpdateScaleDrag(const Camera& camera, float mouseX, float mouseY,
    int width, int height, bool snapping)
{
    if (!m_IsDragging || m_Mode != GizmoMode::Scale || width <= 0 || height <= 0) return 0.0f;
    float total = 0.0f;
    if (m_ActiveAxis == GizmoAxis::Uniform)
    {
        // Dragging the centre handle vertically scales all three dimensions uniformly.
        total = (m_InitialMouseY - mouseY) * 0.01f;
    }
    else
    {
        float current = 0.0f;
        if (!GetAxisParameter(camera, m_DragOrigin, m_ActiveAxis, mouseX, mouseY, width, height, current)) return 0.0f;
        total = (current - m_LastDragParameter) / GIZMO_LENGTH;
    }
    if (snapping) total = std::round(total / SCALE_SNAP_SIZE) * SCALE_SNAP_SIZE;
    float delta = total - m_LastAppliedScale;
    m_LastAppliedScale = total;
    return delta;
}

void TransformGizmo::EndDrag() { if (m_IsDragging) std::cout << "TC: Finished gizmo drag\n"; m_IsDragging = false; }
bool TransformGizmo::IsDragging() const { return m_IsDragging; }
GizmoAxis TransformGizmo::GetHoveredAxis() const { return m_HoveredAxis; }
GizmoAxis TransformGizmo::GetActiveAxis() const { return m_ActiveAxis; }
void TransformGizmo::SetActiveAxis(GizmoAxis axis) { m_ActiveAxis = axis; }
void TransformGizmo::ClearActiveAxis() { m_ActiveAxis = GizmoAxis::None; }
void TransformGizmo::SetMode(GizmoMode mode) { if (!m_IsDragging) { m_Mode = mode; m_HoveredAxis = GizmoAxis::None; m_ActiveAxis = GizmoAxis::None; } }
GizmoMode TransformGizmo::GetMode() const { return m_Mode; }

void TransformGizmo::Render(const Camera& camera, const glm::vec3& position, float aspectRatio)
{
    m_Shader.Use();
    float scale = CalculateGizmoScale(camera, position);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = glm::scale(model, glm::vec3(scale));
    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
    m_Shader.SetInt("u_LightingEnabled", 0);
    glBindVertexArray(m_VAO);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);
    auto colorFor = [&](GizmoAxis axis, const glm::vec3& base) {
        if (m_ActiveAxis == axis) return glm::vec3(1.0f, 0.65f, 0.05f);
        if (m_HoveredAxis == axis) return glm::vec3(1.0f, 1.0f, 0.15f);
        return base;
    };
    const GizmoAxis axes[] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
    const glm::vec3 colors[] = {{1.0f, 0.1f, 0.1f}, {0.1f, 1.0f, 0.1f}, {0.1f, 0.3f, 1.0f}};

    if (m_Mode == GizmoMode::Translate)
    {
        const float vertices[] = {
            0,0,0, 1.3f,0,0,  1.3f,0,0, 1.12f,0.08f,0, 1.12f,0,-0.08f,
            0,0,0, 0,1.3f,0,  0,1.3f,0, 0.08f,1.12f,0, 0,1.12f,0.08f,
            0,0,0, 0,0,1.3f,  0,0,1.3f, 0.08f,0,1.12f, 0,0.08f,1.12f
        };
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        for (int i = 0; i < 3; ++i)
        {
            m_Shader.SetVec3("u_Color", colorFor(axes[i], colors[i]));
            glDrawArrays(GL_LINES, i * 5, 2);
            glDrawArrays(GL_TRIANGLES, i * 5 + 2, 3);
        }
    }
    else if (m_Mode == GizmoMode::Rotate)
    {
        std::vector<float> vertices;
        vertices.reserve(3 * (RING_SEGMENTS + 1) * 3);
        for (GizmoAxis axis : axes)
        {
            for (int i = 0; i <= RING_SEGMENTS; ++i)
            {
                float angle = 6.28318530718f * float(i) / float(RING_SEGMENTS);
                float c = std::cos(angle) * GIZMO_LENGTH;
                float s = std::sin(angle) * GIZMO_LENGTH;
                glm::vec3 p;
                if (axis == GizmoAxis::X) p = {0, c, s};
                else if (axis == GizmoAxis::Y) p = {c, 0, s};
                else p = {c, s, 0};
                vertices.insert(vertices.end(), {p.x, p.y, p.z});
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        int ringCount = RING_SEGMENTS + 1;
        for (int i = 0; i < 3; ++i)
        {
            m_Shader.SetVec3("u_Color", colorFor(axes[i], colors[i]));
            glDrawArrays(GL_LINE_STRIP, i * ringCount, ringCount);
        }
    }
    else
    {
        // Scale handles: axis shafts with square endpoint handles, plus a centre uniform handle.
        const float vertices[] = {
            0,0,0, 1.3f,0,0,  1.3f,0,0, 1.3f,0.12f,0, 1.3f,0,0, 1.3f,0,0.12f,
            0,0,0, 0,1.3f,0,  0,1.3f,0, 0.12f,1.3f,0, 0,1.3f,0, 0,1.3f,0.12f,
            0,0,0, 0,0,1.3f,  0,0,1.3f, 0.12f,0,1.3f, 0,0,1.3f, 0,0.12f,1.3f
        };
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        for (int i = 0; i < 3; ++i)
        {
            m_Shader.SetVec3("u_Color", colorFor(axes[i], colors[i]));
            glDrawArrays(GL_LINES, i * 6, 6);
        }
        const float center[] = {0,0,0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(center), center, GL_DYNAMIC_DRAW);
        m_Shader.SetVec3("u_Color", colorFor(GizmoAxis::Uniform, glm::vec3(1.0f)));
        glPointSize(13.0f);
        glDrawArrays(GL_POINTS, 0, 1);
        glPointSize(1.0f);
    }

    glLineWidth(1.0f);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void TransformGizmo::Shutdown()
{
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    m_VAO = m_VBO = 0;
    m_IsDragging = false;
}
}
