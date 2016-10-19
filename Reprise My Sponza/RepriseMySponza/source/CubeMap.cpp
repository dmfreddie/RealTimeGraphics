#include "Cubemap.h"
#include <scene/scene.hpp>
#include <tygra/FileHelper.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>

CubeMap::CubeMap(const char* rightFace, const char* leftFace, const char* frontFace, const char* backFace, const char* topFace, const char* bottomFace, GLuint shaderHandle_, const char* handleName_)
{
	std::vector<const char*> faces;
	faces.reserve(sizeof(const char*)* 6);
	faces.push_back(rightFace);
	faces.push_back(leftFace);
	faces.push_back(topFace);
	faces.push_back(bottomFace);
	faces.push_back(backFace);
	faces.push_back(frontFace);
			
	shaderHandle = shaderHandle_;
	handleName = handleName_;
	texureID = LoadCubeMap(faces);

}

const GLuint CubeMap::GetTextureID() const
{
	return texureID;
}

unsigned int CubeMap::LoadCubeMap(std::vector<const char*>& faces)
{
	glUseProgram(shaderHandle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texureID);
	glActiveTexture(GL_TEXTURE0 + texureID - 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texureID);
	auto textureArrayLocation = glGetUniformLocation(shaderHandle, handleName);
	glUniform1i(textureArrayLocation, 0);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			
	GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
	for (unsigned int i = 0; i < faces.size(); ++i)
	{
		tygra::Image texture_image = tygra::createImageFromPngFile(faces[i]);

		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, 
			GL_RGBA, 
			texture_image.width(),
			texture_image.height(), 
			0, 
			pixel_formats[texture_image.componentsPerPixel()],
			texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
			texture_image.pixelData()
			);
		
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
			std::cerr << err << std::endl;
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	return texureID;
}
