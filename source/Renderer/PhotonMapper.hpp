#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <embree3/rtcore.h>
#include "../../cdalitz-kdtree-cpp/kdtree.hpp"

#include "../IOManagers/MeshGeometry.hpp"
#include "PointLight.hpp"

struct PhotonData
{
    glm::vec3 direction;
    glm::vec3 colour;
};

struct Photon
{
    glm::vec3 position;
    PhotonData data;
};

class PhotonMapper
{
public:
    PhotonMapper(std::vector<MeshGeometry*>* meshObjects, bool caustics, int photonNumber, int maxBounces);

private:
    Kdtree::KdTree* m_photonTree;
    std::vector<Photon> m_photons;

    std::vector<MeshGeometry*>* m_meshObjects;

    bool m_caustics;
    int m_photonNumber;
    int m_maxBounces;

public:
    const Kdtree::KdTree& photons() { return *m_photonTree; };
    //const std::vector<Photon>& photons() { return m_photons; };

    int GeneratePhotons(PointLight light, RTCScene scene);
    void ClearPhotons();
    Kdtree::KdNodeVector GetClosestPhotons(glm::vec3 hitPoint, float maxDistance, int &numberPhotons);
    Kdtree::KdNodeVector GetClosestPhotons(glm::vec3 hitPoint, int maxNumber, float &photonDistance);

private:
    bool CastPhotonRay(glm::vec3 photonColour, glm::vec3 photonOrigin, glm::vec3 photonDirection, RTCScene scene, RTCIntersectContext& context, int rayDepth);
};