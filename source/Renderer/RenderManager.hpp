#pragma once

#include <embree3/rtcore.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "../IOManagers/MeshGeometry.hpp"
#include "PointLight.hpp"

class RenderManager
{
public:
    RenderManager(RTCDevice* device, float borderL, float borderR, float borderT, float borderB, u_int16_t maxRayDepth);

private:
    RTCDevice* m_device;
    RTCScene m_scene;

    float m_borderL;
    float m_borderR;
    float m_borderT;
    float m_borderB;

    u_int16_t m_maxRayDepth;

    std::vector<MeshGeometry*> m_meshObjects;
    MeshProperties getMeshGeometryProperties(int meshGeometryID) { return m_meshObjects[meshGeometryID]->properties(); }

    std::vector<PointLight> m_sceneLights;

public:
    void AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position);
    void AddLight(glm::vec3 position, glm::vec3 colour, float intensity);

    void RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight);

private:
    glm::vec3 TraceRay(glm::vec3 origin, glm::vec3 direction, u_int16_t& rayDepth);

    glm::vec3 CastShadowRays(glm::vec3 origin, glm::vec3 normal, glm::vec3 pointColour);
};