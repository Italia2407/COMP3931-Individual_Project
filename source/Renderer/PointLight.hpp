#pragma once

#include <glm/glm.hpp>

struct PointLight
{
public:
    PointLight(glm::vec3 position, glm::vec3 colour, float intensity);

public:
    glm::vec3 position;

    glm::vec3 colour;
    float intensity;

public:
    glm::vec3 GetDirectionFromPoint(glm::vec3 point);
    float GetDistanceFromPoint(glm::vec3 point);
};