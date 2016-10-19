#pragma once

#include "Cubemap.h"


class Skybox : public CubeMap 
{
public:
	Skybox(const char* rightFace, const char* leftFace, const char* frontFace, const char* backFace, const char* topFace, const char* bottomFace, GLuint shaderHandle_, const char* handleName_);
	
	void Bind();
	void Unbind();
protected:
	unsigned int VAO, VBO;
			
};


