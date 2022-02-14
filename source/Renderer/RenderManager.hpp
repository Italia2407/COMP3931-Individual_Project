#pragma once

#include <embree3/rtcore.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "../IOManagers/MeshGeometry.hpp"
#include "PointLight.hpp"

struct Camera
{
public:
    Camera(glm::vec3 position, float fov, float np, float fp);

    glm::vec3 position;

    float fieldOfView;

    float nearPlane;
    float farPlane;

public:
    glm::vec3 getPixelRayDirection(int x, int y, u_int16_t imgWidth, u_int16_t imgHeight);
};

class RenderManager
{
public:
    RenderManager(RTCDevice* device, Camera camera, u_int16_t maxRayDepth);

private:
    RTCDevice* m_device;
    RTCScene m_scene;

    Camera m_camera;

    u_int16_t m_maxRayDepth;

    std::vector<MeshGeometry*> m_meshObjects;
    MeshProperties getMeshGeometryProperties(int meshGeometryID) { return m_meshObjects[meshGeometryID]->properties(); }

    std::vector<PointLight> m_sceneLights;

public:
    void AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position);
    void AddLight(glm::vec3 position, glm::vec3 colour, float intensity);

    void RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight);

private:
    glm::vec3 TraceRay(glm::vec3 origin, glm::vec3 direction, float near, float far, u_int16_t& rayDepth);

    glm::vec3 CastShadowRays(glm::vec3 hitPosition, glm::vec3 surfaceNormal, glm::vec3 reflectionDirection, MeshProperties surfaceProperties);
};