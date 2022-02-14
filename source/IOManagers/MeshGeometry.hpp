#pragma once

#include <embree3/rtcore.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct MeshProperties
{
public:
    MeshProperties() {}

    glm::vec3 albedo;

    float reflectivity;
    float translucency;

    float glossiness;
    float glossFalloff;

    float refractionIndex;
};

class MeshGeometry
{
public:
    MeshGeometry(MeshProperties properties);

private:
    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec3> m_normals;

    std::vector<glm::uvec3> m_faceVID;
    std::vector<glm::uvec3> m_faceNID;

    MeshProperties m_properties;

public:
    bool LoadFromOBJ(std::string fileName);

    void CalculateBarycentricOfFace(u_int32_t faceID, glm::vec3 point, glm::vec3 normal, float& a, float& b, float& c);

public:
    const std::vector<glm::vec3>& vertices() { return m_vertices; }
    const std::vector<glm::vec3>& normals() { return m_normals; }

    const std::vector<glm::uvec3>& faceVIDs() { return m_faceVID; }
    const std::vector<glm::uvec3>& faceNIDs() { return m_faceNID; }

    const MeshProperties& properties() { return m_properties; }
};