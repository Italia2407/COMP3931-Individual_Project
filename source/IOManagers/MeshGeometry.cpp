#include "MeshGeometry.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

MeshGeometry::MeshGeometry(MeshProperties properties) :
    m_vertices(std::vector<glm::vec3>()), m_normals(std::vector<glm::vec3>()), m_faceVID(std::vector<glm::uvec3>()), m_faceNID(std::vector<glm::uvec3>()), m_properties(properties) {}

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
		else if (word == "vn")
		{
			std::string n0, n1, n2;
			wordStream >> n0;
			wordStream >> n1;
			wordStream >> n2;

			glm::vec3 normal(std::stof(n0), std::stof(n1), std::stof(n2));
			m_normals.push_back(normal);
		}
		else if (word == "f")
		{
			std::string fc0, fc1, fc2;
			wordStream >> fc0; std::istringstream fs0(fc0); std::string f0, fn0; std::getline(fs0, f0, '/'); std::getline(fs0, fn0, '/'); std::getline(fs0, fn0, '/');
			wordStream >> fc1; std::istringstream fs1(fc1); std::string f1, fn1; std::getline(fs1, f1, '/'); std::getline(fs1, fn1, '/'); std::getline(fs1, fn1, '/');
			wordStream >> fc2; std::istringstream fs2(fc2); std::string f2, fn2; std::getline(fs2, f2, '/'); std::getline(fs2, fn2, '/'); std::getline(fs2, fn2, '/');

			glm::uvec3 faceV(std::stoi(f0) - 1, std::stoi(f1) - 1, std::stoi(f2) - 1);
			glm::uvec3 faceN(std::stoi(fn0) - 1, std::stoi(fn1) - 1, std::stoi(fn2) - 1);
			m_faceVID.push_back(faceV);
			m_faceNID.push_back(faceN);
		}
	}

	file.close();
	return true;
}

void MeshGeometry::CalculateBarycentricOfFace(u_int32_t faceID, glm::vec3 point, glm::vec3 normal, float& a, float& b, float& c)
{
	glm::vec3 pa = m_vertices[m_faceVID[faceID].x] - point;
	glm::vec3 pb = m_vertices[m_faceVID[faceID].y] - point;
	glm::vec3 pc = m_vertices[m_faceVID[faceID].z] - point;

	a = glm::length(glm::cross(pb, pc)) * glm::sign(glm::dot(glm::cross(pb, pc), normal));
	b = glm::length(glm::cross(pc, pa)) * glm::sign(glm::dot(glm::cross(pc, pa), normal));
	c = glm::length(glm::cross(pa, pb)) * glm::sign(glm::dot(glm::cross(pa, pb), normal));
}