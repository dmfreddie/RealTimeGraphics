#pragma once

#include <glm/glm.hpp>

struct PBRMaterial
{
	glm::vec3 diffuseColour;
	float metallic;
	glm::vec3 specularColour;
	float roughness = 0.5f;
	float vertexShineyness;
	float ambientOcclusion;
	int diffuseTextureID;
	float padding = 0.0f;
};


struct PBRMaterialDataBlock
{
	PBRMaterial materials[30];
};