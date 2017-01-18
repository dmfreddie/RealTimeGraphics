#pragma once
#include <glm/glm.hpp>
struct DirectionalLight
{
	glm::vec3 direction;
	float padding1;
	glm::vec3 intensity;
	float padding2;
};

struct PointLight
{
	glm::vec3 position;
	float range;
	glm::vec3 intensity;
	float padding;
};

struct SpotLight
{
	glm::vec3 position;
	float range;
	glm::vec3 direction;
	float coneAngle;
	glm::vec3 intensity;
	bool castShadow;
};