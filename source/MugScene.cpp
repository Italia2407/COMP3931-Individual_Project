#include <embree3/rtcore.h>
#include "RenderManager.hpp"
#include "IOManagers/MeshGeometry.hpp"
#include "IOManagers/PPMWriter.hpp"

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RenderManager renderer(&device, -4.0f, 4.0f, -4.0f, 4.0f, 8);

    MeshProperties spoonMaterial(glm::vec3(0.5f, 0.5f, 0.52f));
    MeshGeometry* spoon = new MeshGeometry(spoonMaterial);
    spoon->LoadFromOBJ("../assets/Spoon.obj");

    renderer.AttachMeshGeometry(spoon, glm::vec3(2.0f, 0.0f, 0.0f));
    renderer.AddLight(glm::vec3(2.0f, 0.0f, -2.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    renderer.RenderScene("Spoon_512x512.ppm", 512, 512);
}