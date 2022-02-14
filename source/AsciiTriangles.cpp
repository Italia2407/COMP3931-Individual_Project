#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"

#include "IOManagers/PPMWriter.hpp"

#include <iostream>
#include <vector>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, Camera(glm::vec3(0.0f, 0.0f, 0.0f), 30.0f, 0.01f, 1000.0f), false, 8);

    MeshProperties shader = MeshProperties();
    MeshGeometry* TriForce = new MeshGeometry(MeshProperties(shader));
    TriForce->LoadFromOBJ("../assets/TriForce.obj");

    renderer.AttachMeshGeometry(TriForce, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.RenderScene("Image_256x256.ppm", 256, 256);
}