#pragma once
#define TGL_TARGET_GL_4_4
#include <scene/scene_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include "Shader.h"
#include <map>

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

struct DirectionalLightDataBlock
{
	DirectionalLight directionalLights[3];
	glm::vec3 cameraPosition;
	float maxDirectional;
};

struct AmbientLightBlock
{
	glm::vec3 cameraPosition;
	glm::vec3 ambient_light;;
};

struct PointLightDataBlock
{
	PointLight pointLight;
	glm::vec3 cameraPosition;
	float maxPointLights;
};

struct DataBlock
{
	DirectionalLight directionalLights[3];
	glm::vec3 globalAmbientLight;
	float maxPointLights;
	glm::vec3 cameraPosition;
	float maxDirectionalLights;
	float maxSpotlights;
};

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

	void Stop(tygra::Window* window);
private:

    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;

    const scene::Context * scene_;
	DrawElementsIndirectCommand commands[30];
	std::map<scene::MeshId, MeshGL> meshes_;
	std::unordered_map<std::string, GLuint> textures;
	std::unordered_map<std::string, GLuint> uniforms;
	std::vector<glm::mat4> matrices;

	GLuint vao; // VertexArrayObject for the shape's vertex array settings
	GLuint vertex_vbo;
	GLuint element_vbo; // VertexBufferObject for the elements (indices)
	GLuint instance_vbo; // VertexBufferObject for the model xforms
	GLuint material_vbo;
	GLuint commandBuffer;

#pragma region GBuffer
	struct Mesh
	{
		GLuint vertex_vbo{ 0 };
		GLuint element_vbo{ 0 };
		GLuint vao{ 0 };
		int element_count{ 0 };
	};

	Mesh light_quad_mesh_; // vertex array of vec2 position
	Mesh light_sphere_mesh_; // element array into vec3 position
	Mesh light_cone_mesh_; // element array into vec3 position

	GLuint gbuffer_position_tex_{ 0 };
	GLuint gbuffer_normal_tex_{ 0 };
	GLuint gbuffer_depth_tex_{ 0 };
	GLuint gbuffer_material_tex_{ 0 };

	GLuint lbuffer_fbo_{ 0 };
	GLuint lbuffer_colour_rbo_{ 0 };
#pragma endregion 

#pragma  region Shaders
	Shader *gbufferShadr, *ambientLightShader, *pointLightShader, *directionalLightShader;
#pragma endregion 

	DirectionalLightDataBlock directionalLightDataBlock;
	AmbientLightBlock ambientLightBlock;
	PointLightDataBlock pointLightBlock;

	GLuint ambientLightUBO;
	GLuint directionalLightBlockUBO;
	GLuint pointLightBlockUBO;
};
