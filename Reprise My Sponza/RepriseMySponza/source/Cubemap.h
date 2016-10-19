#pragma once
#include <vector>
#include <scene/scene_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

class CubeMap
{
public:
	CubeMap(const char* rightFace, const char* leftFace, const char* frontFace, const char* backFace, const char* topFace, const char* bottomFace, GLuint shaderHandle_, const char* handleName_);
	const GLuint GetTextureID() const;

protected:
	unsigned int LoadCubeMap(std::vector<const char*>& faces);
	unsigned int texureID = 0;
	GLuint shaderHandle;
	const char* handleName;
};
