#include "OBJLoader.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

bool OBJLoader::LoadObject(std::string fileName, MeshObject* mesh)
{
    std::ifstream file(fileName);
	if (!file)
		return false;

    
}