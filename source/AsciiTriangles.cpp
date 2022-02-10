#include <embree3/rtcore.h>
#include "RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"

#include "IOManagers/PPMWriter.hpp"

#include <iostream>
#include <vector>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, -4.0f, 4.0f, -4.0f, 4.0f, 8);

    MeshProperties shader = MeshProperties(glm::vec3(1.0f, 1.0f, 1.0f));
    MeshGeometry* TriForce = new MeshGeometry(MeshProperties(shader));
    TriForce->LoadFromOBJ("../assets/TriForce.obj");

    renderer.AttachMeshGeometry(TriForce, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.RenderScene("Image_256x256.ppm", 256, 256);
}