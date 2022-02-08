#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

bool WriteToPPM(std::string fileName, u_int32_t imgWidth, u_int32_t imgHeight, std::vector<glm::vec3> pixels);