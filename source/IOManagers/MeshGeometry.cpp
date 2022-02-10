#include "MeshGeometry.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

MeshGeometry::MeshGeometry(MeshProperties properties) :
    m_vertices(std::vector<glm::vec3>()), m_faces(std::vector<glm::uvec3>()), m_properties(properties) {}

bool MeshGeometry::LoadFromOBJ(std::string fileName)
{
    std::ifstream file(fileName);
	if (!file)
		return false;

    std::string line, word;
	while (std::getline(file, line))
	{
		std::stringstream wordStream(line);
		wordStream >> word;

		if (word == "v")
		{
			std::string v0, v1, v2;
			wordStream >> v0;
			wordStream >> v1;
			wordStream >> v2;

			glm::vec3 vertex(std::stof(v0), std::stof(v1), std::stof(v2));
			m_vertices.push_back(vertex);
		}
		else if (word == "f")
		{
			std::string fc0, fc1, fc2;
			wordStream >> fc0; std::istringstream fs0(fc0); std::string f0; std::getline(fs0, f0, '/');
			wordStream >> fc1; std::istringstream fs1(fc1); std::string f1; std::getline(fs1, f1, '/');
			wordStream >> fc2; std::istringstream fs2(fc2); std::string f2; std::getline(fs2, f2, '/');

			glm::uvec3 face(std::stoi(f0) - 1, std::stoi(f1) - 1, std::stoi(f2) - 1);
			m_faces.push_back(face);
		}
	}

	file.close();
	return true;
}