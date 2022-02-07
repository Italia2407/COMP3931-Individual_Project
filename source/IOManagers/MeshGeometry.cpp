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
			std::string f0, f1, f2;
			wordStream >> f0;
			wordStream >> f1;
			wordStream >> f2;

			glm::uvec3 face(std::stoi(f0), std::stoi(f1), std::stoi(f2));
			m_faces.push_back(face);
		}
	}
}