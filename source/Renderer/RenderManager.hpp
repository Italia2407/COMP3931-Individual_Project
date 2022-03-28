#pragma once

#include <embree3/rtcore.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "../IOManagers/MeshGeometry.hpp"
#include "PointLight.hpp"
#include "PhotonMapper.hpp"

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

struct RayHitPoint
{
    glm::vec3 position;
    glm::vec3 surfaceNormal;
    glm::vec3 incidentDirection;
    MaterialProperties surfaceProperties;

    glm::uvec2 imageLocation;

    float photonRadius;
    int photonCount;
    glm::vec3 accumulatedFlux;
};

class RenderManager
{
public:
    RenderManager(RTCDevice* device, Camera camera, bool smoothShading, u_int32_t multisamplingIterations, u_int16_t maxRayDepth);

private:
    RTCDevice* m_device;
    RTCScene m_scene;

    PhotonMapper* m_photonMapper;

    Camera m_camera;
    bool m_smoothShading;

    u_int32_t m_multisamplingIterations;
    u_int16_t m_maxRayDepth;

    std::vector<MeshGeometry*> m_meshObjects;
    MaterialProperties getMeshGeometryProperties(int meshGeometryID) { return m_meshObjects[meshGeometryID]->properties(); }

    std::vector<PointLight> m_sceneLights;

    std::vector<RayHitPoint*> m_hitPoints;

    float m_alphaReduction = 0.75f;

public:
    void AttachMeshGeometry(MeshGeometry* meshGeometry, glm::vec3 position);
    void AddLight(glm::vec3 position, glm::vec3 colour, float intensity);

    void RenderScene(std::string outputFileName, u_int32_t imgWidth, u_int32_t imgHeight);

private:
    //glm::vec3 TraceRay(glm::vec3 origin, glm::vec3 direction, float near, float far, u_int16_t& rayDepth);
    glm::vec3 CastRay(glm::vec3 origin, glm::vec3 direction, float near, float far, RTCIntersectContext& context, u_int16_t rayDepth, glm::uvec2 imageLocation);

    glm::vec3 CalculateDiffuseColour(glm::vec3 hitPoint, glm::vec3 surfaceNormal, glm::vec3 reflectionDirection, PointLight light, MaterialProperties surfaceProperties, RTCIntersectContext& context);
    glm::vec3 CalculateCausticColour(RayHitPoint* rayHitPoint, RTCIntersectContext& context, int iterationNum);
    glm::vec3 CalculateReflectionColour(glm::vec3 hitPoint, glm::vec3 reflectionDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth, glm::uvec2 imageLocation);
    glm::vec3 CalculateRefractionColour(glm::vec3 hitPoint, glm::vec3 surfaceNormal, glm::vec3 incidenceDirection, MaterialProperties surfaceProperties, RTCIntersectContext& context, u_int32_t rayDepth, glm::uvec2 imageLocation);
};