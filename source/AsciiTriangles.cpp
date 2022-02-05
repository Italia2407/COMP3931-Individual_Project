#include <embree3/rtcore.h>
#include <iostream>

#include <vector>

int main()
{
    RTCDevice device = rtcNewDevice(NULL);
    RTCScene scene = rtcNewScene(device);

    // Create Scene Geometry
    RTCGeometry tetraforceGeo = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = (float*)rtcSetNewGeometryBuffer(tetraforceGeo, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3*sizeof(float), 5);
    vertices[ 0] =  0.0f; vertices[ 1] =  1.0f; vertices[ 2] =  0.0f;
    vertices[ 3] = -1.0f; vertices[ 4] =  0.0f; vertices[ 5] =  0.0f;
    vertices[ 6] =  0.0f; vertices[ 7] =  0.0f; vertices[ 8] =  0.0f;
    vertices[ 9] =  1.0f; vertices[10] =  0.0f; vertices[11] =  0.0f;
    vertices[12] =  0.0f; vertices[13] = -1.0f; vertices[14] =  0.0f;


    std::vector<char> faceCharacters = std::vector<char>();
    unsigned int* faces = (unsigned int*)rtcSetNewGeometryBuffer(tetraforceGeo, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3*sizeof(unsigned int), 4);
    faceCharacters.push_back('*'); faces[ 0] = 0; faces[ 1] = 1; faces[ 2] = 2;
    faceCharacters.push_back(':'); faces[ 3] = 0; faces[ 4] = 2; faces[ 5] = 3;
    faceCharacters.push_back('@'); faces[ 6] = 1; faces[ 7] = 4; faces[ 8] = 2;
    faceCharacters.push_back('*'); faces[ 9] = 2; faces[10] = 4; faces[11] = 3;

    rtcCommitGeometry(tetraforceGeo);
    rtcAttachGeometry(scene, tetraforceGeo);
    rtcReleaseGeometry(tetraforceGeo);
    rtcCommitScene(scene);

    for (float x = -2.0f; x <= 2.0f; x += 0.1f)
    {
        for (float y = -2.0f; y <= 2.0f; y += 0.1f)
        {
            RTCRayHit rayhit;
            rayhit.ray.org_x = x; rayhit.ray.org_y = y; rayhit.ray.org_z = -1.0f;
            rayhit.ray.dir_x = 0.0f; rayhit.ray.dir_y = 0.0f; rayhit.ray.dir_z = 1.0f;
            rayhit.ray.tnear = 0.01f;
            rayhit.ray.tfar = 100.0f;
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);
            rtcIntersect1(scene, &context, &rayhit);

            if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
            {
                std::cout << faceCharacters[rayhit.hit.primID];
            }
            else
            {
                std::cout << ' ';
            }
        }
        std::cout << std::endl;
    }

    return 0;
}