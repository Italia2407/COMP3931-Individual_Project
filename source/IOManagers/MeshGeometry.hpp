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
    std::vector<glm::uvec3> m_faces;

    MeshProperties m_properties;

public:
    bool LoadFromOBJ(std::string fileName);

public:
    const std::vector<glm::vec3>& vertices() { return m_vertices; }
    const std::vector<glm::uvec3>& faces() { return m_faces; }

    const MeshProperties& properties() { return m_properties; }
};