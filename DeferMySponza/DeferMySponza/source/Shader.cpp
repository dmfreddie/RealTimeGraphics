#include "Shader.h"
#include <iostream>
#include <tygra/FileHelper.hpp>
#include <tgl/tgl.h>
#include <fstream>


Shader::Shader(const char* vertexShaderPath, const char* fragMentShaderPath, bool directEntry)
{
	m_shaderProgram = glCreateProgram();
	CompileShader(vertexShaderPath, GL_VERTEX_SHADER, m_vertexShader, directEntry);
	CompileShader(fragMentShaderPath, GL_FRAGMENT_SHADER, m_fragmentShader, directEntry);

	
	glAttachShader(m_shaderProgram, m_vertexShader);
	glDeleteShader(m_vertexShader);
	glAttachShader(m_shaderProgram, m_fragmentShader);
	glDeleteShader(m_fragmentShader);
	glLinkProgram(m_shaderProgram);

	if (CheckLinkStatus(m_shaderProgram) && !directEntry)
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
		glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniformIntValue(const char* name, int value)
{
		glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniformMatrix4FValue(const char* name, glm::mat4 value)
{
	GLuint loc = GetUniformLocation(name);
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable, bool directEntry)
{
	GLint compile_status = 0;
	shaderVariable = glCreateShader(shaderType);
	std::string shader_string;

	if(directEntry)
	{
		shader_string = shaderFileName;
		shader_include(shader_string);
		replace_all(shader_string, "hash ", "#");
	}
	else
	{
		shader_string = tygra::createStringFromFile("resource:///" + shaderFileName);
	}
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

bool Shader::UniformExistsInMap(const char* name, bool addToMapIfExists)
{
	if (m_uniforms.find(name) == m_uniforms.end())
	{
		if (addToMapIfExists)
		{
			GetUniformLocation(name);
			return true;
		}
		return false;
	}
	else
		return true;
}

const unsigned int Shader::GetShaderID() const
{
	return m_shaderProgram;
}

//this function adds #include functionality to GLSL
void Shader::shader_include(std::string& shader)
{
	size_t start_pos = 0;
	std::string include_dir = "#include ";

	while ((start_pos = shader.find(include_dir, start_pos)) != std::string::npos)
	{
		int pos = start_pos + include_dir.length() + 1;
		int length = shader.find("\"", pos);
		std::string file = shader.substr(pos, length - pos);
		std::string content = "";

		std::ifstream f;
		f.open(file.c_str());

		if (f.is_open())
		{
			char buffer[1024];

			while (!f.eof())
			{
				f.getline(buffer, 1024);
				content += buffer;
				content += "\n";
			}
		}
		else
		{
			std::cerr << "Couldn't include shader file: " << file << "\n";
		}

		shader.replace(start_pos, (length + 1) - start_pos, content);
		start_pos += content.length();
	}
}

//replaces all occurances of a string in another string
void Shader::replace_all(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;

	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}