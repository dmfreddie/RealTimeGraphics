#pragma once
#define TGL_TARGET_GL_4_4
#include <scene/scene_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

#define MAX_SPOTLIGHTS 15
#define MAX_POINTLIGHTS 22
#define MAX_DIRECTIONALLIGHTS 5


// TODO: Texture Arrays are broken on loading

struct DrawElementsIndirectCommand
{
	GLuint vertexCount;
	GLuint instanceCount;
	GLuint firstVertex;
	GLuint baseVertex;
	GLuint baseInstance;
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

struct Material
{
	glm::vec3 diffuseColour;
	glm::vec3 specularColour;
	float vertexShineyness;
	int diffuseTextureID;
};

struct PointLight
{
	glm::vec3 position;
	float range;
	glm::vec3 intensity;
	float padding;

};

struct PointLights
{
	PointLight data[22];
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

struct SpotLights
{
	SpotLight data[15];
};

struct DirectionalLight
{
	glm::vec3 direction;
	float padding1;
	glm::vec3 intensity;
	float padding2;
};

struct DirectionalLights
{
	DirectionalLight data[5];
};

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

private:

    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;

	void CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable);
	bool CheckLinkStatus(GLuint shaderProgram);
	void CompileShaders();
	void Getuniforms();
	void LoadTexture(std::string textureName);
	void LoadTextureArray(std::vector<std::string>& textureNames);
	void ResetConsole();


    const scene::Context * scene_;

	std::map<scene::MeshId, MeshGL> meshes_;
	std::unordered_map<std::string, GLuint> textures;
	std::unordered_map<std::string, GLuint> uniforms;
	std::vector<glm::mat4> matrices;
	

	DrawElementsIndirectCommand commands[30];

	PointLights pointLights;
	DirectionalLights directionalLights;
	SpotLights spotLights;

	GLuint shaderProgram;
	GLuint vertex_shader;
	GLuint fragment_shader;



	GLuint vertex_vbo;
	GLuint element_vbo; // VertexBufferObject for the elements (indices)
	GLuint instance_vbo; // VertexBufferObject for the model xforms
	GLuint material_vbo;
	GLuint texture_vbo;

	GLuint vao; // VertexArrayObject for the shape's vertex array settings
	GLuint spotLightUBO;
	GLuint pointLightUBO;
	GLuint directionalLightUBO;
	GLuint commandBuffer;
};
