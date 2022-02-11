#include <embree3/rtcore.h>
#include "Renderer/RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

#include <iostream>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, -4.0f, 4.0f, -4.0f, 4.0f, 8);

    MeshProperties spoonMaterial(glm::vec3(1.0f, 1.0f, 1.0f));
    MeshGeometry* spoon = new MeshGeometry(spoonMaterial);
    spoon->LoadFromOBJ("../assets/Sphere.obj");

    renderer.AttachMeshGeometry(spoon, glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.AddLight(glm::vec3(0.0f, -1.5f, -1.3f), glm::vec3(1.0f, 0.5f, 0.0f), 16.0f);
    renderer.AddLight(glm::vec3(2.0f, 0.0f, -1.5f), glm::vec3(0.0f, 0.0f, 1.0f), 16.0f);
    renderer.RenderScene("Sphere.ppm", 2048, 2048);
}