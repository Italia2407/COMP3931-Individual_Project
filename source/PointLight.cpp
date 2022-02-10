#include "PointLight.hpp"

PointLight::PointLight(glm::vec3 position) :
    position(position), colour(glm::vec3(1.0f, 1.0f, 1.0f)) {}

PointLight::PointLight(glm::vec3 position, glm::vec3 colour) :
    position(position), colour(colour) {}