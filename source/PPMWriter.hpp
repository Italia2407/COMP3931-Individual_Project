#include <glm/glm.hpp>

#include <string>
#include <vector>

bool WriteToPPM(std::string fileName, int imgWidth, int imgHeight, std::vector<glm::vec3> pixels);