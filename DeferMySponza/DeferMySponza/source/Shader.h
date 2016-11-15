#pragma once
#include <GL/glcorearb.h>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
	Shader(const char* vertexShaderPath, const char* fragMentShaderPath);
	Shader(){};
	~Shader();

	void BindAttributeLocation(int location, const char* name);
	unsigned int GetUniformLocation(const char* name);
	void SetUniformFloatValue(const char* name, float value);
	void SetUniformIntValue(const char* name, int value);
	void SetUniformMatrix4FValue(const char* name, glm::mat4 value);

	void Bind();
	void Unbind();
private:
	void CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable);
	bool CheckLinkStatus(GLuint shaderProgram);

private:
	unsigned int m_vertexShader = 0, m_fragmentShader = 0, m_shaderProgram = 0;
	std::unordered_map<const char*, unsigned int> m_uniforms;
};
