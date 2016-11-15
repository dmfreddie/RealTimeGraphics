#include "Shader.h"
#include <iostream>
#include <tygra/FileHelper.hpp>
#include <tgl/tgl.h>


Shader::Shader(const char* vertexShaderPath, const char* fragMentShaderPath)
{
	m_shaderProgram = glCreateProgram();
	CompileShader(vertexShaderPath, GL_VERTEX_SHADER, m_vertexShader);
	CompileShader(fragMentShaderPath, GL_FRAGMENT_SHADER, m_fragmentShader);

	
	glAttachShader(m_shaderProgram, m_vertexShader);
	glDeleteShader(m_vertexShader);
	glAttachShader(m_shaderProgram, m_fragmentShader);
	glDeleteShader(m_fragmentShader);
	glLinkProgram(m_shaderProgram);

	if (CheckLinkStatus(m_shaderProgram))
		std::cout << vertexShaderPath << " & " << fragMentShaderPath << " shaders compiled!" << std::endl;
}
Shader::~Shader()
{
	glDeleteProgram(m_shaderProgram);
}

void Shader::BindAttributeLocation(int location, const char* name)
{
	glBindAttribLocation(m_shaderProgram, location, name);
}

unsigned int Shader::GetUniformLocation(const char* name)
{
	m_uniforms[name] = glGetUniformLocation(m_shaderProgram, name);
	return m_uniforms[name];
}

void Shader::SetUniformFloatValue(const char* name, float value)
{
	glUniform1f(m_uniforms[name], value);
}

void Shader::SetUniformIntValue(const char* name, int value)
{
	glUniform1i(m_uniforms[name], value);
}

void Shader::SetUniformMatrix4FValue(const char* name, glm::mat4 value)
{
	glUniformMatrix4fv(m_uniforms[name], 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable)
{
	GLint compile_status = 0;
	shaderVariable = glCreateShader(shaderType);
	std::string shader_string = tygra::createStringFromFile("resource:///" + shaderFileName);
	const char *shader_code = shader_string.c_str();
	glShaderSource(shaderVariable, 1, (const GLchar **)&shader_code, NULL);
	glCompileShader(shaderVariable);
	glGetShaderiv(shaderVariable, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetShaderInfoLog(shaderVariable, string_length, NULL, log);
		std::cerr << log << std::endl;
	}
}

bool Shader::CheckLinkStatus(GLuint shaderProgram)
{
	GLint link_status = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetProgramInfoLog(shaderProgram, string_length, NULL, log);
		std::cerr << log << std::endl;
		return false;
	}
	return true;
}

void Shader::Bind()
{
	glUseProgram(m_shaderProgram);
}

void Shader::Unbind()
{
	glUseProgram(0);
}