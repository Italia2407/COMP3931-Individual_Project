#include "PointLight.hpp"

PointLight::PointLight(glm::vec3 position, glm::vec3 colour, float intensity) :
    position(position), colour(colour), intensity(intensity) {}

glm::vec3 PointLight::GetDirectionFromPoint(glm::vec3 point)
{
    return position - point;
}

float PointLight::GetDistanceFromPoint(glm::vec3 point)
{
    return glm::length(GetDirectionFromPoint(point));
}