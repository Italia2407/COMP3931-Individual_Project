#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <embree3/rtcore.h>

#include "../IOManagers/MeshGeometry.hpp"
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
    PhotonMapper(std::vector<MeshGeometry*>* meshObjects, bool smoothSurfaces, int photonNumber, int maxBounces);

private:
    std::vector<Photon> m_photons;

    std::vector<MeshGeometry*>* m_meshObjects;

    bool m_smoothSurfaces;
    int m_photonNumber;
    int m_maxBounces;

public:
    const std::vector<Photon>& photons() { return m_photons; };

    void GeneratePhotons(PointLight light, RTCScene scene);

private:
    void CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCScene scene, RTCIntersectContext& context, int rayDepth);
};