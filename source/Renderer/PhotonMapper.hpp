#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <rtcore.h>

#include "RenderManager.hpp"
#include "PointLight.hpp"

struct Photon
{
    glm::vec3 position;
    glm::vec3 direction;

    glm::vec3 colour;
};

class PhotonMapper
{
public:
    PhotonMapper(RenderManager* renderer, int photonNumber, int maxBounces);

private:
    std::vector<Photon> m_photons;

    RenderManager* m_renderer;

    int m_photonNumber;
    int m_maxBounces;

public:
    void GeneratePhotons();

private:
    void CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCIntersectContext& context, int rayDepth);
};