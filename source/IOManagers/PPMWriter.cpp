#include "PPMWriter.hpp"

#include <fstream>
#include <iostream>

bool WriteToPPM(std::string fileName, u_int32_t imgWidth, u_int32_t imgHeight, std::vector<glm::vec3> pixels)
{
    std::ofstream outputFile(fileName);

    outputFile << "P3\n";
    outputFile << imgWidth << " " <<imgHeight << "\n";
    outputFile << "255\n\n";

    for (int i = 0; i < pixels.size(); i++)
    {
        outputFile << (int)(pixels[i].r * 255) << " ";
        outputFile << (int)(pixels[i].g * 255) << " ";
        outputFile << (int)(pixels[i].b * 255);

        if (i+1 % imgWidth == 0)
            outputFile << "\n";
        else
            outputFile << " ";
    }

    outputFile.close();

    return true;
}