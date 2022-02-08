#pragma once

#include <embree3/rtcore.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "IOManagers/MeshGeometry.hpp"

class RenderManager
{
public:
    RenderManager(RTCDevice* device, float borderL, float borderR, float borderT, float borderB);

private:
    RTCDevice* m_device;
    RTCScene m_scene;

    float m_borderL;
    float m_borderR;
    float m_borderT;
    float m_borderB;

    std::vector<MeshGeometry*> m_meshObjects;
    MeshProperties getMeshGeometryProperties(int meshGeometryID) { return m_meshObjects[meshGeometryID]->properties(); }

public:
    void AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position);

    void RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight);
};