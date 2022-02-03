#include "MeshObject.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

std::vector<MeshObject> MeshObject::meshObjects = std::vector<MeshObject>();

MeshObject::MeshObject(RTCDevice* device, RTCScene* scene) :
    m_device(device), m_scene(scene), m_vertices(std::vector<glm::vec3>()), m_faces(std::vector<glm::vec3<unsigned int>>()) {}

bool MeshObject::LoadFromOBJ(std::string fileName)
{
    std::ifstream file(fileName);
	if (!file)unsigned int
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

			glm::vec3<unsigned int> face(std::stoi(f0), std::stoi(f1), std::stoi(f2));
			m_faces.push_back(face);
		}
	}
}

bool MeshObject::BindToScene(glm::vec3 position)
{
    if (m_device == nullptr || m_scene == nullptr)
        return false;
    
    RTCGeometry geometry = rtcNewGeometry(m_device, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), m_vertices.size());
    for (int i = 0; i < m_vertices.size(); i++)
    {
        vertices[i*3 + 0] = m_vertices[i].x + position.x;
        vertices[i*3 + 1] = m_vertices[i].y + position.y;
        vertices[i*3 + 2] = m_vertices[i].z + position.z;
    }

    unsigned int* faces = (unsigned int*)rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned int), m_faces.size());
    for (int i = 0; i < m_faces.size(); i++)
    {
        faces[i*3 + 0] = m_faces[i].x;
        faces[i*3 + 1] = m_faces[i].y;
        faces[i*3 + 2] = m_faces[i].z;
    }

    rtcCommitGeometry(geometry);
    rtcAttachGeometry(m_scene, geometry);
    rtcReleaseGeometry(geometry);

    MeshObject::meshObjects.push_back(this);
}