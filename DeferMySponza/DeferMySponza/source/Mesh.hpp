#pragma once

#include <glm/glm.hpp>
#include <tgl/tgl.h>

struct DrawElementsIndirectCommand
{
	GLuint vertexCount;
	GLuint instanceCount;
	GLuint firstVertex;
	GLuint baseVertex;
	GLuint baseInstance;
};

struct MeshVertex
{
	glm::vec2 uvcoords;
	glm::vec2 position;

};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texcoord;

};

struct MeshGL
{
	GLuint first_element_index;
	GLuint element_count;
	GLuint first_vertex_index;
};
